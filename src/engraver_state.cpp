
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2014 Dominik Lehmann
  
  Licensed under the EUPL, Version 1.1 or - as soon they
  will be approved by the European Commission - subsequent
  versions of the EUPL (the "Licence");
  You may not use this work except in compliance with the
  Licence. You may obtain a copy of the Licence at
  <http://ec.europa.eu/idabc/eupl/>.
  
  Unless required by applicable law or agreed to in
  writing, software distributed under the Licence is
  distributed on an "AS IS" basis, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either expressed or implied.
  See the Licence for the specific language governing
  permissions and limitations under the Licence.
*/

#include <set>                  // std::multiset

#include "engraver_state.hh"    // EngraverState
#include "undefined.hh"         // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                                // this number is interpreted as an undefined value
using namespace ScorePress;

inline int _round(const double d) {return static_cast<int>(d + 0.5);}
#define HEAD_HEIGHT(staff) ((staff).head_height ? (staff).head_height : this->def_head_height)

//
//     class EngraverState
//    =====================
//
// This class represents the internal state of an "Engraver" instance during
// the engraving process. It contains all the information necessary for the
// engraving of a single object.
// Internally an instance of "Pick" is used to calculate the object positions.
//

// engrave current note-object
void EngraverState::engrave()
{
    // set "pvoice" corresponding to the current pick
    const Pick::VoiceCursor& cursor = pick.get_cursor();
    if (cursor.at_end()) {log_warn("Cursor at end during engraving process. (class: EngraverState)"); return;};
    
    // get the voice on the plate
    pvoice = pline->get_voice(cursor.voice());  // get the voice
    if (pvoice == pline->voices.end())          // if the voice doesn't exists
    {
        // create the new voice
        if (cursor.is_main())  // for a main-voice, just...
        {
            pline->voices.push_back(Plate::pVoice(cursor));     // add new voice...
            pvoice = --pline->voices.end();                     // and get the voice
        }
        else    // for a sub-voice
        {
            std::list<Plate::pVoice>::iterator parent   // get parent voice
                = pline->get_voice(static_cast<const SubVoice&>(cursor.voice()).get_parent());
            
            if (parent != pline->voices.end())  // if it exists
            {
                if (static_cast<const SubVoice&>(cursor.voice()).on_top)    // check, where to insert
                {
                    pvoice = pline->voices.insert(parent, Plate::pVoice(cursor));
                }
                else
                {
                    pvoice = pline->voices.insert(++parent, Plate::pVoice(cursor));
                };
            }
            else
            {
                pline->voices.push_back(Plate::pVoice(cursor));     // add new voice...
                pvoice = --pline->voices.end();                     // and get the voice
            };
        };
        
        // search for the context
        bool got_ctx = false;               // set to true, if context is found
        if (pline != plate->lines.begin())  // if there is a previous line
        {                                   //     look there for the voice
            std::list<Plate::pVoice>::iterator prevoice = (--pline)->get_voice(cursor.voice());
            if (prevoice != pline->voices.end())    // and if it exists
            {
                pvoice->context = prevoice->context;    // copy the context
                pvoice->context.set_buffer(NULL);       // reset the buffer
                got_ctx = true;                         // got it!
            };
            ++pline;            // reset line iterator
        };
        
        // if the context could not be detected from the previous line
        // check in the parent voice
        if (!got_ctx && cursor.is_sub())    // if no context found and parent-voice exists
        {
            std::list<Plate::pVoice>::iterator parent;     // parent voice
            parent = pline->get_voice(static_cast<const SubVoice&>(cursor.voice()).get_parent()); // get parent
            
            if (parent != pline->voices.end())  // if we got the parent voice
            {
                pvoice->context = parent->context;  // copy the context
                pvoice->context.reset_buffer();     // reset the buffer
                got_ctx = true;                     // got it!
            }
            else if (pline != plate->lines.begin()) // else try to find it in the previous line
            {
                parent = (--pline)->get_voice(static_cast<const SubVoice&>(cursor.voice()).get_parent());
                if (parent != pline->voices.end())  // if it exists
                {
                    pvoice->context = parent->context;  // copy the context
                    pvoice->context.set_buffer(NULL);   // reset the buffer
                    got_ctx = true;                     // got it!
                };
                ++pline;        // reset line iterator
            };
            
            // if no context was found and we are a sub-voice, dump warn-message
            if (!got_ctx) log_warn("Unable to find context for new sub-voice. Using default context. (class: EngraverState)");
        };
        
        // add position information to the new voice
        pvoice->basePos.x = pick.get_indent();
        pvoice->basePos.y = cursor.ypos + viewport->umtopx_v(pick.staff_offset());
        pvoice->time = cursor.time;
        pvoice->head_height = _round(viewport->umtopx_v(HEAD_HEIGHT(cursor.staff())));
        if (pline->basePos.x > pvoice->basePos.x)
            pline->basePos.x = pvoice->basePos.x;
        if (pline->basePos.y > pvoice->basePos.y)
            pline->basePos.y = pvoice->basePos.y;
        
        // update external data
        if (reengrave_info)
            reengrave_info->update(cursor.voice(), *this);
    };
    
    // update end-time stamp
    pvoice->end_time = cursor.time;
    if (cursor->is(Class::NOTEOBJECT)) pvoice->end_time += static_cast<const NoteObject&>(*cursor).value();
    if (pvoice->end_time > end_time)   end_time = pvoice->end_time;
    
    // set staff-style and head-height
    style = (!!cursor.staff().style) ? &*cursor.staff().style : &default_style; // set staff-style
    
    // cut object on barline (if not necessary, method simply does nothing; no need to double check)
    pick.cut(pvoice->context.restbar(cursor.time));
    
    // create on-plate object (set "pnote")
    Position<mpx_t> pos(cursor.pos, cursor.ypos);
    if (cursor->is(Class::NOTEOBJECT)) pos.y += viewport->umtopx_v(pick.staff_offset(static_cast<const NoteObject&>(*cursor).staff_shift));
    else                               pos.y += viewport->umtopx_v(pick.staff_offset());
    
    pvoice->notes.push_back(Plate::pNote(pos, cursor)); // append the new note to the plate
    pnote = --pvoice->notes.end();                      // get the on-plate note-object
    if (cursor.virtual_obj)                             // set virtual object
        pnote->virtual_obj = Plate::pNote::VirtualPtr(new Plate::pNote::Virtual(*cursor.virtual_obj, cursor.inserted));
    
    // engrave object (polymorphically call "StaffObject::engrave")
    pnote->get_note().engrave(*this);
    
    // apply context modifying movables
    if (cursor->is(Class::VISIBLEOBJECT))
    {
        const VisibleObject& visible = cursor->get_visible();
        
        // apply context modifying movables
        for (MovableList::const_iterator i = visible.attached.begin(); i != visible.attached.end(); ++i)
        {
            if (!(*i)->ctxchange()) continue;
            if (!(*i)->ctxchange()->permanent) continue;
            
            for (std::list<Plate::pVoice>::iterator j = pline->voices.begin(); j != pline->voices.end(); ++j)
            {
                j->context.modify(*(*i)->ctxchange(),
                    ((*i)->ctxchange()->volume_scope == ContextChanging::SCORE) ||
                    ((*i)->ctxchange()->volume_scope == ContextChanging::GROUP      && pick.get_score().same_group(j->begin.staff(), cursor.staff())) ||
                    ((*i)->ctxchange()->volume_scope == ContextChanging::INSTRUMENT && pick.get_score().same_instrument(j->begin.staff(), cursor.staff())) ||
                    ((*i)->ctxchange()->volume_scope == ContextChanging::STAFF      && &j->begin.staff() == &cursor.staff()) ||
                    ((*i)->ctxchange()->volume_scope == ContextChanging::VOICE      && j == pvoice)
                );
            };
            
            pline->context.modify(*(*i)->ctxchange());  // tempo changing
            // Annotation: attachables will be engraved for the whole line after justification
        };
    };
    
    // engrave empty voice (ignored by pick!)
    if (cursor->is(Class::NOTEOBJECT))
    {
        const RefPtr<SubVoice>& subvoice = static_cast<const NoteObject&>(*cursor).subvoice;
        if (subvoice && subvoice->notes.empty())
        {
            // insert new voice
            Plate::VoiceIt parent(pvoice);
            if (!subvoice->on_top) ++pvoice;
            pvoice = pline->voices.insert(pvoice, Plate::pVoice(const_Cursor(cursor.staff(), *subvoice)));
            pvoice->context = parent->context;
            
            // add position information to the new voice
            pvoice->basePos = parent->basePos;
            pvoice->time = cursor.time;
            pvoice->end_time = cursor.time;
            pvoice->head_height = _round(viewport->umtopx_v(HEAD_HEIGHT(cursor.staff())));
            
            // add end-of-voice indicator object
            pvoice->notes.push_back(Plate::pNote(pnote->absolutePos.front(), pvoice->begin));
            pvoice->notes.back().gphBox.pos = pvoice->notes.back().absolutePos.front();
            pvoice->notes.back().gphBox.width = 1000;
            pvoice->notes.back().gphBox.height = _round(viewport->umtopx_v( (cursor.staff().line_count - 1)
                                                                           * HEAD_HEIGHT(cursor.staff())));
            
            // update external data
            if (reengrave_info)
            {
                pnote = --pvoice->notes.end();              // set pnote to end-of-voice indicator
                reengrave_info->update(*subvoice, *this);   // update external data
                pvoice = parent;                            // reset on-plate voice
                pnote = --pvoice->notes.end();              // reset on-plate note-object
            };
        };
    };
    
    // remember engraved symbol
    if (cursor->is(Class::BARLINE))     // barlines will be remembered by all voices
    {
        for (std::list<Plate::pVoice>::iterator v = pline->voices.begin(); v != pline->voices.end(); ++v)
        {
            v->context.set_buffer(&pnote->get_note());
            v->context.set_buffer_xpos(pnote->gphBox.right());
        };
    }
    else
    {
        pvoice->context.set_buffer(&pnote->get_note());
        pvoice->context.set_buffer_xpos(pnote->gphBox.right());
    };
    
    // update external data
    if (reengrave_info)
    {
        reengrave_info->update(cursor.original(), *this);
        if (reengrave_info->is_empty())
            reengrave_info = NULL;
    };
    
    // insert barline
    if (param->auto_barlines && pvoice->context.beat(cursor.ntime) == 0L && barcnt < pvoice->context.bar(cursor.ntime))
    {
        barcnt = pvoice->context.bar(cursor.ntime); // set bar counter
        pick.insert_barline(Barline::singlebar);    // insert barline to be engraved
    }
    
    // check for end of voice
    else if (   !cursor.has_next()          && cursor.remaining_duration <= 0L
             && !cursor->is(Class::NEWLINE) && pick.eov())
    {
        // check dangling ties
        if (!tieinfo[&cursor.voice()].empty())
            log_warn("Got tie exceeding the end of the voice. (class: EngraverState)");
        BeamInfoMap::iterator i = beaminfo.find(&*pvoice);
        if (i != beaminfo.end()) i->second.finish();
        
        // add end-of-voice indicator object
        pvoice->notes.push_back(Plate::pNote(pos, cursor));
        pvoice->notes.back().note.to_end();
        if (cursor->is(Class::BARLINE))
            pvoice->notes.back().gphBox.pos.x = pnote->gphBox.pos.x + 1;
        else
            pvoice->notes.back().gphBox.pos.x = pnote->gphBox.right() + 2 * viewport->umtopx_h(param->min_distance);
        pvoice->notes.back().gphBox.pos.y = pvoice->basePos.y;
        pvoice->notes.back().gphBox.width = 1000;
        pvoice->notes.back().gphBox.height = pvoice->head_height * (cursor.staff().line_count - 1);
        pvoice->notes.back().absolutePos.front() = pvoice->notes.back().gphBox.pos;
    };
}

// calculate line-end information/line's rightmost border (see "Plate::pLine::line_end")
void EngraverState::create_lineend()
{
    // iterate through the voices
    for (std::list<Plate::pVoice>::iterator voice = pline->voices.begin(); voice != pline->voices.end(); ++voice)
    {
        if (voice->notes.empty()) continue; // check if voice contains notes
        
        // set furthest voice-end as staff/line-end
        if (pline->line_end < voice->notes.back().gphBox.right())
            pline->line_end = voice->notes.back().gphBox.right();
    };
    
    // break unfinished ties
    for (std::list<Plate::pVoice>::iterator voice = pline->voices.begin(); voice != pline->voices.end(); ++voice)
    {
        break_ties(tieinfo[&voice->begin.voice()],
                   pline->line_end,
                   pick.eos() ? 0 : pick.get_indent(),
                   voice->head_height);
    };
}

// apply all non-accumulative offsets
void EngraverState::apply_offsets()
{
    // iterate the voices
    for (std::list<Plate::pVoice>::iterator voice = pline->voices.begin(); voice != pline->voices.end(); ++voice)
    {
        // iterate the voice
        for (std::list<Plate::pNote>::iterator it = voice->notes.begin(); it != voice->notes.end(); ++it)
        {
            if (it->at_end() || it->is_inserted() || !it->get_note().is(Class::VISIBLEOBJECT)) continue;
            
            if (it->get_note().get_visible().offset_x)
            {
                const mpx_t head_width = (sprites->head_width(it->sprite) * voice->head_height) / sprites->head_height(it->sprite);
                it->add_offset((it->get_note().get_visible().offset_x * head_width) / 1000);
            };
        };
    };
}

// engrave all stems within beams in the current line
void EngraverState::engrave_stems()
{
    bool got_beam = false;  // indicating notes with beam
    mpx_t  x1(0), y1(0);    // beam coordinates
    double slope(0.0);      // beam slope
    double top(0.0);        // new stem top position
    
    // iterate the voices
    for (pvoice = pline->voices.begin(); pvoice != pline->voices.end(); ++pvoice)
    {
        // set style
        style = &*pvoice->begin.staff().style;
        if (!style) style = &default_style;
        
        // iterate the voice
        std::list<Plate::pNote>::iterator beam_it;      // beam group iterator
        
        // correct stem length for notes with beam (I)
        got_beam = false;
        for (pnote = pvoice->notes.begin(); pnote != pvoice->notes.end(); ++pnote)
        {
            pnote->beam_begin = pvoice->notes.end();    // reset beam_begin
            
            if (got_beam)   // while on beamed note
            {
                // set begin iterator
                pnote->beam_begin = beam_it;
                
                // on the end of the beam
                if (beam_it->beam[VALUE_BASE - 3]->end == &*pnote)
                {
                    got_beam = false;   // reset beam indicator
                }
                else if (pnote->stem_info)  // correct stems within the beam
                {
                    top = y1 + (pnote->stem.x - x1) * slope;
                    if ((pnote->stem.base > top) != (pnote->stem.base > pnote->stem.top))
                    {
                        // correct stem position
                        const SpriteInfo& headsprite = (*sprites)[pnote->sprite];       // get sprite-info
                        const double scale = pvoice->head_height / (sprites->head_height(pnote->sprite) * 1000.0);
                        const unsigned int app_scale = pnote->get_note().get_visible().appearance.scale;
                        const mpx_t stem_width = viewport->umtopx_h(style->stem_width); // get stem width
                        const mpx_t sprite_width = _round(headsprite.width * scale * app_scale);
                        Position<mpx_t> stem;                                           // stem position
                        stem.x = _round(headsprite.get_real("stem.x") * scale * app_scale);
                        stem.y = _round(headsprite.get_real("stem.y") * scale * app_scale);
                        
                        if (pnote->stem.base > top)     // an upward stem is right of the chord
                        {
                            pnote->stem.x = pnote->absolutePos.front().x                            // base position
                                + stem.x                                                            // offset for the sprite
                                - _round(pnote->stem_info->cluster ? 0 : stem_width * scale / 2);   // offset for the stem's width
                            
                            pnote->stem.top = _round(y1 + (pnote->stem.x - x1) * slope);
                            pnote->stem.base = pnote->stem_info->base_pos + (pnote->stem_info->base_side ?
                                _round(((headsprite.height * scale * app_scale - stem.y) * pnote->stem_info->base_scale) / 1000.0) :
                                stem.y);
                        };
                        
                        if (pnote->stem.base < top)     // downward stems are placed left of the chord
                        {
                            pnote->stem.x = pnote->absolutePos.front().x                            // base position
                             + _round(pnote->stem_info->cluster ? stem.x : sprite_width - stem.x + stem_width * scale / 2);
                            
                            pnote->stem.top = _round(y1 + (pnote->stem.x - x1) * slope);
                            pnote->stem.base = pnote->stem_info->top_pos + (pnote->stem_info->top_side ?
                                stem.y :
                                _round(((headsprite.height * scale * app_scale - stem.y) * pnote->stem_info->top_scale) / 1000.0));
                        };
                        
                        pnote->gphBox.extend(Position<mpx_t>(pnote->stem.x, pnote->stem.top));
                    }
                    else pnote->stem.top = _round(top);
                };
            };
            
            if (!got_beam && pnote->beam[VALUE_BASE - 3]) // on beam's begin
            {
                pnote->beam_begin = pnote;  // set on-plate begin iterator
                beam_it = pnote;            // set begin iterator
                got_beam = true;            // set beam indicator
                x1 = pnote->stem.x;         // set coordinates
                y1 = pnote->stem.top;       // set slope
                slope  = pnote->beam[VALUE_BASE - 3]->end->stem.top - y1;
                slope /= pnote->beam[VALUE_BASE - 3]->end->stem.x - x1;
            };
        };
        
        // engrave beams
        BeamInfo beam_info(*pvoice);
        value_t time(pvoice->time);
        beam_it = pvoice->notes.begin();
        for (pnote = pvoice->notes.begin(); pnote != pvoice->notes.end(); ++pnote)
        {
            if (!pnote->at_end() && pnote->get_note().is(Class::NOTEOBJECT))
            {
                time += static_cast<const NoteObject&>(pnote->get_note()).value();
                if (pnote->get_note().is(Class::CHORD))
                {
                    beam_info.apply2(static_cast<const Chord&>(pnote->get_note()), pnote, time, param->beam_group);
                };
            };
        };
        beam_info.finish();
        
        // correct stem length for notes with beam (II)
        got_beam = false;
        for (pnote = pvoice->notes.begin(); pnote != pvoice->notes.end(); ++pnote)
        {
            // on beam's begin
            if (!got_beam && pnote->beam[VALUE_BASE - 3])
            {
                beam_it = pnote;            // set begin iterator
                got_beam = true;            // set beam indicator
            }
            
            // while on beamed note
            else if (got_beam)
            {
                // update stem lengths by beam-offset
                if (pnote->stem.base < pnote->stem.top)
                    pnote->stem.top = _round(pnote->stem.top + pvoice->head_height *
                        (((beam_it->stem.base > beam_it->stem.top) ? style->beam_height : 0)
                           + pnote->stem.beam_off * (style->beam_height + style->beam_distance))
                        / 1000.0);
                else
                    pnote->stem.top = _round(pnote->stem.top - pvoice->head_height *
                        (((beam_it->stem.base < beam_it->stem.top) ? style->beam_height : 0)
                           + pnote->stem.beam_off * (style->beam_height + style->beam_distance))
                        / 1000.0);
                
                got_beam = (beam_it->beam[VALUE_BASE - 3]->end != &*pnote); // update beam indicator
            };
        };
    };
}

// engrave all attachables within a given line
void EngraverState::engrave_attachables()
{
    std::list<DurableInfo> durableinfo; // information for all durable objects
    const VisibleObject* visible;       // visible object reference
    
    // iterate the voices
    for (pvoice = pline->voices.begin(); pvoice != pline->voices.end(); ++pvoice)
    {
        // set style
        style = &*pvoice->begin.staff().style;
        if (!style) style = &default_style;
        
        // iterate the voice
        for (pnote = pvoice->notes.begin(); pnote != pvoice->notes.end(); ++pnote)
        {
            // ignore inserted notes
            if (pnote->at_end() || pnote->is_inserted()) continue;
            
            // check durables
            for (std::list<DurableInfo>::iterator i = durableinfo.begin(); i != durableinfo.end();)
            {
                if ((--(i->durationcountdown)) == 0)    // if the durable objects ends here
                {
                    // engrave object (polymorphically call "Durable::engrave")
                    i->source->engrave(*this, *i);
                    
                    // update external data
                    if (reengrave_info)
                    {
                        const Plate::NoteIt temp(pnote);
                        pnote = i->pnote;
                        reengrave_info->update(*i->source, *this);
                        if (reengrave_info->is_empty())
                            reengrave_info = NULL;
                        pnote = temp;
                    };
                    
                    // remove the object
                    i = durableinfo.erase(i);
                }
                else ++i;
            };
            
            // get visible object
            visible = &pnote->get_note().get_visible();
            
            // engrave movables
            if (visible != NULL)
            for (MovableList::const_iterator i = visible->attached.begin(); i != visible->attached.end(); ++i)
            {
                // append durable
                if ((*i)->is(Class::DURABLE))
                {
                    // create object (polymorphically call "Durable::engrave")
                    (*i)->engrave(*this);
                    
                    // initialize durable-information to wait for the end-position
                    durableinfo.push_back(DurableInfo());
                    durableinfo.back().source = &static_cast<const Durable&>(**i);
                    durableinfo.back().target = &static_cast<Plate::pDurable&>(*pnote->attached.back());
                    durableinfo.back().pnote  = pnote;
                    durableinfo.back().durationcountdown = static_cast<const Durable&>(**i).duration - 1;
                }
                
                // append attachable
                else
                {
                    // engrave object (polymorphically call "Movable::engrave")
                    (*i)->engrave(*this);
                    
                    // update external data
                    if (reengrave_info)
                    {
                        reengrave_info->update(**i, *this);
                        if (reengrave_info->is_empty())
                            reengrave_info = NULL;
                    };
                };
            };
        };
        
        if (!durableinfo.empty())   // warn for durable objects exceeding the linebreak
        {
            log_warn("Got durable object exceeding the end of the line or voice. (class: EngraverState)");
            
            // try to finish the objects
            pnote = --pvoice->notes.end();
            for (std::list<DurableInfo>::iterator i = durableinfo.begin(); i != durableinfo.end(); ++i)
            {
                // engrave object (polymorphically call "Durable::engrave")
                i->source->engrave(*this, *i);
                
                // update external data
                if (reengrave_info)
                {
                    const Plate::NoteIt temp(pnote);
                    pnote = i->pnote;
                    reengrave_info->update(*i->source, *this);
                    if (reengrave_info->is_empty())
                        reengrave_info = NULL;
                    pnote = temp;
                };
            };
            durableinfo.clear();    // erase durable information
        };
    };
}

// brace scale formula (substitute h -> sh; w -> sw)
inline
mpx_t curlybrace_width(const double s,      // scale [mpx/svg]
                       const mpx_t  h,      // height [mpx]
                       const int    w_0,    // sprite width [svg]
                       const double h_min,  // scale range [svg]
                       const double h_max,
                       const double m_lo,   // coefficients [1]
                       const double m_hi)
{
    // h \in [0, h_min[
    if (h < s * h_min)
    {
        // w(h) = w_0 - m_lo * (w_0 - h * (w_0 / h_lo))
        return _round(s * (1.0 - m_lo) * w_0 + (h * m_lo * w_0) / h_min);
    }
    // h \in ]h_max, \infty[
    else if (h > s * h_max)
    {
        // w(h) = w_0 + m_hi * (h - h_max)
        return _round(s * (w_0 - m_hi * h_max) + m_hi * h);
    }
    // h \in [h_min, h_max]
    else
    {
        // W(H) = w_0
        return _round(s * w_0);
    };
}

// engrave braces and brackets for the current line
void EngraverState::engrave_braces()
{
    // local iterators
    std::list<Plate::pVoice>::iterator curlybrace_begin = pline->voices.end(); // curly brace begin
    std::list<Plate::pVoice>::iterator curlybrace_end   = pline->voices.end(); // curly brace end
    std::list<Plate::pVoice>::iterator bracket_begin    = pline->voices.end(); // bracket begin
    std::list<Plate::pVoice>::iterator bracket_end      = pline->voices.end(); // bracket end
    std::list<Plate::pVoice>::iterator tvoice;                                 // temporary
    
    // sprites
    SpriteId brace_sprite =
                (pick.get_score().layout.brace_sprite.setid == UNDEFINED
                 || (*sprites)[pick.get_score().layout.brace_sprite.setid].brace == UNDEFINED) ?
                    SpriteId(0, (*sprites)[0].brace) :
                    ((pick.get_score().layout.brace_sprite.spriteid == UNDEFINED) ?
                        SpriteId(pick.get_score().layout.brace_sprite.setid, (*sprites)[pick.get_score().layout.brace_sprite.setid].brace) :
                        pick.get_score().layout.brace_sprite);
    
    SpriteId bracket_sprite =
                (pick.get_score().layout.bracket_sprite.setid == UNDEFINED
                 || (*sprites)[pick.get_score().layout.bracket_sprite.setid].bracket == UNDEFINED) ?
                    SpriteId(0, (*sprites)[0].bracket) :
                    ((pick.get_score().layout.bracket_sprite.spriteid == UNDEFINED) ?
                        SpriteId(pick.get_score().layout.bracket_sprite.setid, (*sprites)[pick.get_score().layout.bracket_sprite.setid].bracket) :
                        pick.get_score().layout.bracket_sprite);
    
    // minimal position (to calculate offset for the line)
    mpx_t offset = pline->basePos.x;
    
    // iterate the voices
    for (std::list<Staff>::const_iterator staff = pick.get_score().staves.begin(); staff != pick.get_score().staves.end(); ++staff)
    {
        // calculate scales
        double brace_scale = viewport->umtopx_v(HEAD_HEIGHT(*staff));
        double bracket_scale(brace_scale); 
               brace_scale /= sprites->head_height(brace_sprite);
               bracket_scale /= sprites->head_height(bracket_sprite);
        
        // check for curly brace
        if (staff->curlybrace)
        {
            tvoice = pline->get_staff(*staff);  // get the staff on the plate
            if (tvoice != pline->voices.end())  // if the staff exists (i.e. is visible)
            {
                curlybrace_end = tvoice;                        // remember staff
                if (curlybrace_begin == pline->voices.end())    // if no brace has begun yet
                    curlybrace_begin = tvoice;                  //    let one begin here
            };
        }
        else if (curlybrace_begin != pline->voices.end())   // if there is no curly brace
        {                                                   // but one did begin
            tvoice = pline->get_staff(*staff);              //      get the staff on the plate
            if (tvoice != pline->voices.end())              //      if the staff exists (i.e. is visible)
                curlybrace_end = tvoice;                    //          end here
            
            curlybrace_begin->brace.sprite = brace_sprite;  //    set brace sprite
            curlybrace_begin->brace.gphBox.height =
                ( curlybrace_end->basePos.y
                + viewport->umtopx_v(HEAD_HEIGHT(curlybrace_end->begin.staff()) * (curlybrace_end->begin.staff().line_count - 1))
                - curlybrace_begin->basePos.y);
            curlybrace_begin->brace.gphBox.width =
                curlybrace_width(brace_scale,
                                 curlybrace_begin->brace.gphBox.height,
                                 (*sprites)[brace_sprite].width,
                                 (*sprites)[brace_sprite].get_real("hmin"), (*sprites)[brace_sprite].get_real("hmax"),
                                 (*sprites)[brace_sprite].get_real("low"),  (*sprites)[brace_sprite].get_real("high"));
                                 
            curlybrace_begin->brace.gphBox.pos = curlybrace_begin->basePos;
            curlybrace_begin->brace.gphBox.pos.x -= curlybrace_begin->brace.gphBox.width + viewport->umtopx_h(staff->brace_pos);
            if (curlybrace_begin->brace.gphBox.pos.x < offset)  // update minimal x position
                offset = curlybrace_begin->brace.gphBox.pos.x;
            curlybrace_begin = pline->voices.end();             // end brace (reset)
        };
        
        // check for bracket
        if (staff->bracket)
        {
            tvoice = pline->get_staff(*staff);  // get the staff on the plate
            if (tvoice != pline->voices.end())  // if the staff exists (i.e. is visible)
            {
                bracket_end = tvoice;                       // remember staff
                if (bracket_begin == pline->voices.end())   // if no bracket has begun yet
                    bracket_begin = tvoice;                 //    let one begin here
            };
        }
        else if (bracket_begin != pline->voices.end())      // if there is no bracket
        {                                                   // but one did begin
            tvoice = pline->get_staff(*staff);              //      get the staff on the plate
            if (tvoice != pline->voices.end())              //      if the staff exists (i.e. is visible)
                bracket_end = tvoice;                       //          end here
            
            bracket_begin->bracket.sprite = bracket_sprite;   //    set bracket sprite
            bracket_begin->bracket.line_end = bracket_begin->basePos;
            bracket_begin->bracket.line_end.x -= viewport->umtopx_h(staff->bracket_pos);
            bracket_begin->bracket.line_base = bracket_begin->bracket.line_end;
            bracket_begin->bracket.line_base.x -= _round(bracket_scale * (*sprites)[bracket_sprite].get_real("line"));
            bracket_begin->bracket.line_end.y =
                 (bracket_end->basePos.y
                + viewport->umtopx_v(HEAD_HEIGHT(bracket_end->begin.staff()) * (bracket_end->begin.staff().line_count - 1))
                - bracket_begin->basePos.y);
            bracket_begin->bracket.gphBox.pos = bracket_begin->bracket.line_base;
            bracket_begin->bracket.gphBox.height = _round(bracket_scale * (*sprites)[bracket_sprite].height);
            bracket_begin->bracket.gphBox.pos.y -= bracket_begin->bracket.gphBox.height;
            bracket_begin->bracket.gphBox.height *= 2;
            bracket_begin->bracket.gphBox.height += bracket_begin->bracket.line_end.y;
            bracket_begin->bracket.gphBox.width = _round(bracket_scale * (*sprites)[bracket_sprite].width);
            bracket_begin->bracket.line_base.y -= 500;
            bracket_begin->bracket.line_end.y += bracket_begin->basePos.y;
            if (bracket_begin->bracket.gphBox.pos.x < offset)  // update minimal x position
                offset = bracket_begin->bracket.gphBox.pos.x;
            bracket_begin = pline->voices.end();                // end bracket (reset)
        };
    };
    
    // offset the line
    if (offset < pline->basePos.x)
    {
        offset = pline->basePos.x - offset;
        pline->gphBox.width += offset;
        pline->basePos.x += offset;
        pline->line_end += offset;
        
        for (pvoice = pline->voices.begin(); pvoice != pline->voices.end(); ++pvoice)
        {
            pvoice->basePos.x += offset;
            pvoice->brace.gphBox.pos.x += offset;
            pvoice->bracket.gphBox.pos.x += offset;
            pvoice->bracket.line_base.x += offset;
            pvoice->bracket.line_end.x += offset;
            for (pnote = pvoice->notes.begin(); pnote != pvoice->notes.end(); ++pnote)
            {
                pnote->add_offset(offset);
                pnote->add_tieend_offset(offset);
            };
        };
    };
}

// justification information
struct SCOREPRESS_LOCAL DistanceData
{
    mutable mpx_t begin;    // empty space in front of the note
            mpx_t pos;      // position of the note
    mutable mpx_t end;      // end of the graphical objec (start of the next empty space)
    mutable mpx_t dist;     // distance available for scaling
    
    mutable std::string tag;
    
    Plate::pVoice* const src;
    
    DistanceData(Plate::pVoice* v, mpx_t p) : pos(p), src(v) {};
    inline bool operator < (const DistanceData& data) const {return pos < data.pos;};
};

// graphical mask (union over the line)
struct SCOREPRESS_LOCAL GraphicData
{
            mpx_t pos;  // gphBox.pos.x
    mutable mpx_t end;  // gphBox.right
    
    GraphicData(mpx_t p, mpx_t e) : pos(p), end(e) {};
    inline bool operator < (const GraphicData& data) const {return pos < data.pos;};
};

// justify the given line to fit into the score-area
void EngraverState::justify_line()
{
    // initialization and check
    mpx_t width = pline->line_end - pline->basePos.x;               // current line width
    mpx_t diff  = viewport->umtopx_h(lineinfo.dimension->width)     // space to be added
                - pline->line_end - lineinfo.right_margin;
    
    if ((1000 * diff) / width > mpx_t(param->max_justification) - 1000) return;
    
    // prepare justification data
    std::multiset<DistanceData>     dists;          // interleaved distance information
    std::multiset<GraphicData>      gphs;           // merged graphical information
    std::map<Plate::pVoice*, mpx_t> dists_sum;      // sum of available distance
    mpx_t                           dist_sum = 0;   // total of available distances
    
    // calculate distances for each voice
    {
    std::multiset<DistanceData>::iterator tgt;
    for (Plate::pLine::Iterator voice = pline->voices.begin(); voice != pline->voices.end(); ++voice)
    {
        // setup sum
        dists_sum[&*voice] = 0;
        
        // iterate the voice
        mpx_t npos = pline->basePos.x + viewport->umtopx_h(param->min_distance);
        for (Plate::pVoice::const_Iterator it = voice->notes.begin(); it != voice->notes.end(); ++it)
        {
            tgt = dists.insert(DistanceData(&*voice, it->gphBox.pos.x));
            tgt->begin = npos;
            tgt->end   = npos = (lineinfo.forced_justification)
                                ? it->gphBox.right()
                                : it->gphBox.right() + viewport->umtopx_h(param->min_distance);
            tgt->dist  = tgt->pos - tgt->begin;
            tgt->tag   = (!it->at_end()) ? classname(it->get_note().classtype()) : "EOV";
            gphs.insert(GraphicData(it->gphBox.pos.x, npos));
        };
    };
    }
    
    // merge graphic data
    {
    mpx_t npos = 0;
    for (std::multiset<GraphicData>::iterator it = gphs.begin(); it != gphs.end(); ++it)
    {
        if (it->pos < npos)
        {
            if (npos < it->end)
                ((--it)++)->end = npos = it->end;
            gphs.erase(it--);
        }
        else npos = it->end;
    };
    }
    
    // remove graphical data from available spaces ("punch holes")
    {
    std::multiset<GraphicData>::iterator git = gphs.begin();
    for (std::multiset<DistanceData>::iterator it = dists.begin(); it != dists.end(); ++it)
    {
        if (git == gphs.end()) --git;
        while (git->end > it->begin && git != gphs.begin()) --git;
        for (; git != gphs.end(); ++git)
        {
            if (git->end <= it->begin) ;        // case:   ,,|           | |
            else if (git->pos <= it->begin)
            {
                if (git->end <= it->pos)        // case:    ,|,          | |
                    it->dist -= (git->end - it->begin);
                else                            // case:    ,|           |,|
                    it->dist -= (it->end - it->begin);
            }
            else if (git->end <= it->pos)       // case:     |    , ,    | |
                it->dist -= (git->end - git->pos);
            else if (git->pos <= it->pos)       // case:     |          ,|,|
                it->dist -= (it->pos - git->pos);
            else                                // case:     |           | |,,
                break;
        };
        if (it->dist < 0) it->dist = 0;
        dists_sum[&*it->src] += it->dist;
    };
    }
    
    // check distance sum
    for (std::map<Plate::pVoice*, mpx_t>::const_iterator i = dists_sum.begin(); i != dists_sum.end(); ++i)
        if (i->second > dist_sum)
            dist_sum = i->second;
    
    // execute justification
    if (diff < -dist_sum) diff = -dist_sum; // do not force overlapping
    pline->line_end += diff;                // move line-end
    
    // iterate the voices
    mpx_t distance;     // current distance
    mpx_t prev_offset;  // previous offset
    mpx_t offset;       // current offset
    bool  pre_tie;      // indicating the second part of a broken tie
    bool  got_tie;      // indicating the first  part of a broken tie
    for (Plate::pLine::Iterator voice = pline->voices.begin(); voice != pline->voices.end(); ++voice)
    {
        // initialize
        distance = 0;
        offset   = 0;
        pre_tie  = false;
        got_tie  = false;
        
        // iterate the voice
        Plate::pVoice::Iterator               note_it = voice->notes.begin();
        std::multiset<DistanceData>::iterator data_it = dists.begin();
        for (; note_it != voice->notes.end(); ++note_it, ++data_it)
        {
            while (data_it->src != &*voice) ++data_it;              // move to correct data element
            prev_offset = offset;                                   // save previous offset
            distance += data_it->dist;
            offset = _round((double(diff) * distance) / dist_sum);  // calculate offset
            
            // check for broken tie at the end
            if (!pre_tie && got_tie)
                ((--note_it)++)->add_tieend_offset(offset);
            
            // check for broken tie (second node will not be moved)
            pre_tie = (!note_it->ties.empty() && (note_it->ties.front().pos1.x > note_it->ties.front().pos2.x));
            got_tie = !note_it->ties.empty();
            
            // add offset
            note_it->add_offset(offset);
            if (pre_tie) note_it->add_tieend_offset(prev_offset);
            
            /*
              Algorithm Overview:
                update data pointer
                update offset (needs: cur data)
                offset prev tieend (needs: prev pretie/hastie, cur offset)
                update pretie
                update hastie
                offset cur note (needs: cur offset)
                offset cur tieend (needs: prev offset, cur pretie)
            */
        };
    };
}

// transform a score-dimension from micrometer to millipixel
Pageset::ScoreDimension EngraverState::dimtopx(const ScoreDimension& dim) const
{
    return Pageset::ScoreDimension(viewport->umtopx_h(dim.position.x), viewport->umtopx_v(dim.position.y),
                                   viewport->umtopx_h(dim.width), viewport->umtopx_v(dim.height));
}

// break all ties at the specified x-position (prepare restart at given x-position)
void EngraverState::break_ties(      TieInfoChord&  tieinfo,        // tie information structure
                               const mpx_t          endpos,         // x-position to end the tie on
                               const mpx_t          restartpos,     // x-position to restart the tie
                               const mpx_t          head_height)    // the staff's head-height
{
    // iterate through all ties
    for (TieInfoChord::iterator i = tieinfo.begin(); i != tieinfo.end(); ++i)
    {
        i->second.refPos = restartpos;      // set non-head anchor
        if (!i->second.target) continue;    // check, if we did break the tie before
        
        // if not, calculate positions for the first part of the tie
        i->second.target->pos2.x = endpos - (i->second.source->offset1.x * head_height) / 1000;
        i->second.target->pos2.y = i->second.target->pos1.y;
        i->second.target->control2.x = i->second.target->pos2.x - (i->second.source->control1.x * head_height) / 1000;
        i->second.target->control2.y = i->second.target->pos2.y + (i->second.source->control1.y * head_height) / 1000;
        i->second.target = NULL;    // indicate broken tie
    };
}

// constructor (will erase "score" from the "pageset" and prepare for engraving)
EngraverState::EngraverState(const Score&         _score,
                             const unsigned int   _start_page,
                                   Pageset&       _pageset,
                             const Sprites&       _sprites,
                             const unsigned int   _head_height,
                             const EngraverParam& _param,
                             const StyleParam&    _style,
                             const ViewportParam& _viewport) : sprites(&_sprites),
                                                               def_head_height(_score.head_height ? _score.head_height : _head_height),
                                                               param((!!_score.param) ? &*_score.param : &_param),
                                                               style(&_style),
                                                               default_style(_style),
                                                               viewport(&_viewport),
                                                               reengrave_info(NULL),
                                                               pick(_score, (!!_score.param) ? *_score.param : _param, _viewport, _sprites, def_head_height),
                                                               pageset(&_pageset),
                                                               pagecnt(0),
                                                               barcnt(0),
                                                               start_time(0),
                                                               end_time(0)
{
    // prepare the pageset
    pageset->erase(_score);         // erase the plates
    if (pick.eos()) return;         // ignore empty score
    
    // initialize local variables
    page = pageset->get_page(_start_page);
    page->plates.push_back(Pageset::PlateInfo(pagecnt, _start_page, _score, dimtopx(pick.get_dimension())));
    plateinfo = --page->plates.end();
    plate = plateinfo->plate;
    lineinfo.dimension = &pick.get_dimension();
    lineinfo.indent = pick.get_indent();
    lineinfo.justify = pick.get_justify();
    lineinfo.forced_justification = pick.get_forced_justification();
    lineinfo.right_margin = pick.get_right_margin();
    start_time = pick.get_cursor().time;
    
    // prepare the plate
    plate->lines.push_back(Plate::pLine());     // push first line
    pline = plate->lines.begin();               // initialize target line iterator
    pline->basePos.x = pick.get_indent();       // set the base-position of the new line (checked with new voice)
    pline->basePos.y = pick.get_cursor().ypos + viewport->umtopx_v(pick.staff_offset(_score.staves.front()));
    
    // create default clef
    Clef clef;
    const std::map<std::string, size_t>::const_iterator sprite = (*sprites)[0].ids.find("clef.treble");
    clef.sprite = SpriteId(0, sprite->second);
    clef.base_note = static_cast<tone_t>((*sprites)[0][sprite->second].get_integer("basenote"));
    clef.line = static_cast<unsigned char>((*sprites)[0][sprite->second].get_integer("line"));
    clef.keybnd_sharp = static_cast<tone_t>((*sprites)[0][sprite->second].get_integer("keybound.sharp"));
    clef.keybnd_flat = static_cast<tone_t>((*sprites)[0][sprite->second].get_integer("keybound.flat"));
    pline->staffctx[&get_staff()].modify(clef);
}

// get the staff, in which the note is drawn (i.e. apply staff-shift)
const Staff& EngraverState::get_visual_staff() const
{
    if (!pick.get_cursor()->is(Class::NOTEOBJECT)) return pick.get_cursor().staff();
    return pick.get_staff(static_cast<const NoteObject&>(*pick.get_cursor()).staff_shift);
}

// engrave currently referenced object and prepare the next object
bool EngraverState::engrave_next()
{
    if (pick.eos()) return false;
    
    // engrave current object
    engrave();
    
    // calculate data for the next note
    pick.next(pnote->gphBox.width);
    
    // quit here, if there's no newline (and no end of score)
    if (!pick.eos() && !pick.get_cursor()->is(Class::NEWLINE)) return true;
    
    const bool newline   = !pick.eos() && pick.get_cursor()->is(Class::NEWLINE);
    const bool pagebreak = !pick.eos() && pick.get_cursor()->is(Class::PAGEBREAK);
    
    // engrave all newlines at once
    while (!pick.eos())
    {
        // insert automatic clef, key and time signature
        if (pick.get_cursor().is_main())
        {
            Pick::VoiceCursor vcur(pick.get_cursor());
            const Newline& layout = static_cast<const Newline&>(*vcur);
            const Plate::pLine::StaffContextMap::const_iterator ctx = pline->staffctx.find(&vcur.staff());
            if (vcur.has_next())                            ++vcur;
            if (!vcur.at_end() && vcur->is(Class::CLEF))    ++vcur;
            else if (   layout.auto_clef
                     && ctx != pline->staffctx.end())       pick.insert(ctx->second.last_clef());
            if (!vcur.at_end() && vcur->is(Class::KEY))     ++vcur;
            else if (   layout.auto_key
                     && ctx != pline->staffctx.end())       pick.insert(ctx->second.last_key());
            if (!vcur.at_end() && vcur->is(Class::TIMESIG)) ++vcur;
            else if (   layout.auto_timesig)                pick.insert(pvoice->context.last_timesig());
        };
        
        engrave();                      // engrave newline
        pick.next(pnote->gphBox.width); // goto next note
        
        if (!pick.is_within_newline()) break;
    };
    
    // finish the line
    create_lineend();               // calculate line-end information
    engrave_braces();               // engrave braces and brackets
    if (lineinfo.justify)           // justifiy the line
        justify_line();
    apply_offsets();                // apply non-cumulative offsets
    engrave_stems();                // engrave all the missing stems
    engrave_attachables();          // engrave all the line's attached objects
    pline->calculate_gphBox();      // calculate the graphical boundary box
    
    // exit here, if no newline (below is the code for newline handling)
    if (!newline) return false;
    
    // check for pagebreak
    if (pagebreak)
    {
        if (++page == pageset->pages.end())
            page = pageset->add_page();
        page->plates.push_back(Pageset::PlateInfo(++pagecnt, plateinfo->start_page, pick.get_score(), dimtopx(pick.get_dimension())));
        plateinfo = --page->plates.end();
        plate = page->plates.back().plate;
    };
    
    // add new line to the plate
    plate->lines.push_back(Plate::pLine());             // append new line to the plate
    plate->lines.back().basePos.x = pick.get_indent();  // calculate the base-position
    plate->lines.back().basePos.y = pick.get_cursor().ypos + viewport->umtopx_v(pick.staff_offset(get_score().staves.front()));
    plate->lines.back().gphBox.pos = plate->lines.back().basePos;   // initialize graphical boundary box
    plate->lines.back().context = pline->context;       // get previous context
    plate->lines.back().staffctx = pline->staffctx;     // and staff-ctxs
    
    // update line start-time
    pline->end_time = start_time = pick.get_cursor().time;
    
    // update line style information
    lineinfo.dimension = &pick.get_dimension();
    lineinfo.indent = pick.get_indent();
    lineinfo.justify = pick.get_justify();
    lineinfo.forced_justification = pick.get_forced_justification();
    lineinfo.right_margin = pick.get_right_margin();
    
    // add voices
    const Pick::VoiceCursor* cur;
    Position<mpx_t>          pos;
    for (Plate::VoiceList::iterator v = pline->voices.begin(); v != pline->voices.end(); ++v)
    {
        if (v->notes.back().at_end()) continue;
        if (!v->notes.back().get_note().is(Class::NEWLINE))
            log_warn("Found unfinished voice not ending with a Newline. (class: EngraverState)");
        
        cur = pick.peek(v->begin.voice());
        if (!cur)
        {
            // add the voice (with "begin" at end)
            plate->lines.back().voices.push_back(Plate::pVoice(v->begin));
            pvoice = --plate->lines.back().voices.end();
            pvoice->begin.to_end();
            pvoice->context = v->context;
            
            // calculate the voice position
            pvoice->basePos.x = pick.get_indent();
            pvoice->basePos.y = pick.get_cursor().ypos + viewport->umtopx_v(pick.staff_offset(v->begin.staff()));
            pvoice->time = start_time;
            pvoice->head_height = _round(viewport->umtopx_v(HEAD_HEIGHT(v->begin.staff())));

            // calculate the EOV indicator position
            pos = pvoice->basePos;
            pos.x += viewport->umtopx_h(param->min_distance);
            
            // add end-of-voice indicator object
            pvoice->notes.push_back(Plate::pNote(pos, const_Cursor(v->begin)));
            pvoice->notes.back().note.to_end();
            pvoice->notes.back().gphBox.pos = pos;
            pvoice->notes.back().gphBox.width = 1000;
            pvoice->notes.back().gphBox.height = _round(viewport->umtopx_v(  HEAD_HEIGHT(v->begin.staff())
                                                                           * (v->begin.staff().line_count - 1)));
        }
        else
        {
            // add the voice (with "begin" at end)
            plate->lines.back().voices.push_back(Plate::pVoice(*cur));
            pvoice = --plate->lines.back().voices.end();
            pvoice->context = v->context;
            
            // calculate the voice position
            pvoice->basePos.x = pick.get_indent();
            pvoice->basePos.y = cur->ypos + viewport->umtopx_v(pick.staff_offset(v->begin.staff()));
            pvoice->time = start_time;
            pvoice->head_height = _round(viewport->umtopx_v(HEAD_HEIGHT(v->begin.staff())));
        };
    };
    
    // increment on-plate line iterator
    if (pagebreak)
        pline = plate->lines.begin();   // initialize target line iterator (first of new page)
    else
        ++pline;                        // increment target line iterator
    
    // we have not reached the end of score, yet
    return !pick.eos();
}

// calculate beam information and adjust stem lengths
void EngraverState::engrave_beam(const Chord& chord, const StemInfo& info)
{
    BeamInfoMap::iterator i = beaminfo.find(&*pvoice);
    if (i == beaminfo.end())
        i = beaminfo.insert(BeamInfoMap::value_type(&*pvoice, BeamInfo(*pvoice))).first;
    i->second.apply1(chord, param->beam_group, info);
}

// add the given offset in front of the note to be engraved
void EngraverState::add_offset(const mpx_t offset)
{
    const mpx_t refpos = pnote->absolutePos.front().x;  // save reference front position
    
    // apply offset to engraved notes (including oneself)
    for (std::list<Plate::pVoice>::iterator i = pline->voices.begin(); i != pline->voices.end(); ++i)
    {
        // iterate through the voice backwards
        for (std::list<Plate::pNote>::reverse_iterator n = i->notes.rbegin(); n != i->notes.rend(); ++n)
        {
            // if we got a note, which is early enough to not be influenced by the offset
            if (n->absolutePos.front().x < refpos)
            {
                if (n->virtual_obj)     // ignore barline
                    if (n->virtual_obj->object->is(Class::BARLINE))
                        continue;
                
                if (n != i->notes.rend() && !n->ties.empty())   // do not move broken tie front
                    if (n->ties.front().pos1.x <= n->ties.front().pos2.x)
                        n->add_tieend_offset(offset);
                
                break;  // stop iterating the voice
            };
            
            // otherwise apply offset
            n->add_offset(offset);
            if (!n->ties.empty() && n->ties.front().pos1.x <= n->ties.front().pos2.x)
                n->add_tieend_offset(offset);   // do not move broken tie front
        };
    };
    
    // apply offset to notes yet to be engraved
    pick.add_distance(offset, get_time());
}

// logging control
void EngraverState::log_set(Log& log)
{
    this->Logging::log_set(log);
    pick.log_set(log);
}

void EngraverState::log_set(Logging& log)
{
    this->Logging::log_set(log);
    pick.log_set(log);
}

void EngraverState::log_unset()
{
    this->Logging::log_unset();
    pick.log_unset();
}


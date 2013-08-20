
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2013 Dominik Lehmann

  Licensed under the EUPL, Version 1.1 or - as soon they
  will be approved by the European Commission - subsequent
  versions of the EUPL (the "Licence");
  You may not use this work except in compliance with the
  Licence.
  
  Unless required by applicable law or agreed to in
  writing, software distributed under the Licence is
  distributed on an "AS IS" basis, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either expressed or implied.
  See the Licence for the specific language governing
  permissions and limitations under the Licence.
*/

#include <cmath>                // sqrt
#include <set>                  // std::multiset

#include "engraver_state.hh"    // EngraverState
#include "undefined.hh"         // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                                // this number is interpreted as an undefined value
using namespace ScorePress;

inline int _round(const double d) {return static_cast<int>(d + 0.5);}

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
                = pline->get_voice(*static_cast<const SubVoice&>(cursor.voice()).parent);
            
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
            parent = pline->get_voice(*static_cast<const SubVoice&>(cursor.voice()).parent); // get parent
            
            if (parent != pline->voices.end())  // if we got the parent voice
            {
                pvoice->context = parent->context;  // copy the context
                pvoice->context.reset_buffer();     // reset the buffer
                got_ctx = true;                     // got it!
            }
            else if (pline != plate->lines.begin()) // else try to find it in the previous line
            {
                parent = (--pline)->get_voice(*static_cast<const SubVoice&>(cursor.voice()).parent);
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
        if (pline->basePos.x > pvoice->basePos.x)
            pline->basePos.x = pvoice->basePos.x;
        if (pline->basePos.y > pvoice->basePos.y)
            pline->basePos.y = pvoice->basePos.y;
    };
    
    // update end-time stamp
    pvoice->end_time = cursor.time;
    if (cursor->is(Class::NOTEOBJECT)) pvoice->end_time += static_cast<const NoteObject&>(*cursor).value();
    if (pvoice->end_time > end_time)   end_time = pvoice->end_time;
    
    // set staff-style and head-height
    style = (!!cursor.staff().style) ? &*cursor.staff().style : &default_style; // set staff-style
    head_height = _round(viewport->umtopx_v(cursor.staff().head_height));       // set head-height
    
    // cut object on barline (if not necessary, method simply does nothing; no need to double check)
    pick.cut(pvoice->context.restbar(cursor.time));
    
    // create on-plate object (set "pnote")
    Position<mpx_t> pos(cursor.pos, cursor.ypos);
    if (cursor->is(Class::NOTEOBJECT)) pos.y += viewport->umtopx_v(pick.staff_offset(static_cast<const NoteObject&>(*cursor).staff_shift));
    else                               pos.y += viewport->umtopx_v(pick.staff_offset());
    
    pvoice->notes.push_back(Plate::pNote(pos, cursor)); // append the new note to the plate
    pnote = &pvoice->notes.back();                      // get the on-plate note-object
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
            pVoiceIt newvoice(pvoice);
            if (subvoice->on_top)
            {
                newvoice = pline->voices.insert(newvoice, Plate::pVoice(const_Cursor(cursor.staff(), *subvoice)));
            }
            else
            {
                ++newvoice;
                newvoice = pline->voices.insert(newvoice, Plate::pVoice(const_Cursor(cursor.staff(), *subvoice)));
            };
            newvoice->context = pvoice->context;
            
            // add position information to the new voice
            newvoice->basePos = pvoice->basePos;
            newvoice->time = cursor.time;
            newvoice->end_time = cursor.time;
            
            // add end-of-voice indicator object
            newvoice->notes.push_back(Plate::pNote(pnote->absolutePos.front(), newvoice->begin));
            newvoice->notes.back().gphBox.pos = newvoice->notes.back().absolutePos.front();
            newvoice->notes.back().gphBox.width = 1000;
            newvoice->notes.back().gphBox.height = head_height * (cursor.staff().line_count - 1);
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
    
    // check for end of voice
    if (   !cursor.has_next()          && cursor.remaining_duration <= 0L
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
        pvoice->notes.back().gphBox.pos.x = pnote->gphBox.right() + 2 * viewport->umtopx_h(param->min_distance);
        pvoice->notes.back().gphBox.pos.y = pvoice->basePos.y;
        pvoice->notes.back().gphBox.width = 1000;
        pvoice->notes.back().gphBox.height = head_height * (cursor.staff().line_count - 1);
        pvoice->notes.back().absolutePos.front() = pvoice->notes.back().gphBox.pos;
    }
    
    // insert barline
    else if (param->auto_barlines && pvoice->context.beat(cursor.ntime) == 0L && barcnt < pvoice->context.bar(cursor.ntime))
    {
        barcnt = pvoice->context.bar(cursor.ntime); // set bar counter
        pick.insert_barline(Barline::singlebar);    // insert barline to be engraved
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
                   _round(viewport->umtopx_v(voice->begin.staff().head_height)));
    };
}

// apply all non-accumulative offsets
void EngraverState::apply_offsets()
{
    // iterate the voices
    for (std::list<Plate::pVoice>::iterator voice = pline->voices.begin(); voice != pline->voices.end(); ++voice)
    {
        // calculate head-height for the current voice
        const mpx_t _head_height = _round(viewport->umtopx_v(pvoice->begin.staff().head_height));
        
        // iterate the voice
        for (std::list<Plate::pNote>::iterator it = voice->notes.begin(); it != voice->notes.end(); ++it)
        {
            if (it->at_end() || it->is_inserted() || !it->get_note().is(Class::VISIBLEOBJECT)) continue;
            
            if (it->get_note().get_visible().offset_x)
            {
                const mpx_t head_width = (sprites->head_width(it->sprite) * _head_height) / sprites->head_height(it->sprite);
                it->add_offset((it->get_note().get_visible().offset_x * head_width) / 1000);
            };
        };
    };
}

// begin durable on the given note (helper function for attachable engraver)
void EngraverState::begin_durable(const Durable& source, DurableInfo& target, const Plate::pVoice& voice, Plate::pNote& note)
{
    // calculate first node position
    note.attachables.push_back(
    Plate::pNote::AttachablePtr(new Plate::pDurable(
        source,
        Position<mpx_t>(_round(
            (source.typeX == Movable::PAGE)   ? viewport->umtopx_h(source.position.x) : (
            (source.typeX == Movable::LINE)   ? pline->basePos.x + viewport->umtopx_h(source.position.x) : (
            (source.typeX == Movable::STAFF)  ? voice.basePos.x + (head_height * source.position.x) / 1000 : (
            (source.typeX == Movable::PARENT) ? note.absolutePos.back().x + (head_height * source.position.x) / 1000 :
            0)))), _round(
            (source.typeY == Movable::PAGE)   ? viewport->umtopx_v(source.position.y) : (
            (source.typeY == Movable::LINE)   ? pline->basePos.y + viewport->umtopx_v(source.position.y) : (
            (source.typeY == Movable::STAFF)  ? voice.basePos.y + (head_height * source.position.y) / 1000 : (
            (source.typeY == Movable::PARENT) ? note.absolutePos.back().y + (head_height * source.position.y) / 1000 :
            0))))
        )
        )
    ));
    
    // initialize durable-information to wait for the end-position
    target.source = &source;
    target.target = &static_cast<Plate::pDurable&>(*note.attachables.back());
    target.durationcountdown = source.duration - 1;
}

// end durable on the given note (helper function for attachable engraver)
void EngraverState::end_durable(const Durable& source, Plate::pDurable& target, const Plate::pVoice& voice, const Plate::pNote& note)
{
    // calculate position of the end-node
    target.endPos.x = _round(
        (source.typeX == Movable::PAGE)   ? viewport->umtopx_h(source.end_position.x) : (
        (source.typeX == Movable::LINE)   ? pline->basePos.x + viewport->umtopx_h(source.end_position.x) : (
        (source.typeX == Movable::STAFF)  ? voice.basePos.x + (head_height * source.end_position.x) / 1000 : (
        (source.typeX == Movable::PARENT) ? note.absolutePos.back().x + (head_height * source.end_position.x) / 1000 :
        0))));
    target.endPos.y = _round(
        (source.typeY == Movable::PAGE)   ? viewport->umtopx_h(source.end_position.y) : (
        (source.typeY == Movable::LINE)   ? pline->basePos.y + viewport->umtopx_h(source.end_position.y) : (
        (source.typeY == Movable::STAFF)  ? voice.basePos.y + (head_height * source.end_position.y) / 1000 : (
        (source.typeY == Movable::PARENT) ? note.absolutePos.back().y + (head_height * source.end_position.y) / 1000 :
        0))));
    
    // calculate the graphical boundary box
    if (source.is(Class::SLUR))
    {
        const Slur& obj = static_cast<const Slur&>(source);
        target.gphBox = calculate_gphBox(
            target.absolutePos,
            (obj.typeX == Movable::PAGE || obj.typeX == Movable::LINE) ?
                Position<mpx_t>(
                    target.absolutePos.x + viewport->umtopx_h(obj.control1.x),
                    target.absolutePos.y + viewport->umtopx_v(obj.control1.y)) :
                Position<mpx_t>(
                    target.absolutePos.x + _round((head_height * obj.control1.x) / 1000.0),
                    target.absolutePos.y + _round((head_height * obj.control1.y) / 1000.0)),
            (obj.typeX == Movable::PAGE || obj.typeX == Movable::LINE) ?
                Position<mpx_t>(
                    target.endPos.x + viewport->umtopx_h(obj.control2.x),
                    target.endPos.y + viewport->umtopx_v(obj.control2.y)) :
                Position<mpx_t>(
                    target.endPos.x + _round((head_height * obj.control2.x) / 1000.0),
                    target.endPos.y + _round((head_height * obj.control2.y) / 1000.0)),
            target.endPos,
            _round((obj.thickness1 * viewport->umtopx_h(style->stem_width)) / 1000.0),
            _round((obj.thickness2 * viewport->umtopx_h(style->stem_width)) / 1000.0));
    }
    else if (source.is(Class::HAIRPIN))
    {
        const Hairpin& obj = static_cast<const Hairpin&>(source);
        target.gphBox.pos = target.absolutePos;
        target.gphBox.pos.y -= _round((obj.height * head_height) / 2000.0) + 1000;
        target.gphBox.width = target.endPos.x - target.absolutePos.x;
        target.gphBox.height = _round((obj.height * head_height) / 1000.0);
    };
    
    // scale the object
    target.endPos.x = target.absolutePos.x + ((target.endPos.x - target.absolutePos.x) * source.appearance.scale) / 1000;
    target.endPos.y = target.absolutePos.y + ((target.endPos.y - target.absolutePos.y) * source.appearance.scale) / 1000;
    target.gphBox.width = _round((target.gphBox.width * source.appearance.scale) / 1000.0);
    target.gphBox.height = _round((target.gphBox.height * source.appearance.scale) / 1000.0);
}

// engrave all stems within beams in the current line
void EngraverState::engrave_stems()
{
    bool got_beam = false;  // indicating notes with beam
    mpx_t  x1(0), y1(0);    // beam coordinates
    double slope(0.0);      // beam slope
    mpx_t  top(0);          // new stem top position
    
    // iterate the voices
    for (std::list<Plate::pVoice>::iterator voice = pline->voices.begin(); voice != pline->voices.end(); ++voice)
    {
        head_height = viewport->umtopx_v(voice->begin.staff().head_height); // get head height
        style = &*voice->begin.staff().style;                               // set style
        if (!style) style = &default_style;
        
        // iterate the voice
        std::list<Plate::pNote>::iterator beam_it;      // beam group iterator
        
        // correct stem length for notes with beam (I)
        got_beam = false;
        for (std::list<Plate::pNote>::iterator it = voice->notes.begin(); it != voice->notes.end(); ++it)
        {
            it->beam_begin = voice->notes.end();    // reset beam_begin
            
            if (got_beam)   // while on beamed note
            {
                // set begin iterator
                it->beam_begin = beam_it;
                
                // on the end of the beam
                if (beam_it->beam[VALUE_BASE - 3]->end == &*it)
                {
                    got_beam = false;   // reset beam indicator
                }
                else if (it->stem_info)     // correct stems within the beam
                {
                    top = _round(y1 + (it->stem.x - x1) * slope);
                    if ((it->stem.base > top) != (it->stem.base > it->stem.top))
                    {
                        // correct stem position
                        const SpriteInfo& headsprite = (*sprites)[it->sprite];          // get sprite-info
                        const double scale = head_height / (sprites->head_height(it->sprite) * 1000.0);
                        const unsigned int app_scale = it->get_note().get_visible().appearance.scale;
                        const mpx_t stem_width = viewport->umtopx_h(style->stem_width); // get stem width
                        const mpx_t sprite_width = _round(headsprite.width * scale * app_scale);
                        Position<mpx_t> stem;                                           // stem position
                        stem.x = _round(headsprite.get_real("stem.x") * scale * app_scale);
                        stem.y = _round(headsprite.get_real("stem.y") * scale * app_scale);
                        
                        if (it->stem.base > top) // an upward stem is right of the chord
                        {
                            it->stem.x = it->absolutePos.front().x                              // base position
                                + stem.x                                                        // offset for the sprite
                                - _round(it->stem_info->cluster ? 0 : stem_width * scale / 2);  // offset for the stem's width
                            
                            it->stem.top = top;
                            it->stem.base = it->stem_info->base_pos + (it->stem_info->base_side ?
                                _round(((headsprite.height * scale * app_scale - stem.y) * it->stem_info->base_scale) / 1000.0) :
                                stem.y);
                        }
                        else                            // downward stems are placed left of the chord
                        {
                            it->stem.x = it->absolutePos.front().x                              // base position
                             + _round(it->stem_info->cluster ? stem.x : sprite_width - stem.x + stem_width * scale / 2);
                            
                            it->stem.top = top;
                            it->stem.base = it->stem_info->top_pos + (it->stem_info->top_side ?
                                stem.y :
                                _round(((headsprite.height * scale * app_scale - stem.y) * it->stem_info->top_scale) / 1000.0));
                        };
                        it->gphBox.extend(Position<mpx_t>(it->stem.x, it->stem.top));
                    }
                    else it->stem.top = top;
                    free(it->stem_info);
                };
            };
            
            if (!got_beam && it->beam[VALUE_BASE - 3])  // on beam's begin
            {
                it->beam_begin = it;        // set on-plate begin iterator
                beam_it = it;               // set begin iterator
                got_beam = true;            // set beam indicator
                x1 = it->stem.x;            // set coordinates
                y1 = it->stem.top;          // set slope
                slope  = it->beam[VALUE_BASE - 3]->end->stem.top - y1;
                slope /= it->beam[VALUE_BASE - 3]->end->stem.x - x1;
            };
        };
        
        // engrave beams
        BeamInfo beam_info(*voice);
        value_t time(voice->time);
        beam_it = voice->notes.begin();
        for (std::list<Plate::pNote>::iterator it = voice->notes.begin(); it != voice->notes.end(); ++it)
        {
            if (!it->at_end() && it->get_note().is(Class::NOTEOBJECT))
            {
                time += static_cast<const NoteObject&>(it->get_note()).value();
                if (it->get_note().is(Class::CHORD))
                {
                    beam_info.apply(static_cast<const Chord&>(it->get_note()), it, time, param->beam_group);
                };
            };
        };
        beam_info.finish();
        
        // correct stem length for notes with beam (II)
        got_beam = false;
        for (std::list<Plate::pNote>::iterator it = voice->notes.begin(); it != voice->notes.end(); ++it)
        {
            // on beam's begin
            if (!got_beam && it->beam[VALUE_BASE - 3])
            {
                beam_it = it;               // set begin iterator
                got_beam = true;            // set beam indicator
            };
            
            // while on beamed note
            if (got_beam)
            {
                if (it->stem.base < it->stem.top)
                    it->stem.top = _round(it->stem.top + head_height * (style->beam_height + it->stem.beam_off * (style->beam_height + style->beam_distance)) / 1000.0);
                else
                    it->stem.top = _round(  it->stem.top - head_height * (it->stem.beam_off * (style->beam_height + style->beam_distance)) / 1000.0);
                
                if (beam_it->beam[VALUE_BASE - 3]->end == &*it)
                    got_beam = false;       // reset beam indicator
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
    for (std::list<Plate::pVoice>::iterator voice = pline->voices.begin(); voice != pline->voices.end(); ++voice)
    {
        head_height = viewport->umtopx_v(voice->begin.staff().head_height); // get head height
        style = &*voice->begin.staff().style;                               // set style
        if (!style) style = &default_style;
        
        // iterate the voice
        for (std::list<Plate::pNote>::iterator it = voice->notes.begin(); it != voice->notes.end(); ++it)
        {
            // ignore inserted notes
            if (it->at_end() || it->is_inserted()) continue;
            
            // check durables
            for (std::list<DurableInfo>::iterator i = durableinfo.begin(); i != durableinfo.end();)
            {
                if ((--(i->durationcountdown)) == 0)    // if the durable objects ends here
                {
                    end_durable(*i->source, *i->target, *voice, *it);    // finish the durable
                    i = durableinfo.erase(i);                                   // remove the object
                }
                else ++i;
            };
            
            // get visible object
            visible = &it->get_note().get_visible();
            
            // engrave movables
            if (visible != NULL)
            for (MovableList::const_iterator i = visible->attached.begin(); i != visible->attached.end(); ++i)
            {
                // append durable
                if ((*i)->is(Class::DURABLE))
                {
                    durableinfo.push_back(DurableInfo());
                    begin_durable(static_cast<const Durable&>(**i), durableinfo.back(), *voice, *it);
                }
                
                // append attachable
                else
                {
                    it->attachables.push_back(
                        Plate::pNote::AttachablePtr(new Plate::pAttachable(**i,
                        Position<mpx_t>(_round(
                            ((*i)->typeX == Movable::PAGE)   ? viewport->umtopx_h((*i)->position.x) : (
                            ((*i)->typeX == Movable::LINE)   ? pline->basePos.x + viewport->umtopx_h((*i)->position.x) : (
                            ((*i)->typeX == Movable::STAFF)  ? voice->basePos.x + (head_height * (*i)->position.x) / 1000 : (
                            ((*i)->typeX == Movable::PARENT) ? it->absolutePos.back().x + (head_height * (*i)->position.x) / 1000 :
                            0)))), _round(
                            ((*i)->typeY == Movable::PAGE)   ? viewport->umtopx_v((*i)->position.y) : (
                            ((*i)->typeY == Movable::LINE)   ? pline->basePos.y + viewport->umtopx_v((*i)->position.y) : (
                            ((*i)->typeY == Movable::STAFF)  ? voice->basePos.y + (head_height * (*i)->position.y) / 1000 : (
                            ((*i)->typeY == Movable::PARENT) ? it->absolutePos.back().y + (head_height * (*i)->position.y) / 1000 :
                            0))))
                        )
                        )
                    ));
                    
                    // calculate the graphical boundary box
                    if ((*i)->is(Class::TEXTAREA))
                    {
                        const TextArea& obj = static_cast<const TextArea&>(**i);
                        it->attachables.back()->gphBox.pos = it->attachables.back()->absolutePos;
                        it->attachables.back()->gphBox.width =
                            _round((viewport->umtopx_h(obj.width) * obj.appearance.scale) / 1000.0);
                        it->attachables.back()->gphBox.height =
                            _round((viewport->umtopx_h(obj.height) * obj.appearance.scale) / 1000.0);
                    }
                    else if ((*i)->is(Class::CUSTOMSYMBOL))
                    {
                        const CustomSymbol& obj = static_cast<const CustomSymbol&>(**i);
                        const SpriteId sprite_id =
                            (obj.sprite.setid == UNDEFINED) ?
                                SpriteId(0, sprites->front().undefined_symbol) :
                                ((obj.sprite.spriteid == UNDEFINED) ?
                                    SpriteId(obj.sprite.setid, (*sprites)[obj.sprite.setid].undefined_symbol) :
                                    obj.sprite);
                        const double scale =  (head_height / sprites->head_height(sprite_id))
                                            * (obj.appearance.scale / 1000.0);
                        
                        it->attachables.back()->gphBox.pos    = it->attachables.back()->absolutePos;
                        it->attachables.back()->gphBox.width  = _round(scale * (*sprites)[sprite_id].width);
                        it->attachables.back()->gphBox.height = _round(scale * (*sprites)[sprite_id].height);
                    };
                };
            };
        };
        
        if (!durableinfo.empty())   // warn for durable objects exceeding the linebreak
        {
            log_warn("Got durable object exceeding the end of the line or voice. (class: EngraverState)");
            for (std::list<DurableInfo>::iterator i = durableinfo.begin(); i != durableinfo.end(); ++i)
                end_durable(*i->source, *i->target, *voice, voice->notes.back());
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
        double brace_scale = viewport->umtopx_v(staff->head_height);
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
                + viewport->umtopx_v(curlybrace_end->begin.staff().head_height * (curlybrace_end->begin.staff().line_count - 1))
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
                + viewport->umtopx_v(bracket_end->begin.staff().head_height * (bracket_end->begin.staff().line_count - 1))
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
        
        for (std::list<Plate::pVoice>::iterator voice = pline->voices.begin(); voice != pline->voices.end(); ++voice)
        {
            voice->basePos.x += offset;
            voice->brace.gphBox.pos.x += offset;
            voice->bracket.gphBox.pos.x += offset;
            voice->bracket.line_base.x += offset;
            voice->bracket.line_end.x += offset;
            for (std::list<Plate::pNote>::iterator note = voice->notes.begin(); note != voice->notes.end(); ++note)
            {
                note->add_offset(offset);
                note->add_tieend_offset(offset);
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
// TODO: don't do forced justification; check "param->force_justification"
void EngraverState::justify_line()
{
    // initialization and check
    const mpx_t   width = pline->line_end - pline->basePos.x;               // current line width
    const mpx_t   diff  =   viewport->umtopx_h(lineinfo.dimension->width)   // space to be added
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
            tgt->end   = npos = it->gphBox.right();
            tgt->dist  = tgt->pos - tgt->begin;
            tgt->tag   = (!it->at_end()) ? classname(it->get_note().classtype()) : "EOV";
            gphs.insert(GraphicData(it->gphBox.pos.x, it->gphBox.right()));
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
    pline->line_end += diff;    // move line-end
    
    // iterate the voices
    mpx_t prev_offset;  // previous offset
    mpx_t offset;       // current offset
    bool  pre_tie;      // indicating the second part of a broken tie
    bool  got_tie;      // indicating the first  part of a broken tie
    for (Plate::pLine::Iterator voice = pline->voices.begin(); voice != pline->voices.end(); ++voice)
    {
        // initialize
        offset  = 0;
        pre_tie = false;
        got_tie = false;
        
        // iterate the voice
        Plate::pVoice::Iterator               note_it = voice->notes.begin();
        std::multiset<DistanceData>::iterator data_it = dists.begin();
        for (; note_it != voice->notes.end(); ++note_it, ++data_it)
        {
            while (data_it->src != &*voice) ++data_it;                      // move to correct data element
            prev_offset = offset;                                           // save previous offset
            offset += _round((double(diff) * data_it->dist) / dist_sum);    // calculate offset
            
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
PageSet::ScoreDimension EngraverState::dimtopx(const ScoreDimension& dim)
{
    return PageSet::ScoreDimension(viewport->umtopx_h(dim.position.x), viewport->umtopx_v(dim.position.y),
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

// calculate the graphical box for the voices within a line
void EngraverState::calculate_gphBox(Plate::pLine& line)
{
    line.gphBox.extend(Position<mpx_t>(line.line_end, line.gphBox.pos.y));
    
    // iterate the voices
    for (std::list<Plate::pVoice>::iterator voice = line.voices.begin(); voice != line.voices.end(); ++voice)
    {
        // iterate the voice
        for (std::list<Plate::pNote>::iterator pnote = voice->notes.begin(); pnote != voice->notes.end(); ++pnote)
        {
            line.gphBox.extend(pnote->gphBox);
            
            for (Plate::pNote::AttachableList::iterator a = pnote->attachables.begin(); a != pnote->attachables.end(); ++a)
                line.gphBox.extend((*a)->gphBox);
        };
    };
}

// calculate the graphical box for the given bezier spline
Plate::pGraphical::Box EngraverState::calculate_gphBox(Position<mpx_t> p1, Position<mpx_t> c1, Position<mpx_t> c2, Position<mpx_t> p2, mpx_t w0, mpx_t w1)
{
    double tx1, tx2, ty1, ty2;  // extreme parameters
    
    // calculate extreme parameters (X coordinate)
    if (p2.x - 3 * c2.x + 3 * c1.x - p1.x != 0) // cubic polynomial
    {
        const double px = (c2.x - 2.0 * c1.x + p1.x) / (p2.x - 3.0 * c2.x + 3.0 * c1.x - p1.x);
        const double qx = (3.0 * (c1.x - p1.x)) / (p2.x - 3.0 * c2.x + 3.0 * c1.x - p1.x);
        if (px * px - qx >= 0)    // monotonous?
        {       // non-monotonous => extremum
            tx1 = -px + sqrt(px * px - qx);
            tx2 = -px - sqrt(px * px - qx);
            if (tx1 < 0 || tx1 > 1) tx1 = 0;    // cut to interval [0,1]
            if (tx2 < 0 || tx2 > 1) tx2 = 0;
        }
        else    // monotonous => extremum in endpoint
        {
            tx1 = tx2 = 0;
        };
    }
    else if (c2.x - 2 * c1.x + p1.x != 0)   // quadratic polynomial
    {
        tx1 = tx2 = -(c1.x - p1.x) / (2.0 * (c2.x - 2.0 * c1.x + p1.x));
        if (tx1 < 0 || tx1 > 1) tx1 = 0;    // cut to interval [0,1]
        if (tx2 < 0 || tx2 > 1) tx2 = 0;
    }
    else    tx1 = tx2 = 0;  // linear polynomial => extremum in endpoint
    
    // calculate extreme parameters (Y coordinate)
    if (p2.y - 3 * c2.y + 3 * c1.y - p1.y != 0) // cubic polynomial
    {
        const double py = (c2.y - 2.0 * c1.y + p1.y) / (p2.y - 3.0 * c2.y + 3.0 * c1.y - p1.y);
        const double qy = (3.0 * (c1.y - p1.y)) / (p2.y - 3.0 * c2.y + 3.0 * c1.y - p1.y);
        if (py * py - qy >= 0)     // monotonous?
        {       // non-monotonous => extremum
            ty1 = -py + sqrt(py * py - qy);
            ty2 = -py - sqrt(py * py - qy);
            if (ty1 < 0 || ty1 > 1) ty1 = 0;    // cut to interval [0,1]
            if (ty2 < 0 || ty2 > 1) ty2 = 0;
        }
        else    // monotonous => extremum in endpoint
        {
            ty1 = ty2 = 0;
        };
    }
    else if (c2.y - 2 * c1.y + p1.y != 0)   // quadratic polynomial
    {
        ty1 = ty2 = - (c1.y - p1.y) / (2.0 * (c2.y - 2.0 * c1.y + p1.y));
        if (ty1 < 0 || ty1 > 1) ty1 = 0;    // cut to interval [0,1]
        if (ty2 < 0 || ty2 > 1) ty2 = 0;
    }
    else    ty1 = ty2 = 0;  // linear polynomial => extremum in endpoint
    
    // calculate bezier-function values
    const double x1 = (1-tx1)*(1-tx1)*((1-tx1)*p1.x + 3*tx1*c1.x) + tx1*tx1*(3*(1-tx1)*c2.x + tx1*p2.x);
    const double x2 = (1-tx2)*(1-tx2)*((1-tx2)*p1.x + 3*tx2*c1.x) + tx2*tx2*(3*(1-tx2)*c2.x + tx2*p2.x);
    const double y1 = (1-ty1)*(1-ty1)*((1-ty1)*p1.y + 3*ty1*c1.y) + ty1*ty1*(3*(1-ty1)*c2.y + ty1*p2.y);
    const double y2 = (1-ty2)*(1-ty2)*((1-ty2)*p1.y + 3*ty2*c1.y) + ty2*ty2*(3*(1-ty2)*c2.y + ty2*p2.y);
    
    // calculate extremata                                         (and consider line width)
    const mpx_t Mx = _round((x1 > x2 && x1 > p1.x && x1 > p2.x) ? x1   + 2*(w1-w0)*tx1*(1-tx1)+w0 :
                           ((x2 > p1.x && x2 > p2.x)            ? x2   + 2*(w1-w0)*tx2*(1-tx2)+w0 :
                           ((p1.x > p2.x)                       ? p1.x + w0 / 2 :
                                                            p2.x + w0 / 2 )));
    const mpx_t mx = _round((x1 < x2 && x1 < p1.x && x1 < p2.x) ? x1   - 2*(w1-w0)*tx1*(1-tx1)-w0 :
                           ((x2 < p1.x && x2 < p2.x)            ? x2   - 2*(w1-w0)*tx2*(1-tx2)-w0 :
                           ((p1.x < p2.x)                       ? p1.x - w0 / 2 :
                                                            p2.x - w0 / 2)));
    const mpx_t My = _round((y1 > y2 && y1 > p1.y && y1 > p2.y) ? y1   + 2*(w1-w0)*ty1*(1-ty1)+w0 :
                           ((y2 > p1.y && y2 > p2.y)            ? y2   + 2*(w1-w0)*ty2*(1-ty2)+w0 :
                           ((p1.y > p2.y)                       ? p1.y + w0 / 2 :
                                                            p2.y + w0 / 2 )));
    const mpx_t my = _round((y1 < y2 && y1 < p1.y && y1 < p2.y) ? y1   - 2*(w1-w0)*ty1*(1-ty1)-w0 :
                           ((y2 < p1.y && y2 < p2.y)            ? y2   - 2*(w1-w0)*ty2*(1-ty2)-w0 :
                           ((p1.y < p2.y)                       ? p1.y - w0 / 2 :
                                                            p2.y - w0 / 2 )));
    
    // create box
    return Plate::pGraphical::Box(Position<mpx_t>(mx, my), Mx - mx, My - my);
}

// constructor (will erase "score" from the "pageset" and prepare for engraving)
EngraverState::EngraverState(const Score&         _score,
                             const unsigned int   _start_page,
                                   PageSet&       _pageset,
                             const Sprites&       _sprites,
                             const EngraverParam& _param,
                             const StyleParam&    _style,
                             const ViewportParam& _viewport) : sprites(&_sprites),
                                                               param((!!_score.param) ? &*_score.param : &_param),
                                                               style(&_style),
                                                               default_style(_style),
                                                               viewport(&_viewport),
                                                               pick(_score, _param, _viewport, _sprites),
                                                               pageset(&_pageset),
                                                               pagecnt(0),
                                                               barcnt(0),
                                                               start_time(0),
                                                               end_time(0)
{
    // prepare the pageset
    pageset->erase_score(_score);   // erase the plates
    if (pick.eos()) return;         // ignore empty score
    
    // initialize local variables
    page = pageset->get_page(_start_page);
    page->plates.push_back(PageSet::PlateInfo(_start_page, _score, dimtopx(pick.get_dimension())));
    plate = &page->plates.back().plate;
    lineinfo.dimension = &pick.get_dimension();
    lineinfo.indent = pick.get_indent();
    lineinfo.justify = pick.get_justify();
    lineinfo.right_margin = pick.get_right_margin();
    start_time = pick.get_cursor().time;
    
    // prepare the plate
    plate->lines.push_back(Plate::pLine());     // push first line
    pline = plate->lines.begin();               // initialize target line iterator
    pline->basePos.x = pick.get_indent();       // set the base-position of the new line (checked with new voice)
    pline->basePos.y = pick.get_cursor().ypos + viewport->umtopx_v(pick.staff_offset(_score.staves.front()));
    pline->gphBox.pos = plate->lines.back().basePos;    // initialize graphical boundary box
    
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
    engrave_stems();                // engrave all the missing stems
    engrave_attachables();          // engrave all the line's attached objects
    engrave_braces();               // engrave braces and brackets
    if (lineinfo.justify)           // justifiy the line
        justify_line();
    apply_offsets();                // apply non-cumulative offsets
    calculate_gphBox(*pline);       // calculate the graphical boundary box
    barcnt = pvoice->context.bar(pick.get_cursor().time);
    
    // exit here, if no newline (below is the code for newline handling)
    if (!newline) return false;
    
    // check for pagebreak
    if (pagebreak)
    {
        if (++page == pageset->pages.end())
        {
            pageset->pages.push_back(PageSet::pPage());
            page = --pageset->pages.end();
        };
        page->plates.push_back(PageSet::PlateInfo(++pagecnt, get_score(), dimtopx(pick.get_dimension())));
        plate = &page->plates.back().plate;
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
    lineinfo.right_margin = pick.get_right_margin();
    
    // add voices
    const Pick::VoiceCursor* cur;
    Position<mpx_t>          pos;
    for (Plate::PVoiceList::iterator v = pline->voices.begin(); v != pline->voices.end(); ++v)
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
            
            // calculate the EOV indicator position
            pos = pvoice->basePos;
            pos.x += viewport->umtopx_h(param->min_distance);
            
            // add end-of-voice indicator object
            pvoice->notes.push_back(Plate::pNote(pos, const_Cursor(v->begin)));
            pvoice->notes.back().note.to_end();
            pvoice->notes.back().gphBox.pos = pos;
            pvoice->notes.back().gphBox.width = 1000;
            pvoice->notes.back().gphBox.height = head_height * (v->begin.staff().line_count - 1);
        }
        else
        {
            // add the voice (with "begin" at end)
            plate->lines.back().voices.push_back(Plate::pVoice(*cur));
            pvoice = --plate->lines.back().voices.end();
            pvoice->context = v->context;
            
            // calculate the voice position
            pvoice->basePos.x = pick.get_indent();
            pvoice->basePos.y = pick.get_cursor().ypos + viewport->umtopx_v(pick.staff_offset(v->begin.staff()));
            pvoice->time = start_time;
        };
    };
    
    // increment on-plate line iterator
    ++pline;
    
    // we have not reached the end of score, yet
    return !pick.eos();
}

// calculate beam information and adjust stem lengths
void EngraverState::engrave_beam(const Chord& chord, const StemInfo& info)
{
    BeamInfoMap::iterator i = beaminfo.find(&*pvoice);
    if (i == beaminfo.end())
        i = beaminfo.insert(BeamInfoMap::value_type(&*pvoice, BeamGroupInfo(*pvoice))).first;
    i->second.apply(chord, param->beam_group, info);
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

void EngraverState::log_unset()
{
    this->Logging::log_unset();
    pick.log_unset();
}

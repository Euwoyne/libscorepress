
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2016 Dominik Lehmann
  
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

#include <iostream>
#include <cmath>        // pow
#include "pick.hh"      // Pick, const_Cursor, Sprites, List, [score classes]
#include "undefined.hh" // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                        // this number is interpreted as an undefined value
using namespace ScorePress;

inline int _round(const double d) {return static_cast<mpx_t>(d + 0.5);}
#define HEAD_HEIGHT(staff) ((staff).head_height ? (staff).head_height : this->head_height)


//
//     class VoiceCursor
//    ===================
//
// a special cursor containing position and time information about the note
// pointed to
//

// constructor for the "VoiceCursor" class
Pick::VoiceCursor::VoiceCursor() : const_Cursor(), pos(0), npos(0), ypos(0), time(0), ntime(0),
                                   virtual_obj(NULL), inserted(false), remaining_duration(-1) {}


//     class LineLayout
//    ==================
//
// collection of newlines for each voice
//

// exception class
Pick::LineLayout::VoiceNotFoundException::VoiceNotFoundException()
    : ScorePress::Error("Cannot find layout of requested voice. (class Pick::LineLayout)") {}

// associate a voice with its newline object
void Pick::LineLayout::set(const Voice& voice, const LayoutParam& layout)
{
    data[&voice] = &layout;
}

// check, if the voice has a newline object
bool Pick::LineLayout::exist(const Voice& voice) const
{
    std::map<const Voice*, const LayoutParam*>::const_iterator i = data.find(&voice);
    return (i != data.end() && i->second);
}

// get arbitrary newline object (for line-properties)
const LayoutParam& Pick::LineLayout::get() const
{
    if (first_voice) return get(*first_voice);
    if (data.empty()) throw VoiceNotFoundException();
    return *data.begin()->second;
}

// get the newline object of the given voice
const LayoutParam& Pick::LineLayout::get(const Voice& voice) const
{
    std::map<const Voice*, const LayoutParam*>::const_iterator i = data.find(&voice);
    if (i == data.end() || !i->second) throw VoiceNotFoundException();
    return *i->second;
}

// remove a voice's layout
void Pick::LineLayout::remove(const Voice& voice)
{
    data.erase(&voice);
    if (first_voice == &voice) first_voice = NULL;
}

// set the voice for line-properties
void Pick::LineLayout::set_first_voice(const Voice& voice)
{
    if (data.find(&voice) == data.end()) throw VoiceNotFoundException();
    first_voice = &voice;
}

//
//     class VoiceOrder
//    ==================
//
// maps voices to their index thus providing an order comparison for voices
//

// exception class
Pick::VoiceOrder::VoiceNotFoundException::VoiceNotFoundException()
    : ScorePress::Error("Cannot find index of parent voice. (class Pick::VoiceOrder)") {}

// insert new voice above "parent"
void Pick::VoiceOrder::add_above(const Voice& voice, const Voice& parent)
{
    iterator i = find(&parent);
    if (i == end()) throw VoiceNotFoundException();
    const size_t idx = i->second;
    for (i = begin(); i != end(); ++i)
        if (i->second >= idx)
            ++i->second;
    insert(value_type(&voice, idx));
}

// insert new voice above "parent"
void Pick::VoiceOrder::add_below(const Voice& voice, const Voice& parent)
{
    iterator i = find(&parent);
    if (i == end()) throw VoiceNotFoundException();
    const size_t idx = i->second + 1;
    for (i = begin(); i != end(); ++i)
        if (i->second >= idx)
            ++i->second;
    insert(value_type(&voice, idx));
}

// insert new staff at the top
void Pick::VoiceOrder::add_above(const Staff& staff)
{
    for (iterator i = begin(); i != end(); ++i)
        ++i->second;
    insert(value_type(&static_cast<const Voice&>(staff), 0));
}

// insert new staff at the bottom
void Pick::VoiceOrder::add_below(const Staff& staff)
{
    insert(value_type(&static_cast<const Voice&>(staff), size()));
}

// check voice order ("v1" above "v2"?)
bool Pick::VoiceOrder::is_above(const Voice& v1, const Voice& v2) const
{
    const const_iterator i1 = find(&v1);
    const const_iterator i2 = find(&v2);
    if (i1 == end() || i2 == end()) throw VoiceNotFoundException();
    return i1->second < i2->second;
}

// check voice order ("v1" below "v2"?)
bool Pick::VoiceOrder::is_below(const Voice& v1, const Voice& v2) const
{
    const const_iterator i1 = find(&v1);
    const const_iterator i2 = find(&v2);
    if (i1 == end() || i2 == end()) throw VoiceNotFoundException();
    return i1->second > i2->second;
}


//
//     class Pick
//    ============
//
// This class is an iterator for "Score", which, for each iteration step,
// calculates the position of the current Staff-Object.
// This is done parallely for each voice in the score to ensure, that the
// Objects are ordered according to their position along the score.
//

// comparison defining the engraving order for VoiceCursor
bool Pick::compare(const VoiceCursorPtr& cur1, const VoiceCursorPtr& cur2)
{
    if ((*cur1)->is(Class::PAGEBREAK))
        return (!(*cur2)->is(Class::PAGEBREAK) || cur2->time < cur1->time);
    if ((*cur2)->is(Class::PAGEBREAK))
        return false;
    if ((*cur1)->is(Class::NEWLINE))
        return (!(*cur2)->is(Class::NEWLINE) || cur2->time < cur1->time);
    if ((*cur2)->is(Class::NEWLINE))
        return false;
    if (cur1->time != cur2->time)
        return (cur2->time < cur1->time);
    if (cur1->ntime != cur2->ntime)
        return (cur2->ntime < cur1->ntime);
    if ((*cur1)->classtype() == (*cur2)->classtype())
        return (cur2->time < cur1->time);
    
    if ((*cur1)->is(Class::TIMESIG)) return true;
    if ((*cur2)->is(Class::TIMESIG)) return false;
    if ((*cur1)->is(Class::KEY))     return true;
    if ((*cur2)->is(Class::KEY))     return false;
    if ((*cur1)->is(Class::BARLINE)) return true;
    if ((*cur2)->is(Class::BARLINE)) return false;
    return !((*cur1)->is(Class::CLEF));
}

// return the graphical width for the number
mpx_t Pick::width(const SpriteSet& spr, unsigned int n, const mpx_t height)
{
    mpx_t width = 0;
    
    do
    {
        width += (    spr[
                    (spr.digits_time[n % 10] == UNDEFINED) ?
                        spr.undefined_symbol :
                        spr.digits_time[n % 10]
                ].width * height
            ) / spr.head_height;
        
        width += (spr.timesig_digit_space * height) / spr.head_height;
        n /= 10;
    } while(n != 0);
    
    return width;
}

// return the width of the staff-object's graphic
#define _width(idx, type)      \
    ((height == 0) ?            \
        spr[idx].width * 1000 : \
        (((spr[idx].width * height) / spr.head_height(idx)) * static_cast<const type*>(obj)->appearance.scale) / 1000)

mpx_t Pick::width(const Sprites& spr, const StaffObject* obj, const mpx_t height)
{
    SpriteId idx = obj->get_sprite(spr);    // get sprite id (if possible)
    
    if (obj->is(Class::CHORD))          // if the object is a chord...
    {
        return _width(idx, Chord);
    }
    else if (obj->is(Class::CLEF))      // if the object is a clef...
    {
        return _width(idx, Clef);
    }
    else if (obj->is(Class::REST))      // if the object is a rest...
    {
        const Rest& rest = *static_cast<const Rest*>(obj);
        
        // if it is a simple rest, just return the width of the sprite
        if (rest.val.exp >= VALUE_BASE - 2) return _width(idx, Rest);
        
        // otherwise calculate width of composed graphic
        if (spr[idx].real.find("stem.slope") == spr[idx].real.end())    // if there is no slope,
            return spr[idx].width * 1000;                               //    return simple width
        
        return (height == 0) ? (    // if height is not given, suppose the sprite-set's default height
             _round((
                        spr[idx].width + 
                        spr[idx].real.find("stem.slope")->second * (VALUE_BASE - 2 - rest.val.exp)
                     ) * rest.appearance.scale
               )) : (               // on given height, scale appropriately
             _round(((((
                        spr[idx].width + 
                        spr[idx].real.find("stem.slope")->second * (VALUE_BASE - 2 - rest.val.exp)
                     ) * height) / spr.head_height(idx)) * rest.appearance.scale) / 1000
               ));
    }
    else if (obj->is(Class::KEY))       // if the object is a key...
    {   // multiply number with sprite width
        return static_cast<const Key*>(obj)->number * _width(idx, Key);
    }
    else if (obj->is(Class::TIMESIG))   // if the object is a time-signature...
    {
        // set-id buffer
        size_t setid = 0;
        
        // if a custom sprite is given
        if (obj->is(Class::CUSTOMTIMESIG))
        {
            const SpriteId& sprite = static_cast<const CustomTimeSig*>(obj)->sprite;
            if (sprite.ready())                     // and if the sprite information is complete
                return _width(sprite, TimeSig);     //    return width of that sprite
            else if (sprite.setid != UNDEFINED)     // if we only have a set-id
                setid = sprite.setid;               //    change set-id for normal timesig
            
            // if the sprite is not ready, handle object like a normal time-signature
        };
        
        // get the width of the "number of beats" (within the default set)
        mpx_t width_number = width(spr[setid], static_cast<const TimeSig*>(obj)->number, height);
        
        // get the width of the beat (within the default set)
        mpx_t width_beat = width(spr[setid], static_cast<const TimeSig*>(obj)->beat, height);
        
        // return the maximum of both
        return (width_number < width_beat) ? width_beat : width_number;
    }
    else if (obj->is(Class::BARLINE))   // if the object is a barline...
    {
        mpx_t out = 0;
        for (std::string::const_iterator i  = static_cast<const Barline*>(obj)->style.begin();
                                         i != static_cast<const Barline*>(obj)->style.end();
                                         ++i)
            out += *i;
        return out;
    };
    
    return 0;   // return zero width for non-graphical objects
}

// caluclate the width specified by the value of the note
mpx_t Pick::value_width(const value_t& value, const EngraverParam& param, const ViewportParam& viewport)
{
    return _round(((param.exponent == 1000L) ?
            (value.real() * param.linear_coeff * viewport.hppm) / 1e3 :
            (pow(value.real(), param.exponent / 1.0e3) * param.linear_coeff * viewport.hppm) / 1e3
           ) + param.constant_coeff * viewport.hppm / 1e3);
}

// add cursors for the sub-voices of a note to the queue
void Pick::add_subvoices(const VoiceCursor& parent, CQueue& cqueue)
{
    if (!parent->is(Class::NOTEOBJECT)) return;                         // check if object can have voices
    const NoteObject& obj = static_cast<const NoteObject&>(*parent);    // cast to "NoteObject"
    bool below = false;                                                 // SubVoice position indicator
    for (SubVoiceList::const_iterator voice = obj.subvoices.begin(); voice != obj.subvoices.end(); ++voice)
    {
        if (obj.subvoices.is_first_below(voice))
            below = true;
        
        if (!(*voice)->notes.empty())   // ignore empty voices (checked by engraver)
        {
            // create cursor to the new voice's beginning
            VoiceCursorPtr cursor = VoiceCursorPtr(new VoiceCursor());
            cursor->set(parent.staff(), **voice);
            cursor->parent = parent;
            
            // set layout
            _layout.set(**voice, _layout.get(parent.voice()));
            
            // set position
            cursor->pos = parent.pos;   // copy horizontal position
            cursor->ypos = parent.ypos; // copy vertical position
            calculate_npos(*cursor);    // calculate estimated position of the following note
            
            // setup time-stamp
            cursor->time = parent.time;             // set time to current timestamp
            cursor->ntime = parent.time;            // initialize end-time
            if ((*cursor)->is(Class::NOTEOBJECT))   // if we got a note-object
            {
                cursor->ntime += static_cast<const NoteObject&>(**cursor).value();  // increase end-time
            };
            
            // insert note
            if (below) _voice_order.add_below(**voice, parent.voice());
            else       _voice_order.add_above(**voice, parent.voice());
            cqueue.push(cursor);            // add cursor to the queue
            add_subvoices(*cursor, cqueue); // add the subvoices of the first note of the new voice
        };
    };
}

// intialize the cursors to the score's beginning
void Pick::_initialize()
{
    for (std::list<Staff>::const_iterator s = score->staves.begin(); s != score->staves.end(); ++s)
    {
        if (!s->notes.empty())  // if the staff contains any notes...
        {
            VoiceCursorPtr cursor = VoiceCursorPtr(new VoiceCursor);
            
            // append main-voice
            cursor->set(*s);            // create cursor to the staff's beginning
            if (s->notes.front()->is(Class::NOTEOBJECT))
                cursor->pos = viewport->umtopx_h(param->barline_distance +              // set pos to barline distance
                                             score->staves.front().layout.indent);  // plus the initial line indent
            else
                cursor->pos = viewport->umtopx_h(param->min_distance +                  // set pos to min-distance
                                             score->staves.front().layout.indent);  // plus the initial line indent
            
            // calculate estimated position of the following note
            calculate_npos(*cursor);
            
            // add initial top-distance
            cursor->ypos = 0;
            
            // setup List time-stamp
            cursor->time = 0;                       // initialize with zero
            if ((*cursor)->is(Class::NOTEOBJECT))   // if we got a note-object
            {                                       //    set future time-stamp
                cursor->ntime = static_cast<const NoteObject&>(**cursor).value();
            };
            
            // set layout
            _layout.set(*s, s->layout);
            _layout.set_first_voice(*s);
            
            // insert note
            _voice_order.add_below(*s);
            cursors.push(cursor);   // add cursor to the queue
            add_subvoices(*cursor); // add the subvoices of the first note of the new voice
        };
    };
    
    // set dimension of the first page
    _dimension = &score->layout.dimension;
}

// calculate estimated position of the following note
void Pick::calculate_npos(VoiceCursor& nextNote)
{
    // the note's graphical width (plus minimal distance)
    const mpx_t note_width = width(*sprites,
                                   &*nextNote,
                                   viewport->umtopx_v(HEAD_HEIGHT(nextNote.staff())));
    nextNote.npos = nextNote.pos + note_width;
}

// insert next note of current voice
void Pick::insert_next(const VoiceCursor& engravedNote)
{
    VoiceCursorPtr nextNotePtr = VoiceCursorPtr(new VoiceCursor(engravedNote));
    VoiceCursor& nextNote(*nextNotePtr);    // copy current cursor
    
    // virtual object
    if (engravedNote.virtual_obj)       // if the object was virtual
    {
        if (engravedNote.remaining_duration < 0L)   // and it was not a cut note
        {
            if (engravedNote.inserted)              // if we have an inserted note
            {               // search for the already existant next one in voice
                for (CQueue::iterator i = cursors.begin(); i != cursors.end(); ++i)
                {
                    if ((*i)->time == engravedNote.time)
                    {
                        (*i)->npos += (engravedNote.npos - (*i)->pos);
                        (*i)->pos = engravedNote.npos;  // update the next notes position
                    };
                };
                return;                 // if the note was inserted, the next one is already there
            };
            ++nextNote;                     // eventually goto next note
            free(nextNote.virtual_obj);     // remove virtual object reference
            nextNote.inserted = false;      // reset insert flag
        }
        else if (engravedNote.remaining_duration == 0L) // if it was the last part of a cut note
        {
            ++nextNote;                         // goto next note
            free(nextNote.virtual_obj);         // remove virtual object reference
            nextNote.inserted = false;          // reset insert flag
            nextNote.remaining_duration = -1;   // reset remaining duration
        }
        else if (engravedNote->is(Class::CHORD))    // if it was a virtually tied note
        {
            Chord* nchord = new Chord(static_cast<const Chord&>(engravedNote.original()), true);
            nchord->set_value(engravedNote.remaining_duration);
            nextNote.virtual_obj = StaffObjectPtr(nchord);
            nextNote.inserted = true;
            nextNote.remaining_duration = 0;
        }
        else if (engravedNote->is(Class::REST)) // if it was a broken rest
        {
            Rest* nrest = new Rest(static_cast<const Rest&>(engravedNote.original()), true);
            nrest->set_value(engravedNote.remaining_duration);
            nextNote.virtual_obj = StaffObjectPtr(nrest);
            nextNote.inserted = true;
            nextNote.remaining_duration = 0;
        };
    }
    
    // pagebreak
    else if (engravedNote->is(Class::PAGEBREAK))    // if we just got a pagebreak
    {
        // pagebreak among newlines is illegal
        if (_newline)
            log_warn("Got pagebreak object whilst processing a newline. (class: Pick)");
        
        // on the first pagebreak encountered
        if (!_pagebreak && !_newline)
        {
            // store height of this line
            _pagebreak = true;
            _line_height = viewport->umtopx_v(line_height());
            
            // set newline time
            for (CQueue::iterator i = cursors.begin(); i != cursors.end(); ++i)
                if ((*i)->time > _newline_time) _newline_time = (*i)->time;
            if (param->newline_time_reset)
                for (CQueue::iterator i = cursors.begin(); i != cursors.end(); ++i)
                    (*i)->ntime = _newline_time;
        };
        
        // copy new line layout
        const Pagebreak& obj = static_cast<const Pagebreak&>(*engravedNote);
        if (obj.dimension.width && obj.dimension.height)    // set new score dimension
            _dimension = &obj.dimension;
        _next_layout.set(engravedNote.voice(), obj.layout); // set new line layout
        _next_layout.set_first_voice(engravedNote.voice());
        
        // reset "pos" and "ypos"
        nextNote.pos = viewport->umtopx_h(param->min_distance + obj.layout.indent);
        if (param->newline_time_reset || !obj.layout.visible)
            nextNote.ntime = _newline_time;
        
        if (!(++nextNote).at_end()) calculate_npos(nextNote);
    }
    
    // newline
    else if (engravedNote->is(Class::NEWLINE))  // if the object was a newline
    {
        // newline among pagebreaks is illegal
        if (_pagebreak)
            log_warn("Got newline object whilst processing a pagebreak. (class: Pick)");
        
        // on the first newline encountered
        if (!_newline && !_pagebreak)
        {
            // store height of this line
            _newline = true;
            _line_height = viewport->umtopx_v(line_height());
            
            // set newline time
            _newline_time = engravedNote.ntime;
            for (CQueue::iterator i = cursors.begin(); i != cursors.end(); ++i)
                if ((*i)->time > _newline_time) _newline_time = (*i)->time;
            if (param->newline_time_reset)
                for (CQueue::iterator i = cursors.begin(); i != cursors.end(); ++i)
                    (*i)->ntime = _newline_time;
        };
        
        // copy new line layout
        const Newline& obj = static_cast<const Newline&>(*engravedNote);
        _next_layout.set(engravedNote.voice(), obj.layout); // set new line layout
        _next_layout.set_first_voice(engravedNote.voice());
        
        if (!(++nextNote).at_end())     // if there is a next note
        {
            // reset "pos" and add newline-distance to "ypos"
            nextNote.pos = viewport->umtopx_h(param->min_distance + obj.layout.indent);
            nextNote.ypos += _line_height;
            if (param->newline_time_reset || !obj.layout.visible)
                nextNote.ntime = _newline_time;
            
            calculate_npos(nextNote);
        };
    }
    
    // every other object
    else ++nextNote;        // goto next note
    
    // calculate time and insert next note
    if (!nextNote.at_end())                 // if there is a next note
    {
        nextNote.time = engravedNote.ntime;     // set time-stamp
        
        if (nextNote->is(Class::NOTEOBJECT))    // and if we got a note-object
        {                                       //    increase future time-stamp
            nextNote.ntime += static_cast<const NoteObject*>(&*nextNote)->value();
        }
        else if (nextNote->is(Class::NEWLINE))  // if we have a newline
        {                                       //    precalculate position (won't be changed during preparation)
            if (engravedNote->is(Class::BARLINE))
                nextNote.pos = engravedNote.pos;
            else
                nextNote.pos = engravedNote.npos;
            nextNote.npos = nextNote.pos;
        };
        
        // insert new note into stack
        ((engravedNote->is(Class::NEWLINE)) ? next_cursors : cursors).push(nextNotePtr);
        
        // add new voices
        if (!nextNote.virtual_obj)
            add_subvoices(nextNote, (engravedNote->is(Class::NEWLINE)) ? next_cursors : cursors);
    };
}

// prepare next note to be engraved
void Pick::prepare_next(const VoiceCursor& engravedNote, mpx_t w)
{
    if (cursors.empty())
    {
        if (next_cursors.empty()) return;   // if there is no next note, abort
        std::swap(cursors, next_cursors);   // swap to the next line's cursors
        _layout.swap(_next_layout);         //                     and layout
        next_cursors.clear();               // erase next line cursors
        _next_layout.clear();               //             and layout
        _newline = false;                   // we're out of the newline
        _pagebreak = false;                 //          and the pagebreak block
    };
    
    VoiceCursor& nextNote = *cursors.top(); // get the object which is to be engraved next
    
    // calculate note position
    if (nextNote->is(Class::NEWLINE))       // if we got a newline/pagebreak
    {
        if (nextNote->is(Class::PAGEBREAK))
            nextNote.ypos = 0;
        if (engravedNote->is(Class::NEWLINE))
        {
            if (engravedNote.time == nextNote.time)
                nextNote.pos = engravedNote.pos;
        }
        else if (engravedNote->is(Class::BARLINE))
            nextNote.pos = engravedNote.pos;
        else
            nextNote.pos = engravedNote.pos + w + viewport->umtopx_h(param->min_distance);
    }
    else if (engravedNote->is(Class::NEWLINE))
    {
        // add distance
        if (nextNote->is(Class::NOTEOBJECT))    // add barline distance for note objects
            nextNote.pos += viewport->umtopx_h(param->barline_distance);
        else                                    // non-note objects have just minimal distance from barlines
            nextNote.pos += viewport->umtopx_h(param->min_distance);
    }
    else if (nextNote->classtype() == engravedNote->classtype() && !engravedNote->is(Class::NOTEOBJECT)
                                                                && !engravedNote->is(Class::TIMESIG))
    {   // if there are two consequent non-note-objects of the same class (no time-signatures)
        nextNote.pos = engravedNote.pos;        // render them at the same position (horizontally)
    }
    else if (nextNote->is(Class::TIMESIG) && engravedNote->is(Class::TIMESIG) &&
             static_cast<const TimeSig*>(&*nextNote)->number == static_cast<const TimeSig*>(&*engravedNote)->number &&
             static_cast<const TimeSig*>(&*nextNote)->beat   == static_cast<const TimeSig*>(&*engravedNote)->beat)
    {   // if there are two consequent AND EQUAL time-signatures
        nextNote.pos = engravedNote.pos;    // render them at the same position (horizontally)
    }
    else if (nextNote.time <= engravedNote.time && engravedNote->is(Class::NOTEOBJECT)
                                                && nextNote->is(Class::NOTEOBJECT))
    {   // if there are two simultaneous note-objects (case < should not occur)
        if (nextNote.time < engravedNote.time) log_warn("Got wrong object order. (class: Pick)");
        nextNote.pos = engravedNote.pos;    // render them at the same position (horizontally)
    }
    else if (engravedNote->is(Class::BARLINE))  // if we just engraved a barline
    {
        nextNote.pos = engravedNote.npos;       // the next notes are rendered right behind it
        
        // add distance
        if (nextNote->is(Class::NOTEOBJECT))    // add barline distance for note objects
            nextNote.pos += viewport->umtopx_h(param->barline_distance);
        else                                    // non-note objects have just minimal distance from barlines
            nextNote.pos += viewport->umtopx_h(param->min_distance);
    }
    else    // in any other case
    {
        // assume the position to be the previously estimated one
        nextNote.pos = engravedNote.npos;
        
        // apply value dependant distance
        {
            const mpx_t valpos = engravedNote.pos + value_width(nextNote.time - engravedNote.time, *param, *viewport);
            if (nextNote.pos < valpos) nextNote.pos = valpos;
        };
        
        // check minimal distance
        if (nextNote->is(Class::NOTEOBJECT))
        {
            if (engravedNote->is(Class::NOTEOBJECT))
            {
                const mpx_t minpos = engravedNote.pos + w + viewport->umtopx_h(param->min_distance);
                if (nextNote.pos < minpos) nextNote.pos = minpos;
            }
            else
            {
                const mpx_t minpos = engravedNote.pos + w + viewport->umtopx_h(param->nonnote_distance);
                if (nextNote.pos < minpos) nextNote.pos = minpos;
            };
        }
        else
        {
            const mpx_t minpos = engravedNote.pos + w + viewport->umtopx_h(param->default_distance);
            if (nextNote.pos < minpos) nextNote.pos = minpos;
        };
    };
    
    // apply accumulative offset
    if (nextNote->acc_offset)
        nextNote.pos += viewport->umtopx_v((HEAD_HEIGHT(nextNote.staff()) * nextNote->acc_offset) / 1000.0);
    
    // calculate estimated position of the following note
    if (!nextNote->is(Class::NEWLINE))
        calculate_npos(nextNote);
    
    // correct the width of shorter non-note-objects (compared to simultaneous non-note-objects)
    if (nextNote->classtype() == engravedNote->classtype() && !engravedNote->is(Class::NOTEOBJECT)
                                                           && !engravedNote->is(Class::TIMESIG))
    {   // if there are two consequent non-note-objects of the same class (no time-signatures)
        if (engravedNote.npos > nextNote.npos)  // correct the estimated width, being the maximum
            nextNote.npos = engravedNote.npos;  //        of both simultaneously rendered objects
    };
}

// constructor: create a new pick
Pick::Pick(const Score& _score, const EngraverParam& _param, const ViewportParam& _viewport, const Sprites& _sprites, int def_head_height) :
                                            score(&_score),
                                            param(&_param),
                                            viewport(&_viewport),
                                            sprites(&_sprites),
                                            head_height(def_head_height),
                                            cursors(&compare),
                                            next_cursors(&compare),
                                            _dimension(NULL),
                                            _newline(false),
                                            _pagebreak(false),
                                            _newline_time(-1L),
                                            _line_height(-1)
{
    _initialize();  // intialize the cursors to the score's beginning
}

// pop current note from stack and add next note in voice
void Pick::next(mpx_t w)
{
    // remove engraved note
    if (cursors.empty()) return;
    
    VoiceCursorPtr engravedNote(cursors.top());
    cursors.pop();
    insert_next(*engravedNote);
    prepare_next(*engravedNote, w);
}

// reset cursors to the beginning of the score
void Pick::reset()
{
    cursors.clear();        // delete all cursors
    _line_height = -1;      // reset previous line height
    _newline = false;       // reset newline indicator
    _pagebreak = false;     // reset pagebreak indicator
    _newline_time = -1L;    // reset newline timestamp
    _initialize();          // re-initialize cursors to the score's beginning
}

// get cursor of a special voice
const Pick::VoiceCursor& Pick::get_cursor(const Voice& voice) const
{
    for (CQueue::const_iterator i = cursors.begin(); i != cursors.end(); ++i)
        if (&(*i)->voice() == &voice) return **i;
    return *cursors.top();
}

// apply additional distance to all notes at and after a given time
void Pick::add_distance(mpx_t distance, value_t time)
{
    // iterate through the voices
    for (CQueue::iterator i = cursors.begin(); i != cursors.end(); ++i)
    {
        if ((*i)->time >= time) // if the note occurs at or after the insertion
        {
            (*i)->pos += distance;  // move the note
            (*i)->npos += distance; // and all next notes
        }
        else                    // if the insertion is during the note
        {
            (*i)->npos += distance; // move all next notes
        };
    };
}

// apply additional distance to all notes after a given time
void Pick::add_distance_after(mpx_t distance, value_t time)
{
    // iterate through the voices
    for (CQueue::iterator i = cursors.begin(); i != cursors.end(); ++i)
    {
        if ((*i)->time > time)  // if the note occurs after the insertion
        {
            (*i)->pos += distance;  // move the note
            (*i)->npos += distance; // and all next notes
        }
        else                    // if the insertion is during the note
        {
            (*i)->npos += distance; // move all next notes
        };
    };
}

// insert a virtual object (after the current one)
void Pick::insert(const StaffObject& obj)
{
    if (cursors.empty()) return;
    VoiceCursorPtr curptr(new VoiceCursor(*cursors.top()));
    VoiceCursor& cur(*curptr);
    cur.time = cur.ntime;               // set timestamp to current time
    if (obj.is(Class::NOTEOBJECT))      // if we got a note-object
        cur.ntime += static_cast<const NoteObject&>(obj).value();   // increase end time-stamp
    
    if (cur->is(Class::PAGEBREAK))
    {
        cur.pos = viewport->umtopx_h(param->min_distance + static_cast<const Pagebreak&>(*cur).layout.indent);
    }
    else if (cur->is(Class::NEWLINE))
    {
        if (!_newline) _line_height = viewport->umtopx_v(line_height());
        cur.pos = viewport->umtopx_h(param->min_distance + static_cast<const Newline&>(*cur).layout.indent);
        cur.ypos += _line_height;
    }
    else cur.pos = cur.npos;
    
    calculate_npos(cur);
    cur.virtual_obj = StaffObjectPtr(obj.clone());
    cur.inserted = true;
    cur.remaining_duration = -1;
    ++cur;
    ((*cursors.top())->is(Class::NEWLINE) ? next_cursors : cursors).push(curptr);
}

// insert a virtual barline object at a given time
void Pick::insert_barline(const Barline::Style& style)
{
    if (cursors.empty()) return;
    VoiceCursorPtr barline(new VoiceCursor(*cursors.top()));
    barline->time = barline->ntime;
    barline->virtual_obj = StaffObjectPtr(new Barline(style));
    barline->inserted = true;
    barline->remaining_duration = -1;
    cursors.push(barline);
}

// insert a virtual object (changes current cursor)
bool Pick::insert_before(const StaffObject& obj)
{
    return insert_before(obj, cursors.top()->voice());
}

// insert a virtual object (into given voice)
bool Pick::insert_before(const StaffObject& obj, const Voice& voice)
{
    CQueue::iterator cursor;
    for (CQueue::iterator c = cursors.begin(); c != cursors.end(); ++c)
        if (&(*c)->voice() == &voice) {cursor = c; break;};
    
    for (CQueue::iterator c = cursors.begin(); c != cursors.end(); ++c)
        if ((*c)->time > (*cursor)->time) return false;
    
    VoiceCursorPtr vobj(new VoiceCursor(**cursor));
    vobj->ntime = vobj->time;
    if (obj.is(Class::NOTEOBJECT)) vobj->ntime += static_cast<const NoteObject&>(obj).value();
    vobj->virtual_obj = StaffObjectPtr(obj.clone());
    vobj->inserted = true;
    vobj->remaining_duration = -1;
    calculate_npos(*vobj);
    cursors.top()->npos = vobj->npos;
    cursors.push(vobj);
    prepare_next(*cursors.top(), 0);
    return true;
}

// cut the note into two tied notes (given duration of the first)
void Pick::cut(value_t duration)
{
    if (!(*cursors.top())->is(Class::NOTEOBJECT)) return;
    if (static_cast<const NoteObject&>(**cursors.top()).value() <= duration) return;
    
    // cut a chord
    if ((*cursors.top())->is(Class::CHORD))
    {
        // create first part of virtually tied note
        const Chord& chord = static_cast<const Chord&>(**cursors.top());
        Chord* nchord = new Chord(static_cast<const Chord&>(cursors.top()->original()), true);
        nchord->set_value(duration);
        nchord->heads.clear();
        nchord->beam = Chord::BEAM_NONE;
        for (HeadList::const_iterator i = chord.heads.begin(); i != chord.heads.end(); ++i)
        {
            nchord->heads.push_back(HeadPtr(new TiedHead(**i)));
            static_cast<TiedHead&>(*nchord->heads.back()).offset1 = param->tieup_offset1;
            static_cast<TiedHead&>(*nchord->heads.back()).control1 = param->tieup_control1;
            static_cast<TiedHead&>(*nchord->heads.back()).control2 = param->tieup_control2;
            static_cast<TiedHead&>(*nchord->heads.back()).offset2 = param->tieup_offset2;
        };
        cursors.top()->remaining_duration = cursors.top()->ntime - cursors.top()->time - duration;
        cursors.top()->ntime = cursors.top()->time + duration;
        cursors.top()->virtual_obj = StaffObjectPtr(nchord);    // inserted status stays ("false" for first, "true" for following)
        
        // calculate estimated position of the following note
        calculate_npos(*cursors.top());
    }
    
    // cut a rest
    else if ((*cursors.top())->is(Class::REST))
    {
        // create first part of virtually tied note
        Rest* nrest = new Rest(static_cast<const Rest&>(cursors.top()->original()), true);
        nrest->set_value(duration);
        cursors.top()->remaining_duration = cursors.top()->ntime - cursors.top()->time - duration;
        cursors.top()->ntime = cursors.top()->time + duration;
        cursors.top()->virtual_obj = StaffObjectPtr(nrest); // inserted status stays ("false" for first, "true" for following)
        
        // calculate estimated position of the following note
        calculate_npos(*cursors.top());
    };
}

// get the current staff (shifted)
const Staff& Pick::get_staff(int idx_shift) const
{
    if (idx_shift == 0) return cursors.top()->staff();
    
    std::list<Staff>::const_iterator i = score->staves.begin(); // iterator (real)
    std::list<Staff>::const_iterator j = score->staves.begin(); // iterator (offset)
    const Staff& staff = cursors.top()->staff();
    
    // push idx-offset without adding position
    while (idx_shift < 0 && &*i != &staff && i != score->staves.end())
    {
            if (_layout.get(*i).visible) ++idx_shift;
            ++i;
    };
    if (idx_shift < 0) idx_shift = 0;
    
    // iterate until given staff is found (or we run out of staves)
    while ((idx_shift || &*i != &staff) && i != score->staves.end())
    {
        if (idx_shift < 0 && _layout.get(*j).visible) ++idx_shift;
        if (!idx_shift) ++i;
        if (&*++j == &score->staves.back()) break;
    };
    
    // if we did not find the given staff, issue warning
    if (i == score->staves.end())
        log_warn("Unable to find the current staff within the score object. (class: Pick)");
    
    // add staff offset and return
    return *j;
}

// calculate the current staff's offset relative to the line's position (in micrometer)
int Pick::staff_offset(int idx_shift) const
{
    int out = 0;                                                // staff offset
    std::list<Staff>::const_iterator i = score->staves.begin(); // iterator (real)
    std::list<Staff>::const_iterator j = score->staves.begin(); // iterator (offset)
    const Staff& staff = cursors.top()->staff();
    
    // push idx-offset without adding position
    while (idx_shift < 0 && &*i != &staff && i != score->staves.end())
    {
            if (_layout.get(*i).visible) ++idx_shift;
            ++i;
    };
    if (idx_shift < 0) idx_shift = 0;
    
    // iterate until given staff is found (or we run out of staves)
    while ((idx_shift || &*i != &staff) && i != score->staves.end())
    {
        if (_layout.get(*j).visible)
        {
            out += ((j->offset_y + _layout.get(*j).distance) * HEAD_HEIGHT(*j)) / 1000  // add staff offset
                +  (j->line_count - 1) * HEAD_HEIGHT(*j);                               // add staff height
            if (idx_shift < 0) ++idx_shift;
        };
        if (!idx_shift) ++i;
        if (&*++j == &score->staves.back()) break;
    };
    
    // if we did not find the given staff, return
    if (i == score->staves.end())
    {
        log_warn("Unable to find the current staff within the score object. (class: Pick)");
        return out;
    };
    
    // add staff offset and return
    return out + ((j->offset_y + _layout.get(*j).distance) * HEAD_HEIGHT(*j)) / 1000;
}

// calculate the staff's offset relative to the line's position (in micrometer)
int Pick::staff_offset(const Staff& staff) const
{
    int out = 0;                                                // staff offset
    std::list<Staff>::const_iterator i = score->staves.begin(); // iterator
    
    while (i != score->staves.end() && &*i != &staff)   // iterate until given staff is found (or we run out of staves)
    {
        if (_layout.get(*i).visible)
            out += ((i->offset_y + _layout.get(*i).distance) * HEAD_HEIGHT(*i)) / 1000  // add staff offset
                +  (i->line_count - 1) * HEAD_HEIGHT(*i);                               // add staff height
        ++i;
    };
    
    // if we did not find the given staff, return
    if (i == score->staves.end())
    {
        log_warn("Unable to find the current staff within the score object. (class: Pick)");
        return out;
    };
    
    // add staff offset and return
    return out + ((staff.offset_y + _layout.get(staff).distance) * HEAD_HEIGHT(staff)) / 1000;
}

// calculate the complete line height (in micrometer)
int Pick::line_height() const
{
    int out = 0; // staff offset
    for (std::list<Staff>::const_iterator i = score->staves.begin(); i != score->staves.end(); ++i)
    {
        out += ((i->offset_y + _layout.get(*i).distance) * HEAD_HEIGHT(*i)) / 1000  // add staff offset
            +  (i->line_count - 1) * HEAD_HEIGHT(*i);                               // add staff height
    };
    return out;
}

// check if the current cursor is last in voice
bool Pick::eov() const
{
    if (cursors.empty()) return true;
    const Voice* const voice = &cursors.top()->voice();
    for (CQueue::const_iterator i = ++cursors.begin(); i != cursors.end(); ++i)
        if (&(*i)->voice() == voice) return false;
    for (CQueue::const_iterator i = next_cursors.begin(); i != next_cursors.end(); ++i)
        if (&(*i)->voice() == voice) return false;
    return true;
}

// peek at the next note in the voice (NULL if not there)
const Pick::VoiceCursor* Pick::peek(const Voice& v) const
{
    for (CQueue::const_iterator i = cursors.begin(); i != cursors.end(); ++i)
        if (&(*i)->voice() == &v) return &**i;
    for (CQueue::const_iterator i = next_cursors.begin(); i != next_cursors.end(); ++i)
        if (&(*i)->voice() == &v) return &**i;
    return NULL;
}

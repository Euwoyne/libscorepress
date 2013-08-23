
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

#include <iostream>         // std::cout

#include "user_cursor.hh"   // UserCursor
using namespace ScorePress;


//
//     class UserCursor
//    ==================
//
// This cursor class iterates simultaneously the score and the corresponding
// plate. It has a graphical representation, which can be rendered by a
// "Press" instance. It can be manipulated by directions and graphical
// coordinates. (Modification methods implemented in child-class "EditCursor")
//


inline int _round(const double d) {return static_cast<mpx_t>(d + 0.5);}
inline value_t get_value(const StaffObject& obj) {return (obj.is(Class::NOTEOBJECT)) ? static_cast<const NoteObject&>(obj).value() : value_t(0);}

// exception classes
UserCursor::Error::Error(const std::string& msg) : ScorePress::Error(msg) {}
UserCursor::NotValidException::NotValidException()
        : Error("Unable to dereference invalid user-cursor.") {}
UserCursor::NotValidException::NotValidException(const std::string& msg) : Error(msg) {}
UserCursor::NoScoreException::NoScoreException()
        : NotValidException("No score is set for this user-cursor.") {}
UserCursor::InvalidMovement::InvalidMovement()
        : Error("Unable to move the user-cursor in the desired direction.") {}
UserCursor::InvalidMovement::InvalidMovement(const std::string& dir)
        : Error("Unable to move the user-cursor in the desired direction (" + dir + ").") {}


//     voice-cursor movement
//    -----------------------

// to the previous note (fails, if "!has_prev()")
void UserCursor::VoiceCursor::prev() throw(InvalidMovement, Error)
{
    // check validity of movement
    if (!has_prev()) throw InvalidMovement("prev");
    
    // set note
    --note;                         // goto previous note
    ntime = time;                   // set end-time
    time -= get_value(*note);       // calculate time
    
    // set on-plate note
    while ((pnote->at_end() || pnote->is_inserted() || &*pnote->note != &*note) && pnote != pvoice->notes.begin())
        --pnote;
    
    // check, if we got the note
    if (!pnote->at_end() && (&*pnote->note != &*note || pnote->is_inserted()))
        throw Error("Cannot find previous note on-plate.");
}

// to the next note (fails, if "at_end()")
void UserCursor::VoiceCursor::next() throw(InvalidMovement, Error)
{
    // check validity of movement
    if (at_end()) throw InvalidMovement("next");
    
    // set on-plate note
    ++note;
    do ++pnote;
    while (pnote->is_inserted() && !pnote->at_end());
    
    // set note
    time = ntime;                   // set time
    if (!pnote->at_end())           // if we are not at the end
        ntime += get_value(*note);      // calculate end-time
    
    // check, if we got the note
    if (&*pnote->note != &*note || pnote->is_inserted())
        throw Error("Cannot find next note on-plate.");
}


//     voice-cursor miscellaneous methods
//    ------------------------------------

// are the objects simultaneous (and equal type)
bool UserCursor::VoiceCursor::is_simultaneous(const VoiceCursor& cur) const
{
    if (time != cur.time) return false;                                 // unequal time -> FALSE
    if (at_end() && cur.at_end()) return true;                          // both at end -> TRUE
    if (at_end() || cur.at_end()) return true;                          // only one at end -> TRUE
    if (note->is(Class::NOTEOBJECT) && cur.note->is(Class::NOTEOBJECT)) // equal time/note -> TRUE
        return true;
    return (note->classtype() == cur.note->classtype());                // equal time/type -> TRUE
}

// is the given object during this one?
bool UserCursor::VoiceCursor::is_during(const VoiceCursor& cur) const
{
    if (is_simultaneous(cur)) return true;                  // simultaneous -> TRUE
    if (cur.at_end()) return false;                         // non-simultaneous/at-end -> FALSE
    if (!cur.note->is(Class::NOTEOBJECT)) return false;     // non-simultaneous/non-note -> FALSE
    //if (time >= cur.time && time < cur.ntime) return true;  // note/during -> TRUE
    //return false;                                           // other -> FALSE
    return (time == cur.time || (time > cur.time && time < cur.ntime));
}

// is this object rendered after the given one?
bool UserCursor::VoiceCursor::is_after(const VoiceCursor& cur) const
{
    if (time == cur.time)   // rendered in order: newline -> clef -> key -> time-signature -> note
    {
        if (at_end() && cur.at_end()) return false;         // both at end -> FALSE
        if (cur.at_end()) return true;                      // end before everything -> TRUE
        if (at_end()) return false;                         // end before everything -> FALSE
        if (note->classtype() == cur.note->classtype() ||   // equal type -> FALSE
            (note->is(Class::TIMESIG) && cur.note->is(Class::CUSTOMTIMESIG)) ||
            (note->is(Class::CUSTOMTIMESIG) && cur.note->is(Class::TIMESIG)) ||
            (note->is(Class::NOTEOBJECT) && cur.note->is(Class::NOTEOBJECT)))
                return false;
        if (note->is(Class::NOTEOBJECT)) return true;       // note after all other -> TRUE
        if (cur.note->is(Class::NOTEOBJECT)) return false;  //  "     "    "    "   -> FALSE
        if (note->is(Class::TIMESIG)) return true;          // time-signature -> TRUE
        if (cur.note->is(Class::TIMESIG)) return false;     //  "       "     -> FALSE
        if (note->is(Class::KEY)) return true;              // key -> TRUE
        if (cur.note->is(Class::KEY)) return false;         //  "  -> FALSE
        if (note->is(Class::CLEF)) return true;             // clef -> TRUE
        if (cur.note->is(Class::CLEF)) return false;        //  "   -> FALSE
        if (note->is(Class::NEWLINE)) return true;          // newline -> TRUE
        if (cur.note->is(Class::NEWLINE)) return false;     //    "    ->  FALSE
        
        return false;
    };
    return (time > cur.time);   // compare time of non-simultaneous objects
}

// is this object rendered before the given one?
bool UserCursor::VoiceCursor::is_before(const VoiceCursor& cur) const
{
    if (time == cur.time)   // rendered in order: newline -> clef -> key -> time-signature -> note
    {
        if (at_end() && cur.at_end()) return false;         // both at end -> FALSE
        if (at_end()) return true;                          // end before everything -> TRUE
        if (cur.at_end()) return false;                     // end before everything -> FALSE
        if (note->classtype() == cur.note->classtype() ||   // equal type -> FALSE
            (note->is(Class::TIMESIG) && cur.note->is(Class::CUSTOMTIMESIG)) ||
            (note->is(Class::CUSTOMTIMESIG) && cur.note->is(Class::TIMESIG)) ||
            (note->is(Class::NOTEOBJECT) && cur.note->is(Class::NOTEOBJECT)))
                return false;
        if (note->is(Class::NEWLINE)) return true;          // newline before all other -> TRUE
        if (cur.note->is(Class::NEWLINE)) return false;     //    "      "     "    "   -> FALSE
        if (note->is(Class::CLEF)) return true;             // clef -> TRUE
        if (cur.note->is(Class::CLEF)) return false;        //  "   -> FALSE
        if (note->is(Class::KEY)) return true;              // key -> TRUE
        if (cur.note->is(Class::KEY)) return false;         //  "  -> FALSE
        if (note->is(Class::TIMESIG)) return true;          // time-signature -> TRUE
        if (cur.note->is(Class::TIMESIG)) return false;     //  "       "     -> FALSE
        if (note->is(Class::NOTEOBJECT)) return true;       // note-object -> TRUE
        if (cur.note->is(Class::NOTEOBJECT)) return false;  //  "     "    -> FALSE
        
        return false;
    };
    return (time < cur.time);   // compare time of non-simultaneous objects
}


//     user-cursor methods
//    ---------------------

// find a voice's cursor
std::list<UserCursor::VoiceCursor>::iterator UserCursor::find(const Voice& voice)
{
    // iterate all voices
    for (std::list<VoiceCursor>::iterator i = vcursors.begin(); i != vcursors.end(); ++i)
    {
        if (&i->note.voice() == &voice) return i;   // on find, return
    };
    return vcursors.end();  // if nothing is found, return invalid
}

// find a voice's cursor (const version)
std::list<UserCursor::VoiceCursor>::const_iterator UserCursor::find(const Voice& voice) const
{
    // iterate all voices
    for (std::list<VoiceCursor>::const_iterator i = vcursors.begin(); i != vcursors.end(); ++i)
    {
        if (&i->note.voice() == &voice) return i;   // on find, return
    };
    return vcursors.end();  // if nothing is found, return invalid
}

// set "VoiceCursor" plate data (helper function for "prepare_voice" and EditCursor::reengrave)
void UserCursor::prepare_plate(VoiceCursor& newvoice, Plate::pVoice& pvoice)
{
    // calculate on-plate data
    newvoice.pnote = pvoice.notes.begin();      // initialize on-plate begin
    newvoice.pvoice = &pvoice;                  // set voice
    newvoice.active = true;                     // set cursor to active (updated below in "prepare_vocies")
    newvoice.time = pvoice.time;                // initialize time
    
    // search for correct begin
    while (!newvoice.pnote->at_end() && newvoice.pnote->is_inserted())
    {
        newvoice.time += get_value(newvoice.pnote->get_note());
        ++newvoice.pnote;
    };
    
    // set ntime
    newvoice.ntime = newvoice.time;
    if (!newvoice.pnote->at_end() && !newvoice.pnote->is_inserted())
        newvoice.ntime += get_value(newvoice.pnote->get_note());
}

// set "VoiceCursor" data (helper function for "prepare_voices")
bool UserCursor::prepare_voice(VoiceCursor& newvoice, Plate::pVoice& pvoice)
{
    // calculate note (casts away const, but not unexpected, since this object got a non-const reference to the score)
    if (pvoice.begin.is_main())
        newvoice.note.set(const_cast<Staff&>(pvoice.begin.staff()));
    else
        newvoice.note.set(const_cast<Staff&>(pvoice.begin.staff()),
                          const_cast<SubVoice&>(static_cast<const SubVoice&>(pvoice.begin.voice())));
    
    // search for the front note
    while (!newvoice.note.at_end() && newvoice.note != pvoice.begin)
            ++newvoice.note;
    if (newvoice.note != pvoice.begin)      // on failing the search
    {                                           // issue warning
        log_warn("Unable to find on-plate voice front in score. (class: UserCursor)");
        return false;                           // return fail
    };
    
    // calculate layout (search previous newline)
    newvoice.layout = newvoice.note;
    if (!newvoice.layout.has_prev())
    {
        newvoice.layout.reset();
    }
    else if (!(--newvoice.layout)->is(Class::NEWLINE))
    {
        newvoice.layout.reset();
        log_warn("Unable to find layout object for voice. (class: UserCursor)");
    };
    
    // prepare on-plate data
    prepare_plate(newvoice, pvoice);
    
    // return success
    return true;
}

// set all voice-cursors to the beginning of the current line
void UserCursor::prepare_voices()
{
    // clear current data
    vcursors.clear();
    cursor = vcursors.end();
    
    // prepare all main-voices of the current line
    for (std::list<Staff>::iterator staff = score->score.staves.begin(); staff != score->score.staves.end(); ++staff)
    {
        // prepare voice
        std::list<Plate::pVoice>::iterator pvoice = line->get_voice(*staff);    // get staff on the plate
        if (pvoice == line->voices.end()) continue;                             // if the staff is not on the plate, we do not need a cursor
        vcursors.push_back(VoiceCursor());                                      // add cursor
        
        if (!prepare_voice(vcursors.back(), *pvoice))   // prepare the voice cursor
            vcursors.pop_back();                        //     if this fails, remove the voice
    };
    cursor = vcursors.begin();  // initialize cursor to first main-voice
    
    // prepare all sub-voices of the current line (expecting all main-voices to exist)
    std::list<VoiceCursor>::iterator newvoice;  // on-plate voice
    for (std::list<Plate::pVoice>::iterator i = line->voices.begin(); i != line->voices.end(); ++i)
    {
        // check voice
        if (find(i->begin.voice()) != vcursors.end()) continue; // if the voice does exist already, ignore
        if (i->begin.is_main())                                 // if the voice is not a sub-voice...
        {                                                       // ...something is wrong
            log_warn("Unable to add a misplaced MainVoice to cursor. (class: UserCursor)");
            continue;
        };
        
        // add new voice (at the correct position)
        newvoice = find(*static_cast<const SubVoice&>(i->begin.voice()).parent);    // find parent
        if (static_cast<const SubVoice&>(i->begin.voice()).on_top)  // if on top
            newvoice = vcursors.insert(newvoice, VoiceCursor());    //     insert new voice above parent
        else                                                        // otherwise
            newvoice = vcursors.insert(++newvoice, VoiceCursor());  //     insert below the parent
        
        // prepare the voice cursor
        if (!prepare_voice(*newvoice, *i))                      // if this fails,
            vcursors.pop_back();                                //     remove the voice
        else if (cursor != vcursors.end())                      // if we got a main-voice
            newvoice->active = newvoice->is_during(*cursor);    //     choose active state
    };
    
    // set cursor, if necessary (i.e. if there are no main-voices)
    if (cursor == vcursors.end()) cursor = vcursors.begin();
}

// move the voice-cursors to the corresponding position within the currently referenced voice
// TODO: empty voices should line up with the corresponding noteobject
//     :     not with non-noteobjects with the same timestamp
void UserCursor::update_voices()
{
    for (std::list<VoiceCursor>::iterator i = vcursors.begin(); i != vcursors.end(); ++i)
    {
        if (i == cursor)
            continue;               // ignore moved cursor
        
        i->active = true;           // initialize at active state
        
        while (i->is_after(*cursor))    // move backward if necessary
        {                               // (set inactive if movement is impossible)
            if (!(i->active = i->has_prev())) break;
            i->prev();
        };
        
        if (i->active |= i->is_during(*cursor)) // check, if the voices begin is simultaneous with the cursor
        {
            while (i->is_before(*cursor))       // move foreward if necessary
            {
                if (i->at_end())                    // if movement is impossible
                {
                    i->active = (i->ntime >= cursor->time); // check, whether we can jump to the cursor (from the end)
                    break;
                };
                i->next();
            };
        };
    };
}

// calculate the horizontal position for the given cursor (position right of the referenced object; fast)
mpx_t UserCursor::fast_x(const VoiceCursor& cur) const throw(NotValidException)
{
    // calculate horizontal position from the note's "gphBox"
    if (!cur.pvoice) throw NotValidException();
    return cur.pnote->gphBox.right();
}

// calculate the horizontal position for the given cursor (graphical cursor position, regarding simultaneous notes)
mpx_t UserCursor::graphical_x(const VoiceCursor& cur) const throw(NotValidException)
{
    // calculate horizontal position from the note's "gphBox"
    if (!cur.pvoice) throw NotValidException();
    mpx_t x = cur.pnote->gphBox.pos.x;
    
    // search for leftmost position (within staff)
    for (std::list<VoiceCursor>::const_iterator i = vcursors.begin(); i != vcursors.end(); ++i)
    {
        if (&i->note.staff() == &cur.note.staff() && i->active && !i->at_end() && x > i->pnote->gphBox.pos.x)
            x = i->pnote->gphBox.pos.x;
    };
    
    return x;
}

// set the cursor near the given x coordinate; rough search (in line)
void UserCursor::set_x_rough(const mpx_t x)
{
    prepare_voices();   // reset positions
    
    // get current x position
    mpx_t newx = fast_x();  // current x position
    bool run = true;        // break checker
    
    while (newx < x && run) // go foreward until we are right of wanted x
    {
        if (!at_end()) next();  // step foreward (in voice, if possible)
        else                    // if not possible
        {                       // check following voices
            while (has_next_voice() && at_end()) next_voice();
            if (at_end())           // if we can't foreward anywhere there
            {                       // check previous voices
                while (has_prev_voice() && at_end()) prev_voice();
                run = false;            // quit searching
            };
        };
        newx = fast_x();        // calculate new "newx"
    };
}

// set the cursor near the given x coordinate; fine adjustment (in voice)
void UserCursor::set_x_voice(const mpx_t x)
{
    // get current x position
    mpx_t newx = fast_x();  // current x position
    
    if (!((newx > x && has_prev()) || (newx < x && !at_end())))    // if no movement is necessary
    {
        return;             // exit
    };
    
    while (newx > x && has_prev())  // go backwards until we are left of wanted x
    {
        prev();             // step backwards (in voice)
        newx = fast_x();    // calculate new "newx"
    };
    while (newx < x && !at_end())  // go foreward until we are right of wanted x
    {
        next();             // step foreward (in voice)
        newx = fast_x();    // calculate new "newx"
    };
}

// constructor (initializing the cursor at the beginning of the given score)
UserCursor::UserCursor(Document& _document, PageSet& _pageset)
    : document(&_document), pageset(&_pageset), score(NULL), plateinfo(NULL) {}

// initialize the cursor at the beginning of a given score
void UserCursor::set_score(Document::Score& _score) throw(Error)
{
    score = &_score;                                    // set score reference
    page = pageset->get_page(_score.start_page);        // get first page
    if (page == pageset->pages.end()) goto on_error;    // skip to error handling
    {
    const std::list<PageSet::PlateInfo>::iterator pi = page->get_plate_by_score(_score.score);
    if (pi == page->plates.end()) goto on_error;        // skip to error handling
    plateinfo = &*pi;                                   // get plate information
    }
    line = plateinfo->plate.lines.begin();              // get first line
    prepare_voices();                                   // prepare voice-cursors
    return;                                             // sucessfully finish
    
    // error handling (reset data)
 on_error:
    score = NULL;
    plateinfo = NULL;
    vcursors.clear();
    cursor = vcursors.end();
    throw Error("Unable to find the given score in the page-set.");
}

// set cursor to graphical position (on the current page)
void UserCursor::set_pos(Position<mpx_t> pos, const ViewportParam& viewport)
{
    pos.x -= pageset->page_layout.margin.left;
    pos.y -= pageset->page_layout.margin.top;
    
    // find PLATE
    PageSet::PlateInfo* new_pinfo = NULL;
    if (plateinfo->dimension.contains(pos))     // if the current plate is the correct one
    {
        new_pinfo = plateinfo;                  //     just use this one
    }
    else                                        // otherwise
    {                                           //     search all plates for a match
        for (std::list<PageSet::PlateInfo>::iterator p = page->plates.begin(); p != page->plates.end(); ++p)
        {
            if (p->dimension.contains(pos)) {new_pinfo = &*p; break;};
        };
    };
    if (!new_pinfo) return;                     // if no plate is found at the position, abort
    pos -= new_pinfo->dimension.position;       // calculate position relative to plate
    
    // find on-plate LINE
    bool check = false;                         // success indicator
    for (std::list<Plate::pLine>::iterator l = new_pinfo->plate.lines.begin(); l != new_pinfo->plate.lines.end(); ++l)
    {                                           // search all lines for a match
        if (l->gphBox.contains(pos)) {line = l; check = true; break;};
    };
    if (!check) return;                         // if no line is found at the position, abort
    plateinfo = new_pinfo;                      // set new plateinfo
    
    // find STAFF and approximate horizontal position
    const Staff* staff = NULL;                  // staff reference
    mpx_t head_height = 0;                      // head height
    mpx_t bottom_diff = 0, old_bottom_diff = 0; // distance from staff bottom (current/last)
    int   line_cnt = 0;                         // staff line count
    prepare_voices();                           // prepare cursors for the chosen line
    for (std::list<VoiceCursor>::iterator i = vcursors.begin(); i != vcursors.end(); ++i)
    {                                           // iterate the voices
        head_height = viewport.umtopx_v(i->pvoice->begin.staff().head_height);  // calculate head-height
        line_cnt    = i->pvoice->begin.staff().line_count;                      // get line count
        bottom_diff = pos.y - i->pvoice->basePos.y - line_cnt * head_height;    // calculate distance from bottom line
        if (bottom_diff < 0)                        // as soon as we finally get below the given position
        {                                           // (we search from the top down)
            staff = &i->pvoice->begin.staff();          // get staff (the one immediately below)
            if (   i != vcursors.begin()                // if this is not the topmost voice...
                && old_bottom_diff < i->pvoice->basePos.y - head_height - pos.y)
            {                                           // ...and the previous staff is closer to the position
                staff = &(--i)->pvoice->begin.staff();                  // use previous staff
                head_height = viewport.umtopx_v(staff->head_height);    // reset head-height
                line_cnt    = staff->line_count;                        // reset line-count
            };
            break;                                      // done
        };
        old_bottom_diff = bottom_diff;              // if we continue the search, save the distance
    };
    if (!staff) staff = &vcursors.back().pvoice->begin.staff(); // if none is found, default to first
    set_x_rough(pos.x);                                         // roughly set the horizontal position
    cursor = vcursors.begin();                                  // set cursor to the calculated staff
    while (cursor != vcursors.end() && &cursor->pvoice->begin.staff() != staff) ++cursor;
    if (cursor == vcursors.end()) --cursor;                     // if none is found, default to last
    set_x_voice(pos.x);                                         // improve position proximity within the voice
    
    // find VOICE and correct position within the voice
    pos.y -= cursor->pvoice->basePos.y;                         // consider position relative to the staff
    const int voice_cnt = staff_voice_count();                  // get voice count
    int voice_idx = (  (pos.y + head_height) * voice_cnt)       // calculate voice index
                     / (head_height * (line_cnt + 1));          // (evenly map vertical space around the staff to voices)
    if (voice_idx >= voice_cnt) voice_idx = voice_cnt - 1;      // handle overflow
    if (voice_idx < 0) voice_idx = 0;                           //                 and underflow
    for (int i = 0; i < voice_idx && has_next_voice(); ++i)     // move to the calculated voice
        next_voice();
    set_x_voice(pos.x);                                         // final fine position adjustment within the voice
    update_voices();                                            // and set other voices
}


//  access methods
// ----------------

// return the voice
const Voice& UserCursor::get_voice() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->note.voice();
}

// return the staff
const Staff& UserCursor::get_staff() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->note.staff();
}

// return the on-plate voice
const Plate::pVoice& UserCursor::get_pvoice() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return *cursor->pvoice;
}

// return the score-cursor
const Cursor& UserCursor::get_cursor() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->note;
}

// return the on-plate note
const Plate::pNote& UserCursor::get_platenote() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return *cursor->pnote;
}

// return the line layout
const Newline& UserCursor::get_layout() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->get_layout();
}

// return the current time-stamp
value_t UserCursor::get_time() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->time;
}

// check, if the cursor is at the end of the voice
bool UserCursor::at_end() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->at_end();
}

// return the index of the current voice
size_t UserCursor::voice_index() const throw (NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    size_t idx = 0;
    for (std::list<VoiceCursor>::const_iterator i = vcursors.begin(); i != vcursors.end(); ++i)
    {
        if (i == cursor) return idx;
        if (i->active) ++idx;
    };
    throw NotValidException("Cannot find current Voice-Cursor.");
}

// return the index of the current voice (in staff)
size_t UserCursor::staff_voice_index() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    size_t idx = 0;
    for (std::list<VoiceCursor>::const_iterator i = vcursors.begin(); i != vcursors.end(); ++i)
    {
        if (i == cursor) return idx;
        if (&i->pvoice->begin.staff() == &cursor->pvoice->begin.staff() && i->active) ++idx;
    };
    throw NotValidException("Cannot find current Voice-Cursor.");
}

// return the number of voices (in staff)
size_t UserCursor::staff_voice_count() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    size_t cnt = 0;
    for (std::list<VoiceCursor>::const_iterator i = vcursors.begin(); i != vcursors.end(); ++i)
    {
        if (&i->pvoice->begin.staff() == &cursor->pvoice->begin.staff() && i->active) ++cnt;
    };
    return cnt;
}


//  direction checkers
// --------------------

// check, if the cursor can be decremented
bool UserCursor::has_prev() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->has_prev();
}

// check, if the cursor can be incremented
bool UserCursor::has_next() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->has_next();
}

// check, if the voice-cursor can be decremented
bool UserCursor::has_prev_voice() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    if (cursor == vcursors.begin()) return false;   // first voice has no previous
    std::list<VoiceCursor>::iterator i = cursor;    // search active voice before the current one
    for (--i; i != vcursors.begin(); --i)
    {
        if (i->active) return true;
    };
    return (i->active);     // test first voice (which is the last one to test)
}

// check, if the voice-cursor can be incremented
bool UserCursor::has_next_voice() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    std::list<VoiceCursor>::iterator i = cursor;    // search active voice after the current one
    for (++i; i != vcursors.end(); ++i)
    {
        if (i->active) return true;
    };
    return false;
}

// check, if there is a previous line
bool UserCursor::has_prev_line() const throw(NotValidException)
{
    if (!ready()) throw NotValidException();
    if (line != plateinfo->plate.lines.begin())     // if there is a previous line on the page
        return true;                                //     return "true"
    std::list<PageSet::pPage>::iterator p = page;   // otherwise search the previous pages
    while (p != pageset->pages.begin())
    {
        --p;
        if (p->get_plate_by_score(score->score) != p->plates.end()) return true;
    };
    return false;
}

// check, if there is a next line
bool UserCursor::has_next_line() const throw(NotValidException)
{
    if (!ready()) throw NotValidException();
    if (line != --plateinfo->plate.lines.end())     // if there is a next line on the page
        return true;                                //     return "true"
    std::list<PageSet::pPage>::iterator p = page;   // otherwise search the next pages
    while (++p != pageset->pages.end())
    {
        if (p->get_plate_by_score(score->score) != p->plates.end()) return true;
    };
    return false;
}


//  movement methods
// ------------------

// to the previous note
void UserCursor::prev() throw(NotValidException, InvalidMovement)
{
    if (!has_prev()) throw InvalidMovement("prev");
    cursor->prev();
    update_voices();
}

// to the next note
void UserCursor::next() throw(NotValidException, InvalidMovement)
{
    if (at_end()) throw InvalidMovement("next");
    cursor->next();
    update_voices();
}

// to the previous voice
void UserCursor::prev_voice() throw(NotValidException, InvalidMovement)
{
    // validity check
    if (!ready()) throw NotValidException();
    if (cursor == vcursors.begin()) throw InvalidMovement("prev_voice");
    
    // search the previous voices for the next active voice
    std::list<VoiceCursor>::iterator i = cursor;
    for (--i; i != vcursors.begin(); --i)
    {
        if (i->active)  // if we found an active voice
        {
            cursor = i;     // success
            update_voices();
            return;
        };
    };
    if (i->active)  // the first voice has to be checked sepeerately
    {
        cursor = i;     // success
        update_voices();
        return;
    };
    throw InvalidMovement("prev_voice");
}

// to the next voice
void UserCursor::next_voice() throw(NotValidException, InvalidMovement)
{
    // validity check
    if (!ready()) throw NotValidException();
    
    // search the next voices for the next active voice
    for (std::list<VoiceCursor>::iterator i = ++cursor; i != vcursors.end(); ++i)
    {
        if (i->active)  // if we found an active voice
        {
            cursor = i;     // success
            update_voices();
            return;
        };
    };
    throw InvalidMovement("next_voice");
}

// to the previous line
void UserCursor::prev_line() throw(NotValidException, InvalidMovement)
{
    if (!ready()) throw NotValidException();    // validity check
    mpx_t x = fast_x();                        // save current position
    
    // select previous line
    if (line == plateinfo->plate.lines.begin()) // if it is on the previous page
    {
        // search previous pages
        std::list<PageSet::pPage>::iterator p = page;                       // page iterator
        std::list<PageSet::PlateInfo>::iterator pinfo = p->plates.end();    // page info
        
        while (p != pageset->pages.begin())
        {
            --p;
            pinfo = p->get_plate_by_score(*plateinfo->score);   // check for score
            if (pinfo != p->plates.end()) break;                // return on the first page featuring this score
        };
        
        // if there is no previous page with this score, throw
        if (pinfo == p->plates.end()) throw InvalidMovement("prev_line");
        
        // set data
        page = p;
        plateinfo = &*pinfo;
        line = plateinfo->plate.lines.begin();
    }
    else --line;    // if there is a previous line on the plate, just use it
    
    // reset x position from the old line
    set_x_rough(x);                         // rough positioning
    while (has_next_voice()) next_voice();  // move to the last voice
    set_x_voice(x);                         // fine position adjustment
    update_voices();                        // set other voices
}

// to the next line
void UserCursor::next_line() throw(NotValidException, InvalidMovement)
{
    if (!ready()) throw NotValidException();    // validity check
    mpx_t x = fast_x();                         // save current position
    
    // select next line
    if (++line == plateinfo->plate.lines.end()) // if it is on the next page
    {
        // search previous pages
        std::list<PageSet::pPage>::iterator p = page;                       // page iterator
        std::list<PageSet::PlateInfo>::iterator pinfo = p->plates.end();    // page info
        
        while (++p != pageset->pages.end())
        {
            pinfo = p->get_plate_by_score(*plateinfo->score);   // check for score
            if (pinfo != p->plates.end()) break;                // return on the first page featuring this score
        };
        
        // if there is no previous page with this score, throw
        if (pinfo == p->plates.end()) throw InvalidMovement("next_line");
        
        // set data
        page = p;
        plateinfo = &*pinfo;
        line = --plateinfo->plate.lines.end();
    };
    
    // reset x position from the old line
    set_x_rough(x);                         // rough positioning
    while (has_prev_voice()) prev_voice();  // move to the first voice
    set_x_voice(x);                         // fine position adjustment
    update_voices();                        // set other voices
}

// to the beginning of the line
void UserCursor::home() throw(NotValidException)
{
    if (!ready()) throw NotValidException();
    Voice& voice = cursor->note.voice();    // save voice
    Staff& staff = cursor->note.staff();    // save staff
    prepare_voices();                       // reset all voice cursors
    cursor = vcursors.end();                // reset cursor
    
    // restore previous voice
    for (std::list<VoiceCursor>::iterator i = vcursors.begin(); i != vcursors.end(); ++i)
    {
        if (&i->pvoice->begin.voice() == &voice && i->active)   // if we got the voice
        {
            cursor = i;     // set cursor
            break;          // quit searching
        };
        if (&i->pvoice->begin.staff() == &staff && i->active)   // if we got the staff
        {
            cursor = i;     // set cursor (perhaps we will find the voice)
        };
    };
    if (cursor == vcursors.end())   // if we did not find the voice,
        cursor = vcursors.begin();  // default to first one
}

// to the beginning of the voice (in line)    
void UserCursor::home_voice() throw(NotValidException)
{
    if (!ready()) throw NotValidException();
    while (cursor->has_prev()) prev();
}

// to the end of the line
void UserCursor::end() throw(NotValidException)
{
    if (!ready()) throw NotValidException();
    
    value_t end_time = 0;                                   // end-time of the latest voice
    std::list<VoiceCursor>::iterator cur = vcursors.end();  // corresponding voice-cursor
    
    // search for the voice with the latest end
    for (std::list<Plate::pVoice>::iterator pvoice = line->voices.begin(); pvoice != line->voices.end(); ++pvoice)
    {
        // if the voice ends later than the one last saved (or the current voice ends simultneously)
        if (pvoice->end_time > end_time || (pvoice->end_time == end_time && &*pvoice == cursor->pvoice))
        {
            cur = find(pvoice->begin.voice());  // search for the voice cursor
            if (cur != vcursors.end())          // if it exists...
            {
                cursor = cur;                       // use
                end_time = pvoice->end_time;        // and remember end-time
            };
        };
    };
    
    // move to the end
    cursor->active = true;              // set cursor active
    update_voices();                    // update other voice cursors
    while (!cursor->at_end()) next();   // move to the end
}

// to the end of the voice (in line)
void UserCursor::end_voice() throw(NotValidException)
{
    if (!ready()) throw NotValidException();
    while (!cursor->at_end()) next();
}

// calculate current staff context (without local accidentals)
StaffContext UserCursor::get_staff_context() const throw(NotValidException)
{
    if (!ready()) throw NotValidException();
    
    // get the context at the lines begin
    StaffContext out_ctx;
    if (line != plateinfo->plate.lines.begin()) // get the end-context of the previous line
    {
        std::list<Plate::pLine>::iterator l(line); --l;
        std::map<const Staff*, StaffContext>::iterator ctx = l->staffctx.find(&cursor->pvoice->begin.staff());
        if (ctx != l->staffctx.end()) out_ctx = ctx->second;
    };  // (if we cannot find this staff within the previous line, empty context is correct)
    
    // get the staff's main voice
    const_Cursor cur(cursor->note);         // search for the main voice of the current staff
    if (!cur.is_main())
    {
        for (std::list<VoiceCursor>::const_iterator c = vcursors.begin(); c != vcursors.end(); ++c)
        {
            // search the main voice within the same staff
            if (&c->note.staff() == &cur.staff() && c->note.is_main())
            {
                cur = c->note;  // save cursor
                break;          // stop search
            };
        };
    }
    if (!cur.is_main()) return out_ctx; // if we cannot find a main-voice, last known context is correct
    if (cur.at_end())                   // if we are at end
    {                                   //     goto previous note, if possible
        if (!cur.has_prev()) return out_ctx;
        --cur;
    };
    
    // apply all (staff-)context modifying objects up to the current cursor
    bool got_clef = false;
    bool got_key  = false;
    while (!got_clef || !got_key)
    {
        // modify the context
        if (!got_clef && cur->is(Class::CLEF))           // by clef
        {
            out_ctx.modify(static_cast<const Clef&>(*cur));
            got_clef = true;
        }
        else if (cur->is(Class::KEY))       // by key
        {
            out_ctx.modify(static_cast<const Key&>(*cur));
            got_key = true;
        };
        
        // move to the previous note
        if (!cur.has_prev()) break;
        --cur;
    };
    
    // return calculated context
    return out_ctx;
}


//  graphical representation
// --------------------------

// vertical position
mpx_t UserCursor::graphical_y(const ViewportParam& viewport) const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    
    // calculate vertical position and height
    mpx_t y = cursor->pvoice->basePos.y;        // initialize at the staff's top line
    mpx_t h = viewport.umtopx_v(cursor->pvoice->begin.staff().head_height); // initialize with head-height
    const size_t voice_cnt = staff_voice_count();
    const unsigned int line_cnt = cursor->pvoice->begin.staff().line_count;
    
    if (voice_cnt <= line_cnt + 1)
    {   // if we do not have too much voices
        y += (static_cast<int>(staff_voice_index()) - 1) * h;   // vertical position
    }
    else
    {   // if there are too much voices for the normal algorithm
        // just divide the staff's height into enough segments, and let the cursor have the height of one head
        y += _round((static_cast<int>(staff_voice_index()) - 1) * ((h * (line_cnt + 1.0)) / voice_cnt));
    };
    
    return y;
}

// height of the cursor
mpx_t UserCursor::graphical_height(const ViewportParam& viewport) const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    
    // calculate vertical position and height
    mpx_t h = viewport.umtopx_v(cursor->pvoice->begin.staff().head_height); // initialize with head-height
    const size_t voice_cnt = staff_voice_count();
    const unsigned int line_cnt = cursor->pvoice->begin.staff().line_count;
    
    if (voice_cnt <= line_cnt + 1)
    {   // if we do not have too much voices
        h *= line_cnt + 2 - voice_cnt;      // height (for each voice one head-height shorter)
    }
    else h *= 2;
    
    return h;
}

// dump cursor state to stdout
void UserCursor::dump() const
{
    for (std::list<VoiceCursor>::const_iterator i = vcursors.begin(); i != vcursors.end(); ++i)
    {
        size_t idx = 0;
        for (std::list<Plate::pNote>::const_iterator j = i->pvoice->notes.begin(); j != i->pvoice->notes.end() && j != i->pnote; ++j, ++idx);
        if (!i->active) std::cout << "["; else std::cout << " ";
        std::cout << i->note.index() << "/" << idx << ":";
        if (i->has_prev()) std::cout << "<"; else std::cout << " ";
        if (i->pnote->at_end()) std::cout << "E";
        else if (i->note->is(Class::NEWLINE)) std::cout << "N";
        else std::cout << "> " << classname(i->note->classtype());
        std::cout << " (@ " << i->time << "-" << i->ntime << ")";
        if (!i->active) std::cout << "]\n"; else std::cout << ((i == cursor) ? " <=\n" : "\n");
    };
    std::cout << "\n";
}


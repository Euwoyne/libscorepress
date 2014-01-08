
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2014 Dominik Lehmann
  
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

#include "edit_cursor.hh"
#include "log.hh"

using namespace ScorePress;

inline int _round(const double d) {return static_cast<int>(d + 0.5);}
inline value_t get_value(const StaffObject* obj) {return (obj->is(Class::NOTEOBJECT)) ? static_cast<const NoteObject*>(obj)->value() : value_t(0);}

static const int tone_off[7]  = {0, 2, 4, 5, 7, 9, 11};    // (tone by offset)

// calculate tone from note-name (regarding input method, ignoring accidentals)
tone_t EditCursor::get_tone(const InputNote& note) const throw(NotValidException)
{
    const tone_t& clef_base = get_staff_context().base_note();     // get clef-base note

    // set the correct octave
    int octave = clef_base / 12;                           // calculate octave of the clef-base
    if (param->input_base <= InterfaceParam::LOWER_B)      // if input-base is to be below the clef-base...
    {
        if (clef_base % 12 < tone_off[param->input_base%7] && octave > 0)  // ...but it is not yet, ...
            --octave;                                                      // ...reduce the input-base octave.
    }
    else if (param->input_base <= InterfaceParam::UPPER_B) // if input-base is to be above the clef-base...
    {
        if (clef_base % 12 > tone_off[param->input_base%7] && octave < 32) // ...but it is not yet, ...
            ++octave;                                                      // ...increase the input-base octave.
    }
    else // (InterfaceParam::NEAREST)                      // if input-base should minimize distance to clef-base...
    {
        const int diff =   (tone_off[note.name] + Accidental::note_modifier[note.accidental]) % 12
                         - tone_off[clef_base % 12];       // caluclate clef-base to note distance
        if (diff > 6)        --octave;                     // if we are too high, reduce input-base
        else if (diff <= -6) ++octave;                     // if we are too low, increase input-base
    };

    // add offset of chosen tone (according to C major scale)
    return tone_t(12 * (octave + ((note.octave < -octave) ? -octave : note.octave)) + tone_off[note.name]);
}

// create the head instance (according to "relative_accidentals" parameter)
HeadPtr EditCursor::create_head(const InputNote& note) const throw(NotValidException)
{
    Head* out = new Head;                   // create head instance
    out->tone = get_tone(note);             // calculate whole tone
    out->dot_offset = param->dot_offset;    // set dot-offset

    // apply accidental (respecting the "relative_accidentals" parameter)
    if (param->relative_accidentals)
    {
        int acc_modifier;       // accidental

        // check user input
        switch (note.accidental)
        {
        case Accidental::double_flat:
        case Accidental::flat:
        case Accidental::natural:
        case Accidental::sharp:
        case Accidental::double_sharp:
            // calculate the accidental dictated by the key
            acc_modifier =  Accidental::note_modifier[get_staff_context().get_key_accidental(out->tone)]
                          + Accidental::note_modifier[note.accidental]; // and modify by user input
            if      (acc_modifier >  2) acc_modifier =  2;  // check range
            else if (acc_modifier < -2) acc_modifier = -2;

            // calculate accidental type (from modifier)
            out->accidental.type = static_cast<Accidental::Type>(2 * (acc_modifier + 2));
            break;

        // do not modify quarter-tone accidentals
        default:
            acc_modifier = Accidental::note_modifier[note.accidental];  // default modifier
            out->accidental.type = note.accidental;                     // set given accidental
            break;
        };

        out->tone = tone_t(out->tone + acc_modifier);
        out->accidental.offset_x = param->accidental_offset;
    }

    // default accidental input mode (does not regard key-signature)
    else
    {
        out->tone = tone_t(out->tone + Accidental::note_modifier[note.accidental]);
        out->accidental.type = note.accidental;
        out->accidental.offset_x = param->accidental_offset;
    };

    // return created head
    return HeadPtr(out);
}

// calculates the automatic stem-length (uses staff reference)
void EditCursor::set_auto_stem_length(Chord& chord) const throw(NotValidException)
{
    // set stem length to param->stem_length, but let the stem at least reach the center line
    const StaffContext& ctx(get_staff_context());
    switch (get_voice().stem_direction)
    {
    case Voice::STEM_AUTOMATIC:
        chord.stem_length =   ctx.note_offset(*chord.heads.front(), 500)
                            + ctx.note_offset(*chord.heads.back(), 500)
                            - 500 * (cursor->note.staff().line_count - 2);
        if (chord.stem_length < param->stem_length && chord.stem_length > -param->stem_length)
            chord.stem_length = (chord.stem_length > 0) ? param->stem_length : -param->stem_length;
        break;
    case Voice::STEM_UPWARDS:
        chord.stem_length =   ctx.note_offset(*chord.heads.back(), 1000)
                            - 500 * (cursor->note.staff().line_count - 2);
        if (chord.stem_length < param->stem_length)
            chord.stem_length = param->stem_length;
        break;
    case Voice::STEM_DOWNWARDS:
        chord.stem_length =   ctx.note_offset(*chord.heads.front(), 1000)
                            - 500 * (cursor->note.staff().line_count - 2);
        if (chord.stem_length > -param->stem_length)
            chord.stem_length = -param->stem_length;
        break;
    };
    
    // elongate stem for flags/beams
    if (chord.val.exp < VALUE_BASE - 2)
    {
        if (!get_staff().style)
        {
            if (chord.stem_length < 0)
                chord.stem_length -=
                    //engraver->get_style().beam_height
                    + (VALUE_BASE - 3 - chord.val.exp)
                      * (engraver->get_style().beam_distance + engraver->get_style().beam_height);
            else
                chord.stem_length +=
                    //engraver->get_style().beam_height
                    + (VALUE_BASE - 3 - chord.val.exp)
                      * (engraver->get_style().beam_distance + engraver->get_style().beam_height);
        }
        else
        {
            if (chord.stem_length < 0)
                chord.stem_length -=
                    //get_staff().style->beam_height
                    + (VALUE_BASE - 3 - chord.val.exp)
                      * (get_staff().style->beam_distance + get_staff().style->beam_height);
            else
                chord.stem_length +=
                    //get_staff().style->beam_height
                    + (VALUE_BASE - 3 - chord.val.exp)
                      * (get_staff().style->beam_distance + get_staff().style->beam_height);
        };
        
        // round to full 500
        chord.stem_length -= chord.stem_length % 500;
    };
}

// sets the stem-length, so that the stem alignes with the second note (uses staff reference)
void EditCursor::set_stem_aligned(Chord& chord, const Chord& chord2) const throw(NotValidException)
{
    const StaffContext& ctx(get_staff_context());
    if (chord2.stem_length < 0)
        chord.stem_length =   chord2.stem_length
                            - ctx.note_offset(*chord2.heads.front(), 1000)
                            + ctx.note_offset(*chord.heads.front(), 1000);
    else
        chord.stem_length =   chord2.stem_length
                            - ctx.note_offset(*chord2.heads.back(), 1000)
                            + ctx.note_offset(*chord.heads.back(), 1000);
}

#define ABS(x) ((x<0)?(-(x)):(x))
void EditCursor::set_stem_aligned(Chord& chord, Chord& chord2, bool shorten) const throw(NotValidException)
{
    const StaffContext& ctx(get_staff_context());
    const int new1 = (chord2.stem_length < 0) ?   chord2.stem_length
                                                - ctx.note_offset(*chord2.heads.front(), 1000)
                                                + ctx.note_offset(*chord.heads.front(), 1000)
                                              :   chord2.stem_length
                                                - ctx.note_offset(*chord2.heads.back(), 1000)
                                                + ctx.note_offset(*chord.heads.back(), 1000);
    const int new2 = (chord.stem_length < 0) ?    chord.stem_length
                                                - ctx.note_offset(*chord.heads.front(), 1000)
                                                + ctx.note_offset(*chord2.heads.front(), 1000)
                                              :   chord.stem_length
                                                - ctx.note_offset(*chord.heads.back(), 1000)
                                                + ctx.note_offset(*chord2.heads.back(), 1000);
    if ((shorten) == (ABS(new1) < ABS(chord.stem_length)))
        chord.stem_length = new1;
    else if ((shorten) == (ABS(new2) < ABS(chord2.stem_length)))
        chord2.stem_length = new2;
    else chord.stem_length = new1;
}

// execute the given function for each chord in the beam-group defined by the cursor
bool EditCursor::for_each_chord_in_beam_do(VoiceCursor& cur, void (*func)(Chord&,const int,int*), const int arg, int* out)
{
    // for notes without beams
    if (   cur.pnote->beam_begin == cur.pvoice->notes.end()
        && cur.note->is(Class::CHORD))
    {   // just set the value, and quit
        (*func)(static_cast<Chord&>(*cur.note), arg, out);
        return true;
    };
    
    // if our note has got a beam, ...
    // goto beam front
    VoiceCursor i(cur);
    while (i.pnote != cur.pnote->beam_begin)
        if (!i.has_prev()) return false;
        else
            i.prev();
    
    const Plate::pNote* e = i.pnote->beam[VALUE_BASE - 3]->end;
    
    // iterate to the end
    while(true)
    {
        if (i.note->is(Class::CHORD))
        {
            (*func)(static_cast<Chord&>(*i.note), arg, out);
        };
        if (&*i.pnote == e || !i.has_next()) break;
        i.next();
    };
    
    return true;
}

// constructors
EditCursor::EditCursor(Document& doc, Pageset& pset, const InterfaceParam& _param) : UserCursor(doc, pset), param(&_param), engraver(NULL) {}
EditCursor::EditCursor(Document& doc, Pageset& pset, const InterfaceParam& _param, Engraver& _engraver) : UserCursor(doc, pset), param(&_param), engraver(&_engraver) {}

// reengrave the score and update this cursor (all iterators and pVoice::begin must be valid when calling this!)
void EditCursor::reengrave(const MoveMode& mode) throw(NoScoreException, Error)
{
    if (!engraver) return;
    if (!has_score()) throw NoScoreException();
    
    // save plate information (plateinfo and pnote/pvoice will be invalidated)
    const unsigned int pageno = plateinfo->pageno;          // current page number (relative to score's beginning)
    const unsigned int start_page = plateinfo->start_page;  // the score's start-page number
    const const_Cursor vbegin = cursor->pvoice->begin;      // current cursor voice-begin
    
    // engrave the score (invalidates page, plateinfo, line and every "pnote" and "pvoice" of the voice-cursors)
    engraver->set_pageset(*pageset);
    engraver->engrave(*score, start_page);
    
    // recalculate on-plate references (page and plateinfo)
    page = pageset->get_page(start_page + pageno);      // get correct page
    if (page != pageset->pages.end())
    {
        plateinfo = &*page->get_plate_by_score(*score); // get plate information
    };
    if (page == pageset->pages.end())   // if the page does not exist
    {
        score = NULL;       // invalidate cursor
        vcursors.clear();
        cursor = vcursors.end();
        throw Error("Unable to find the given score in the page-set after reengraving.");
    };
    
    // search line and current voice (based on saved pVoice::begin cursor "vbegin")
    line = plateinfo->plate->lines.begin();     // initialize "line"
    std::list<Plate::pVoice>::iterator voice;   // voice
    for (; line != plateinfo->plate->lines.end(); ++line)
    {
        // get voice and check for old begin
        voice = line->get_voice(vbegin.voice());
        
        if (voice == line->voices.end()) continue;  // voice exists?
        if (voice->begin != vbegin)      continue;  // past old begin?
        if (mode == NEWLINE)                        // on inserted newline, ...
            if (++line == plateinfo->plate->lines.end()) break; // ...goto next line and break on nonexisting
        break;
    };
    
    // if we did not find the correct line
    if (line == plateinfo->plate->lines.end())
    {
        line = plateinfo->plate->lines.begin(); // use the first on the plate
        prepare_voices();                       // prepare voices
        log_warn("Unable to find the given line on the page after reengraving. Default to first. (class: EditCursor)");
        return;
    };
    
    if (mode == NEWLINE) return prepare_voices();
    
    {
    // initialize cursor
    prepare_plate(*cursor, *voice);
    
    // iterate the line to find the on-plate note
    while (!cursor->pnote->at_end() && &*cursor->pnote->note != &*cursor->note)
    {
        do ++cursor->pnote;                             // increment on-plate iterator
        while (   !cursor->pnote->at_end()              // ignore inserted objects
               && cursor->pnote->is_inserted());
        
        cursor->time = cursor->ntime;                   // update time-stamp
        if (!cursor->pnote->at_end())
            cursor->ntime += get_value(&*cursor->pnote->note);
    };
    
    // if we did not find the note
    if (&*cursor->pnote->note != &*cursor->note)
        log_warn("Unable to find current note within the reengraved voice. (class: EditCursor)");
    }
    
    // find on-plate notes for other voices (matching the VoiceCursor::note iterator)
    {
    for (std::list<VoiceCursor>::iterator c = vcursors.begin(); c != vcursors.end(); )
    {
        if (c == cursor)        // the current cursor has already been prepared
        {                       // (during the search for the correct line)
            ++c;                // so ignore it
            continue;           // and continue
        };
        
        // find on-plate voice
        std::list<Plate::pVoice>::iterator v;
        v = line->get_voice(c->note.voice());   // get on-plate voice
        if (v == line->voices.end())            // check for existance
        {
            c = vcursors.erase(c);                  // remove cursors to non-existant voices
            continue;
        };
        
        // initialize cursor
        prepare_plate(*c, *v);
        
        // iterate the line to find the on-plate note
        while (!c->pnote->at_end() && &*c->pnote->note != &*c->note)
        {
            do ++c->pnote;                      // increment on-plate iterator
            while (   !c->pnote->at_end()       // ignore inserted objects
                   && c->pnote->is_inserted());
            
            c->time = c->ntime;                     // update time-stamp
            if (!c->pnote->note.at_end())
                c->ntime += get_value(&*c->pnote->note);
        };
        
        // if we did not find the note
        if (&*c->pnote->note != &*c->note)
        {
            c = vcursors.erase(c);      // remove the cursor
            log_warn("Unable to find given note within the reengraved voice. (class: EditCursor)");
            continue;
        };
        
        ++c;
    };
    }
    
    // check positions and active flags
    update_voices();
    if (mode == INSERT && !at_end()) next();
}

// insert an object (inserting transfers the objects ownership to the voice-object within the score)
void EditCursor::insert(StaffObject* const object) throw(NotValidException, Cursor::IllegalObjectTypeException)
{
    if (!ready()) throw NotValidException();
    if (!cursor->has_prev())                    // if the object was inserted at the front...
    {
        cursor->note.insert(object);                // insert the new object
        cursor->pvoice->begin = cursor->note;       // ...set voice begin cursor
    }
    else cursor->note.insert(object);           // otherwise, just insert the new object
    reengrave(INSERT);                          // reengrave the score
}

// insert a note
void EditCursor::insert(const InputNote& note) throw(NotValidException, NoScoreException, Error)
{
    if (!ready()) throw NotValidException();    // check cursor
    Chord* chord = new Chord();                 // create new chord
    chord->heads.push_back(create_head(note));  // create head
    chord->set_exp(note.exp);                   // set value
    chord->set_dots(note.dots);                 // set dots
    set_auto_stem_length(*chord);               // set stem-length
    insert(chord);                              // insert the chord
}

// insert a head
void EditCursor::insert_head(const InputNote& note) throw(NotValidException, Cursor::IllegalObjectTypeException, NoScoreException, Error)
{
    if (!ready()) throw NotValidException();            // check cursor
    if (at_end() || !cursor->note->is(Class::CHORD))    // check current object type
        throw Cursor::IllegalObjectTypeException();     //    throw exception, if no chord
    Chord& chord = static_cast<Chord&>(*cursor->note);  // get target chord
    const HeadPtr& head = create_head(note);            // create head instance
    
    // insert head, sorting (ascending) by tone
    for (HeadList::iterator h = chord.heads.begin(); h != chord.heads.end(); ++h)
    {
        if ((*h)->tone == head->tone)   // if the head exists
        {
            chord.heads.erase(h);           // remove the existing head
            reengrave(NONE);                // reengrave score
            return;                         // stop iterating
        };
        if ((*h)->tone > head->tone)    // if we got a higher tone
        {                                   // insert and set default dot offset
            (*chord.heads.insert(h, head))->dot_offset = (*h)->dot_offset;
            set_auto_stem_length(chord);    // reset stem-length
            reengrave(NONE);                // reengrave score
            return;                         // stop iterating
        };
    };
    chord.heads.push_back(head);        // add head
    chord.heads.back()->dot_offset = chord.heads.front()->dot_offset;
    set_auto_stem_length(chord);        // reset stem-length
    reengrave(NONE);                    // reengrave score
}

// insert a rest
void EditCursor::insert_rest(const unsigned char exp, const unsigned char dots) throw(NotValidException, NoScoreException, Error)
{
    if (!ready()) throw NotValidException();    // check cursor
    Rest* rest = new Rest();                    // create new rest
    rest->set_exp(exp);                         // set value
    rest->set_dots(dots);                       // set dots
    insert(rest);
}

// insert newline objects into all active voices
void EditCursor::insert_newline() throw(NotValidException, NoScoreException, Error)
{
    // check if this is a newline completion (i.e. add newline only to those voices without one)
    bool complete = true;
    for (std::list<VoiceCursor>::iterator cur = vcursors.begin(); cur != vcursors.end(); ++cur)
    {
        if (cur->active && (cur->pnote->at_end() || cur->note->classtype() != Class::NEWLINE))
        {
            complete = false;
            break;
        };
    }
    
    // add the newlines
    bool first = true;
    for (std::list<VoiceCursor>::iterator cur = vcursors.begin(); cur != vcursors.end(); ++cur)
    {
        if (   (cur->active || cur->note.is_main()) && cur->note.has_prev()
            && (complete || cur->pnote->at_end() || cur->note->classtype() != Class::NEWLINE))
        {
            // insert newline
            if (!cur->has_prev())
            {
                cur->note.insert(new Newline(cur->get_layout()));
                cur->pvoice->begin = cur->note;
            }
            else cur->note.insert(new Newline(cur->get_layout()));
            
            // set line distance
            if (first) static_cast<Newline&>(*cur->note).distance = param->newline_distance, first = false;
            static_cast<Newline&>(*cur->note).indent = 0;
        };
    };
    
    reengrave(NEWLINE);
}

// insert pagebreak objects into all active voices
void EditCursor::insert_pagebreak() throw(NotValidException, NoScoreException, Error)
{
    // check if this is a newline completion (i.e. add newline only to those voices without one)
    bool complete = true;
    for (std::list<VoiceCursor>::iterator cur = vcursors.begin(); cur != vcursors.end(); ++cur)
    {
        if (cur->active && (cur->pnote->at_end() || cur->note->classtype() != Class::PAGEBREAK))
        {
            complete = false;
            break;
        };
    }
    
    // add the newlines
    bool first = true;
    for (std::list<VoiceCursor>::iterator cur = vcursors.begin(); cur != vcursors.end(); ++cur)
    {
        if (   (cur->active || cur->note.is_main()) && cur->note.has_prev()
            && (complete || cur->pnote->at_end() || cur->note->classtype() != Class::PAGEBREAK))
        {
            // save layout/dimension
            const Newline&        layout    = cur->get_layout();
            const ScoreDimension& dimension = get_dimension();
            
            // insert newline
            if (!cur->has_prev())
            {
                cur->note.insert(new Pagebreak());
                cur->pvoice->begin = cur->note;
            }
            else cur->note.insert(new Pagebreak());
            
            // set layout
            static_cast<Pagebreak&>(*cur->note).Newline::operator=(layout);
            if (first) static_cast<Pagebreak&>(*cur->note).distance = param->newline_distance, first = false;
            static_cast<Pagebreak&>(*cur->note).indent = 0;
            static_cast<Pagebreak&>(*cur->note).dimension = dimension;
        };
    };
    
    reengrave(PAGEBREAK);
}

// remove a note
void EditCursor::remove() throw(NotValidException)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) return;                           // no note at end
    if (cursor->pvoice->begin == cursor->note)      // if the front note is removed...
        ++cursor->pvoice->begin;                    //    increment on-plate voice front (needed by "reengrave()")
    StaffObject* del_note = cursor->note->clone();
    
    // remove note
    cursor->note.remove();                          // remove the note
    
    // migrate subvoice and durable objects to next note
    if (!cursor->note.at_end())                     // if there is a next note
    {
        // migrate subvoice
        if (   cursor->note->is(Class::NOTEOBJECT)              // if this note can hold a subvoice
            && del_note->is(Class::NOTEOBJECT)
            && static_cast<NoteObject&>(*del_note).subvoice)    // and there was a subvoice
        {                                                       //    migrate subvoice to new parent note
            static_cast<NoteObject&>(*cursor->note).add_subvoice(static_cast<NoteObject&>(*del_note).subvoice);
        };
        
        // migrate durable objects
        VisibleObject& src(del_note->get_visible());            // source visible object
        VisibleObject& tgt(cursor->note->get_visible());        // target visible object
        for (MovableList::iterator i = src.attached.begin(); i != src.attached.end(); ++i)
        {
            if ((*i)->is(Class::DURABLE) && static_cast<Durable&>(**i).duration > 1)    // for all durables longer than one note
            {
                tgt.attached.push_back(MovablePtr((*i)->clone()));          // attach cloned instance
                --static_cast<Durable&>(*tgt.attached.back()).duration;     // decrease duration
            };
        };
    };
    delete del_note;                                // free old note
    
    // reengrave score
    reengrave(REMOVE);
}

// remove a voice
void EditCursor::remove_voice() throw(NotValidException, Cursor::IllegalObjectTypeException)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) return;                           // no note at end
    
    // only sub-voices can be removed
    if (!cursor->note.is_sub())
        throw Cursor::IllegalObjectTypeException();
    
    // check for parent type
    if (static_cast<SubVoice&>(cursor->note.voice()).parent->is(Class::SUBVOICE))
    {
        // iterate the parent and delete the reference to this voice
        VoiceObjectList::iterator       i = static_cast<SubVoice*>(static_cast<SubVoice&>(cursor->note.voice()).parent)->notes.begin();
        const VoiceObjectList::iterator e = static_cast<SubVoice*>(static_cast<SubVoice&>(cursor->note.voice()).parent)->notes.end();
        for (; i != e; ++i)
            if ((*i)->is(Class::NOTEOBJECT))
                if (static_cast<NoteObject&>(**i).subvoice)
                    if (&*static_cast<NoteObject&>(**i).subvoice == &cursor->note.voice()) {
                        free(static_cast<NoteObject&>(**i).subvoice); break;};
    }
    else
    {
        // iterate the parent and delete the reference to this voice
        StaffObjectList::iterator       i = static_cast<MainVoice*>(static_cast<SubVoice&>(cursor->note.voice()).parent)->notes.begin();
        const StaffObjectList::iterator e = static_cast<MainVoice*>(static_cast<SubVoice&>(cursor->note.voice()).parent)->notes.end();
        for (; i != e; ++i)
            if ((*i)->is(Class::NOTEOBJECT))
                if (static_cast<NoteObject&>(**i).subvoice)
                    if (&*static_cast<NoteObject&>(**i).subvoice == &cursor->note.voice()) {
                        free(static_cast<NoteObject&>(**i).subvoice); break;};
    };
    
    // erase the corresponding cursor
    vcursors.erase(cursor);
    cursor = find(*static_cast<SubVoice&>(cursor->note.voice()).parent);
    if (cursor == vcursors.end()) --cursor;
    
    // reengrave score
    reengrave(REMOVE);
}

// get the line layout object (non-constant)
const Newline& EditCursor::get_layout() const throw(NotValidException)
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->get_layout();
}

static void _add_stem_length(Chord& chord, const int pohh, int*)
{
    chord.stem_length += pohh;
}

void EditCursor::add_stem_length(int pohh) throw(Cursor::IllegalObjectTypeException)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    if (!for_each_chord_in_beam_do(*cursor, &_add_stem_length, pohh))
        log_warn("Unable to find beam begin. (class: EditCursor)");
}

static void _set_stem_length(Chord& chord, const int pohh, int*)
{
    chord.stem_length = pohh;
}

void EditCursor::set_stem_length(int pohh) throw(Cursor::IllegalObjectTypeException)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    if (!for_each_chord_in_beam_do(*cursor, &_set_stem_length, pohh))
        log_warn("Unable to find beam begin. (class: EditCursor)");
}

void EditCursor::add_stem_slope(int pohh) throw(Cursor::IllegalObjectTypeException)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    
    // check, if there even is a beam
    if (cursor->pnote->beam_begin == cursor->pvoice->notes.end())
        return;
    
    // adjust the end-notes beam length
    VoiceCursor i(*cursor);
    const Plate::pNote* e = cursor->pnote->beam_begin->beam[VALUE_BASE - 3]->end;
    while (!i.at_end() && &*i.pnote != e) i.next();
    if (i.at_end())
        return log_warn("Unable to find beam end. (class: EditCursor)");
    if (!i.note->is(Class::CHORD))
        return log_warn("Beam end is no chord. (class: EditCursor)");
    static_cast<Chord&>(*i.note).stem_length += pohh;
}

void EditCursor::set_stem_slope(int pohh) throw(Cursor::IllegalObjectTypeException)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    
    // check, if there even is a beam
    if (cursor->pnote->beam_begin == cursor->pvoice->notes.end())
        return;
    
    // adjust the end-notes beam length
    VoiceCursor i(*cursor);
    const Plate::pNote* e = cursor->pnote->beam_begin->beam[VALUE_BASE - 3]->end;
    while (!i.at_end() && &*i.pnote != e) i.next();
    if (i.at_end())
        return log_warn("Unable to find beam end. (class: EditCursor)");
    if (!i.note->is(Class::CHORD))
        return log_warn("Beam end is no chord. (class: EditCursor)");
    if (!cursor->pnote->beam_begin->note->is(Class::CHORD))
        return log_warn("Beam begin is no chord. (class: EditCursor)");
    set_stem_aligned(static_cast<Chord&>(*i.note), static_cast<const Chord&>(*cursor->pnote->beam_begin->note));
    static_cast<Chord&>(*i.note).stem_length += pohh;
}

static void _set_stem_dir(Chord& chord, const int dir, int*)
{
    if ((dir < 0) != (chord.stem_length < 0))
        chord.stem_length = -chord.stem_length;
}

void EditCursor::set_stem_dir(bool down) throw(Cursor::IllegalObjectTypeException)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    if (!for_each_chord_in_beam_do(*cursor, &_set_stem_dir, down ? -1 : 1))
        log_warn("Unable to find beam begin. (class: EditCursor)");
}

inline static bool is_below(const Position<mpx_t>& p, const Position<mpx_t>& p1, const Position<mpx_t>& p2, const mpx_t e)
{   // NOTE: it is mandatory, that p2.x > p1.x
    return ((p.y - p1.y) * (p2.x - p1.x) < (p.x - p1.x) * (p2.y - p1.y) - e);
}

inline static bool is_above(const Position<mpx_t>& p, const Position<mpx_t>& p1, const Position<mpx_t>& p2, const mpx_t e)
{   // NOTE: it is mandatory, that p2.x > p1.x
    return ((p.y - p1.y) * (p2.x - p1.x) > (p.x - p1.x) * (p2.y - p1.y) + e);
}

// set auto stem length to current object
void EditCursor::set_stem_length_auto() throw(Cursor::IllegalObjectTypeException)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end() || !cursor->note->is(Class::CHORD))
        throw Cursor::IllegalObjectTypeException();
    
    // for notes without beams
    if (   cursor->pnote->beam_begin == cursor->pvoice->notes.end()
        && cursor->note->is(Class::CHORD))
    {   // just set the stem length, and quit
        set_auto_stem_length(static_cast<Chord&>(*cursor->note));
        return;
    };
    
    // if our note has got a beam, ...
    // goto beam front
    VoiceCursor b(*cursor);
    while (b.pnote != cursor->pnote->beam_begin)
        if (!b.has_prev())
            return log_warn("Unable to find beam begin. (class: EditCursor)");
        else
            b.prev();
    
    // get beam end note
    const Plate::pNote* e = b.pnote->beam[VALUE_BASE - 3]->end;
    
    // calculate inner trapezoid
    Position<mpx_t> begin_top    = *++b.pnote->absolutePos.begin();
    Position<mpx_t> begin_bottom = b.pnote->absolutePos.back();
    if (begin_bottom.y < begin_top.y)
    {
        begin_bottom = begin_top;
        begin_top = b.pnote->absolutePos.back();
    };
    
    Position<mpx_t> end_top    = *++e->absolutePos.begin();
    Position<mpx_t> end_bottom = e->absolutePos.back();
    if (end_bottom.y < end_top.y)
    {
        end_bottom = end_top;
        end_top = e->absolutePos.back();
    };
    
    // calculate the stem direction
    // (we completely avoid beamed notes with different stem directions on end notes)
    const mpx_t err = _round(engraver->get_viewport().umtopx_v(cursor->note.staff().head_height) / 2.0);
    signed char dir = -1;   // -1 - undecided;  0 - default; 1 - up; 2 - down
    VoiceCursor i(b);
    i.next();
    while(&*i.pnote != e)   // do not check the first and last note!
    {
        if (dir != 0 && !i.pnote->is_inserted() && i.pnote->note->is(Class::CHORD))
        {
            // check hull curve
            if (   is_below(*++i.pnote->absolutePos.begin(), begin_bottom, end_bottom, err)
                || is_below(   i.pnote->absolutePos.back(),  begin_bottom, end_bottom, err))
            {                               // if we have a head below the trapezoid
                dir = (dir == 2) ? 0 : 1;   // the beam should be above the notes
            };
            
            if (   is_above(*++i.pnote->absolutePos.begin(), begin_top, end_top, err)
                || is_above(   i.pnote->absolutePos.back(),  begin_top, end_top, err))
            {                               // if we have a head above the trapezoid
                dir = (dir == 1) ? 0 : 2;   // the beam should be below the notes
            };
        };
        i.next();
    };
    
    // check object types (these warnings should never occur)
    if (!i.note->is(Class::CHORD))
        return log_warn("Beam end is no chord. (class: EditCursor)");
    if (!b.note->is(Class::CHORD))
        return log_warn("Beam begin is no chord. (class: EditCursor)");
    
    // calculate stems for begin/end note
    Chord& begin_chord = static_cast<Chord&>(*b.note);
    Chord& end_chord   = static_cast<Chord&>(*i.note);
    set_auto_stem_length(begin_chord);
    if (cursor->note.voice().stem_direction == Voice::STEM_AUTOMATIC)
    {
        switch (dir)
        {
        case -1: // for default and undecided, rely on the first note
        case  0: cursor->note.voice().stem_direction =
                    (begin_chord.stem_length > 0) ? Voice::STEM_UPWARDS : Voice::STEM_DOWNWARDS;
                 break;
        case  1: cursor->note.voice().stem_direction = Voice::STEM_UPWARDS;   break;
        case  2: cursor->note.voice().stem_direction = Voice::STEM_DOWNWARDS; break;
        };
        set_auto_stem_length(begin_chord);
        set_auto_stem_length(end_chord);
        cursor->note.voice().stem_direction = Voice::STEM_AUTOMATIC;
    }
    else    // if the stem direction is dictated by the voice
    {       // and the calculated direction does not comply
        if (dir > 0 && dir != ((cursor->note.voice().stem_direction == Voice::STEM_UPWARDS) ? 1 : 2))
            dir = 0;                        // render the beam horizontally
        set_auto_stem_length(end_chord);
    };
    
    // check the beam slope
    set_stem_aligned(end_chord, begin_chord, false);
    if (dir != 0)       // if there are no disturbing notes
    {                   // we can slope the beam
        if (begin_chord.stem_length > end_chord.stem_length)
            begin_chord.stem_length -= param->autobeam_slope;
        else if (begin_chord.stem_length < end_chord.stem_length)
            end_chord.stem_length -= param->autobeam_slope;
    };
}

// set auto stem direction to current object
void EditCursor::set_stem_dir_auto() throw(Cursor::IllegalObjectTypeException)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end() || !cursor->note->is(Class::CHORD))
        throw Cursor::IllegalObjectTypeException();
    
    switch (get_voice().stem_direction)
    {
    case Voice::STEM_AUTOMATIC:
        {
        const StaffContext& ctx(get_staff_context());
        int stemdir =   ctx.note_offset(*static_cast<Chord&>(*cursor->note).heads.front(), 500)
                      + ctx.note_offset(*static_cast<Chord&>(*cursor->note).heads.back(), 500)
                      - 500 * (cursor->note.staff().line_count - 2);
        if (!for_each_chord_in_beam_do(*cursor, &_set_stem_dir, stemdir))
            log_warn("Unable to find beam begin. (class: EditCursor)");
        }
        break;
    case Voice::STEM_UPWARDS:
        if (!for_each_chord_in_beam_do(*cursor, &_set_stem_dir, -1))
            log_warn("Unable to find beam begin. (class: EditCursor)");
        break;
    case Voice::STEM_DOWNWARDS:
        if (!for_each_chord_in_beam_do(*cursor, &_set_stem_dir, 1))
            log_warn("Unable to find beam begin. (class: EditCursor)");
        break;
    };
}

// set auto accidental to current object
void EditCursor::set_accidental_auto()  throw(Cursor::IllegalObjectTypeException)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end() || !cursor->note->is(Class::CHORD))
        throw Cursor::IllegalObjectTypeException();
    
    Chord& chord = static_cast<Chord&>(*cursor->note);
    const StaffContext& ctx = get_staff_context();
    for (HeadList::iterator head = chord.heads.begin(); head != chord.heads.end(); ++head)
    {
        (*head)->accidental.type = ctx.guess_accidental((*head)->tone, param->prefer_natural);
    };
}



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

#include "edit_cursor.hh"
#include "object_cursor.hh" // ObjectCursor
#include "log.hh"           // Log

using namespace ScorePress;

//  helper functions
// ------------------

inline int _round(const double d) {return static_cast<int>(d + 0.5);}
inline value_t get_value(const StaffObject* obj) {return (obj->is(Class::NOTEOBJECT)) ? static_cast<const NoteObject*>(obj)->value() : value_t(0);}

static const int tone_off[7]  = {0, 2, 4, 5, 7, 9, 11};     // (tone by offset)


// exception classes
EditCursor::RemoveMainException::RemoveMainException()
        : UserCursor::Error("You cannot delete the staff's main-voice.") {}

// calculate tone from note-name (regarding input method, ignoring accidentals)
tone_t EditCursor::get_tone(const InputNote& note) const
{
    const tone_t& clef_base = get_staff_context().base_note();  // get clef-base note

    // set the correct octave
    int octave = clef_base / 12;                            // calculate octave of the clef-base
    if (param.input_base <= InterfaceParam::LOWER_B)        // if input-base is to be below the clef-base...
    {
        if (clef_base % 12 < tone_off[param.input_base%7] && octave > 0)    // ...but it is not yet, ...
            --octave;                                                       // ...reduce the input-base octave.
    }
    else if (param.input_base <= InterfaceParam::UPPER_B)   // if input-base is to be above the clef-base...
    {
        if (clef_base % 12 > tone_off[param.input_base%7] && octave < 32)   // ...but it is not yet, ...
            ++octave;                                                       // ...increase the input-base octave.
    }
    else // (InterfaceParam::NEAREST)                       // if input-base should minimize distance to clef-base...
    {
        const int diff =   (tone_off[note.name] + Accidental::note_modifier[note.accidental]) % 12
                         - tone_off[clef_base % 12];        // caluclate clef-base to note distance
        if (diff > 6)        --octave;                      // if we are too high, reduce input-base
        else if (diff <= -6) ++octave;                      // if we are too low, increase input-base
    };

    // add offset of chosen tone (according to C major scale)
    return tone_t(12 * (octave + ((note.octave < -octave) ? -octave : note.octave)) + tone_off[note.name]);
}

// create the head instance (according to "relative_accidentals" parameter)
HeadPtr EditCursor::create_head(const InputNote& note) const
{
    Head* out = new Head;                   // create head instance
    out->tone = get_tone(note);             // calculate whole tone
    out->dot_offset = param.dot_offset;     // set dot-offset

    // apply accidental (respecting the "relative_accidentals" parameter)
    if (param.relative_accidentals)
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
        out->accidental.offset_x = param.accidental_offset;
    }

    // default accidental input mode (does not regard key-signature)
    else
    {
        out->tone = tone_t(out->tone + Accidental::note_modifier[note.accidental]);
        out->accidental.type = note.accidental;
        out->accidental.offset_x = param.accidental_offset;
    };

    // return created head
    return HeadPtr(out);
}

// execute the given function for each chord in the beam-group defined by the cursor
bool EditCursor::for_each_chord_in_beam_do(VoiceCursor& cur, callback_t func, const int arg, int* out)
{
    // for notes without beams
    if (cur.pnote->beam_begin == cur.pvoice->notes.end())
    {   // just set the value, and quit
        if (cur.note->is(Class::CHORD))
            (*func)(static_cast<Chord&>(*cur.note), cur, arg, out);
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
            (*func)(static_cast<Chord&>(*i.note), i, arg, out);
        };
        if (&*i.pnote == e || !i.has_next()) break;
        i.next();
    };
    
    return true;
}

// set the given cursor "beam_begin" to the first note within the current beam-group
bool EditCursor::get_beam_begin(VoiceCursor& beam_begin) const
{
    // initialize
    beam_begin = *cursor;
    
    // check, if there even is a beam
    if (cursor->pnote->beam_begin == cursor->pvoice->notes.end())
        return false;
    
    // search for the first note within the beam
    while (beam_begin.has_prev() && beam_begin.pnote != cursor->pnote->beam_begin)
        beam_begin.prev();
    
    // check result of the search
    if (beam_begin.pnote != cursor->pnote->beam_begin)
    {
        log_warn("Unable to find beam begin. (class: EditCursor)");
        return false;
    };
    
    if (!beam_begin.note->is(Class::CHORD))
    {
        log_warn("Beam begin is no chord. (class: EditCursor)");
        return false;
    };
    
    // everything is fine
    return true;
}

//  constructor
// -------------
EditCursor::EditCursor(Document& _document, Pageset& _pageset,
                       const InterfaceParam& _param, const ViewportParam& _viewport)
    : UserCursor(_document, _pageset), param(_param), viewport(_viewport) {}

EditCursor::~EditCursor() {}

//  access methods
// ----------------

// return the staff
Staff& EditCursor::get_staff()
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->note.staff();
}

// return the voice
Voice& EditCursor::get_voice()
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->note.voice();
}

// return the score-cursor
Cursor& EditCursor::get_cursor()
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->note;
}

// return objects attached to the note
MovableList& EditCursor::get_attached()
{
    if (cursor == vcursors.end())     throw NotValidException();
    return cursor->note->get_visible().attached;
}

// return the on-plate voice
Plate::pVoice& EditCursor::get_pvoice()
{
    if (cursor == vcursors.end()) throw NotValidException();
    return *cursor->pvoice;
}

// return the on-plate note
Plate::pNote& EditCursor::get_platenote()
{
    if (cursor == vcursors.end()) throw NotValidException();
    return *cursor->pnote;
}

// return the style parameters
StyleParam& EditCursor::get_style()
{
    if (!ready()) throw NotValidException();
    if (!!cursor->note.staff().style) return *cursor->note.staff().style;
    if (!!score->style) return *score->style;
    return document->style;
}

// return the line layout
LayoutParam& EditCursor::get_layout()
{
    if (cursor == vcursors.end()) throw NotValidException();
    if (cursor->newline.ready()) return static_cast<Newline&>(*cursor->newline).layout;
    return cursor->note.staff().layout;
}

// return the score dimension
ScoreDimension& EditCursor::get_dimension()
{
    if (cursor == vcursors.end()) throw NotValidException();
    if (cursor->pagebreak.ready()) return static_cast<Pagebreak&>(*cursor->pagebreak).dimension;
    return score->layout.dimension;
}

// return objects attached to the page
MovableList& EditCursor::get_page_attached()
{
    if (cursor == vcursors.end()) throw NotValidException();
    return cursor->pagebreak.ready() ? static_cast<Pagebreak&>(*cursor->pagebreak).attached
                                     : score->layout.attached;
}

//  insertion interface
// ---------------------

// insert an object (inserting transfers the objects ownership to the voice-object within the score)
void EditCursor::insert(StaffObject* const object)
{
    if (!ready()) throw NotValidException();
    if (!cursor->has_prev())                    // if the object was inserted at the front...
    {
        cursor->note.insert(object);                // insert the new object
        cursor->pvoice->begin = cursor->note;       // ...set voice begin cursor
    }
    else cursor->note.insert(object);           // otherwise, just insert the new object
}

// insert a note
void EditCursor::insert(const InputNote& note)
{
    if (!ready()) throw NotValidException();                        // check cursor
    Chord* chord = new Chord();                                     // create new chord
    chord->heads.push_back(create_head(note));                      // create head
    chord->set_exp(note.exp);                                       // set value
    chord->set_dots(note.dots <= note.exp ? note.dots : note.exp);  // set dots
    insert(chord);                                                  // insert the chord
}

// insert a head
void EditCursor::insert_head(const InputNote& note)
{
    if (!ready()) throw NotValidException();            // check cursor
    if (at_end() || !cursor->note->is(Class::CHORD))    // check current object type
        throw Cursor::IllegalObjectTypeException();     //    throw exception, if no chord
    Chord& chord = static_cast<Chord&>(*cursor->note);  // get target chord
    const HeadPtr& head = create_head(note);            // create head instance
    
    // insert head, sorting (ascending) by tone
    for (HeadList::iterator h = chord.heads.begin(); h != chord.heads.end(); ++h)
    {
        // if the head exists
        if (   static_cast<int>((*h)->tone) - Accidental::note_modifier[(*h)->accidental.type]
            == static_cast<int>(head->tone) - Accidental::note_modifier[head->accidental.type])
        {
            chord.heads.erase(h);           // remove the existing head
            return;                         // stop iterating
        };
        
        // if we got a higher tone
        if ((*h)->tone > head->tone)
        {                                   // insert and set default dot offset
            (*chord.heads.insert(h, head))->dot_offset = (*h)->dot_offset;
            return;                         // stop iterating
        };
    };
    
    chord.heads.push_back(head);        // add head
    chord.heads.back()->dot_offset = chord.heads.front()->dot_offset;
}

// insert a rest
void EditCursor::insert_rest(const unsigned char exp, const unsigned char dots)
{
    if (!ready()) throw NotValidException();    // check cursor
    Rest* rest = new Rest();                    // create new rest
    rest->set_exp(exp);                         // set value
    rest->set_dots(dots);                       // set dots
    insert(rest);
}

// insert newline objects into all active voices
void EditCursor::insert_newline()
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
            
            // set distance
            if (first)
            {
                static_cast<Newline&>(*cur->note).layout.distance = param.newline_distance;
                first = false;
            };
            
            // reset indent
            static_cast<Newline&>(*cur->note).layout.indent = 0;
        };
    };
}

// insert pagebreak objects into all active voices
void EditCursor::insert_pagebreak()
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
            const LayoutParam&    layout    = cur->get_layout();
            const ScoreDimension& dimension = get_dimension();
            
            // insert newline
            if (!cur->has_prev())
            {
                cur->note.insert(new Pagebreak());
                cur->pvoice->begin = cur->note;
            }
            else cur->note.insert(new Pagebreak());
            
            // set layout
            static_cast<Pagebreak&>(*cur->note).layout = layout;
            static_cast<Pagebreak&>(*cur->note).layout.indent = 0;
            static_cast<Pagebreak&>(*cur->note).dimension = dimension;
            
            // set distance
            if (first)
            {
                static_cast<Pagebreak&>(*cur->note).layout.distance = param.newline_distance;
                first = false;
            };
        };
    };
}

//  deletion interface
// --------------------

// remove a note
void EditCursor::remove()
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) return;                           // no note at end
    StaffObject* del_note = cursor->note->clone();
    
    Cursor nextnote = cursor->note;
    ++nextnote;
    
    // migrate subvoice and durable objects to next note
    if (!nextnote.at_end() && !nextnote->is(Class::NEWLINE))    // if there is a next note
    {
        // migrate subvoices
        if (nextnote->is(Class::NOTEOBJECT) && cursor->note->is(Class::NOTEOBJECT))
        {
            bool below = false;
            NoteObject& src(static_cast<NoteObject&>(*cursor->note));
            NoteObject& tgt(static_cast<NoteObject&>(*nextnote));
            for (SubVoiceList::iterator v = src.subvoices.begin(); v != src.subvoices.end(); ++v)
            {
                if (src.subvoices.is_first_below(v))    // if this is the first voice below
                    below = true;                       //     all coming voices will be below
                
                if (below)                                      // if below,
                    v->transfer_to(tgt.subvoices.add_bottom()); //     add voice at the bottom
                else                                            // if above
                    v->transfer_to(tgt.subvoices.add_above());  //     add voice above
            };
        };
        
        // migrate durable objects
        VisibleObject& src(cursor->note->get_visible());
        VisibleObject& tgt(nextnote->get_visible());
        for (MovableList::iterator i = src.attached.begin(); i != src.attached.end(); ++i)
        {
            if (!(*i)->is(Class::DURABLE)) continue;
            if (!del_note->is(Class::NOTEOBJECT))               // if the removed object has no value
            {
                tgt.attached.push_back(MovablePtr((*i)->clone()));      // just attach cloned instance
            }
                                                                // if the durable is longer than the removed note
            else if (static_cast<Durable&>(**i).duration > static_cast<NoteObject&>(*del_note).value())
            {                                                           // decrease duration
                static_cast<Durable&>(**i).duration -= static_cast<NoteObject&>(*del_note).value();
                tgt.attached.push_back(MovablePtr((*i)->clone()));      // attach cloned instance
            };
        };
    };
    
    // remove note
    cursor->note.remove();
}

// remove a voice
void EditCursor::remove_voice()
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) return;                           // no note at end
    
    // only sub-voices can be removed
    if (!cursor->note.is_sub() || !cursor->pvoice->parent.ready())
        throw RemoveMainException();
    
    // delete voice-cursor
    std::list<VoiceCursor>::iterator del_voice(cursor);
    if (has_prev_voice()) prev_voice();
    else next_voice();
    vcursors.erase(del_voice);
    
    // remove voice from parent note
    // (const-cast not unexpected, because there is a non-const instance within the score)
    NoteObject& parent = const_cast<NoteObject&>(static_cast<const NoteObject&>(*del_voice->pvoice->parent));
    parent.subvoices.remove(del_voice->note.voice());
}

// remove newline/pagebreak
void EditCursor::remove_newline()
{
    if (!ready()) throw NotValidException();        // check cursor
    if (   line == plateinfo->plate->lines.begin()  // if we are at the scores front
        && page == pageset->pages.begin())          //   (i.e. first line/first page)
            return;                                 //      do nothing
    home();                                         // goto line front
    
    // prepare voice begin update
    Plate::Iterator pline(line);    // previous line (to get voice begins)
    --pline;                        // move to previous line
    
    // for each voice
    for (std::list<VoiceCursor>::iterator cur = vcursors.begin(); cur != vcursors.end(); ++cur)
    {
        Cursor note(cur->note);
        if (!note.has_prev()) continue; // that does not begin here
        --note;                         // check out the object before the first note in line
        if (note->is(Class::NEWLINE))   // ignore non-newlines
        {
            note.remove();              // remove the newline object
            
            // change on-plate begin cursor (for correct reengraving)
            Plate::pLine::Iterator voice = pline->get_voice(cur->note.voice());
            if (voice != pline->voices.end())
                cur->pvoice->begin = voice->begin;
        };
    };
}

// convert pagebreak to newline
void EditCursor::remove_pagebreak()
{
    if (!ready()) throw NotValidException();        // check cursor
    if (page == pageset->pages.begin())             // if we are at the scores front
        return;                                     //     do nothing
    home();                                         // goto line front
    
    // for each voice
    for (std::list<VoiceCursor>::iterator cur = vcursors.begin(); cur != vcursors.end(); ++cur)
    {
        Cursor note(cur->note);
        if (!note.has_prev()) continue;     // that does not begin here
        --note;                             // check out the object before the first note in line
        if (note->is(Class::PAGEBREAK))     // ignore non-pagebreaks
        {
            // change pagebreak object to newline
            if (note.is_main())
                note.get_staffobject() = StaffObjectPtr(new Newline(static_cast<Pagebreak&>(*note)));
            else
                note.get_voiceobject() = VoiceObjectPtr(new Newline(static_cast<Pagebreak&>(*note)));
        };
    };
}

// remove newline, convert pagebreak
void EditCursor::remove_break()
{
    if (!plateinfo) throw NotValidException();      // check cursor
    if (line != plateinfo->plate->lines.begin())    // if we are not at the page front
        return remove_newline();                    //     remove newline
    else                                            // otherwise 
        return remove_pagebreak();                  //     remove pagebreak, create newline
}

//  stem comtrol
// --------------

// edit the current stem-length (setting type to STEM_CUSTOM)
void EditCursor::_add_stem_length(Chord& chord, const VoiceCursor&, const int pohh, int*) noexcept
{
    chord.stem.length += pohh;
}

void EditCursor::add_stem_length(spohh_t pohh)
{
    set_stem_type(Chord::STEM_CUSTOM);      // checks cursor
    for_each_chord_in_beam_do(*cursor, &_add_stem_length, pohh);
}

// set the current stem-length (setting type to STEM_CUSTOM)
void EditCursor::_set_stem_length(Chord& chord, const VoiceCursor&, const int pohh, int*) noexcept
{
    chord.stem.type = Chord::STEM_CUSTOM;
    chord.stem.length = pohh;
}

void EditCursor::set_stem_length(spohh_t pohh)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    if (!for_each_chord_in_beam_do(*cursor, &_set_stem_length, pohh))
        log_warn("Unable to find beam begin. (class: EditCursor)");
}

// edit the current stem-slope (setting type to SLOPE_CUSTOM)
void EditCursor::add_stem_slope(spohh_t pohh)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    
    // search for the first note within the beam
    VoiceCursor beam_begin;
    if (!get_beam_begin(beam_begin)) return;
    
    // set slope
    if (static_cast<Chord&>(*beam_begin.note).stem.slope_type != Chord::SLOPE_CUSTOM)
        set_slope_type(Chord::SLOPE_CUSTOM);
    static_cast<Chord&>(*beam_begin.note).stem.slope += pohh;
}

// set the current stem-slope (setting type to SLOPE_CUSTOM)
void EditCursor::set_stem_slope(int pohh)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    
    // search for the first note within the beam
    VoiceCursor beam_begin;
    if (!get_beam_begin(beam_begin)) return;
    
    // set slope
    if (static_cast<Chord&>(*beam_begin.note).stem.slope_type != Chord::SLOPE_CUSTOM)
        set_slope_type(Chord::SLOPE_CUSTOM);
    static_cast<Chord&>(*beam_begin.note).stem.slope = pohh;
}

// set the stem-direction (recalculate length, if custom)
void EditCursor::_set_stem_dir(Chord& chord, const VoiceCursor&, const int dir, int*)
{
    if (chord.stem.type == Chord::STEM_CUSTOM)
    {
        if ((dir < 0) != (chord.stem.length < 0))
            chord.stem.length = -chord.stem.length;
    }
    else
    {
        chord.stem.type = ((dir < 0) ? Chord::STEM_UP: Chord::STEM_DOWN);
        if (chord.stem.length < 0)
            chord.stem.length = -chord.stem.length;
    }
}

void EditCursor::set_stem_dir(bool down)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    if (!for_each_chord_in_beam_do(*cursor, &_set_stem_dir, down ? -1 : 1))
        log_warn("Unable to find beam begin. (class: EditCursor)");
}

// set the stem-type within the current beam group
void EditCursor::_set_stem_type(Chord& chord, const VoiceCursor& cursor, const int type, int*)
{
    switch (static_cast<const Chord::StemType>(type))
    {
    case Chord::STEM_CUSTOM:
        if (chord.stem.type == Chord::STEM_CUSTOM) break;
        chord.stem.length = static_cast<spohh_t>(((cursor.pnote->stem.base - cursor.pnote->stem.top) * 1000.0) / cursor.pvoice->head_height + .5);
        chord.stem.length -= chord.stem.length % 500;
        chord.stem.type = Chord::STEM_CUSTOM;
        break;
    case Chord::STEM_UP:
        chord.stem.type = Chord::STEM_UP;
        break;
    case Chord::STEM_DOWN:
        chord.stem.type = Chord::STEM_DOWN;
        break;
    case Chord::STEM_VOICE:
        chord.stem.type = Chord::STEM_VOICE;
        break;
    case Chord::STEM_AUTO:
        chord.stem.type = Chord::STEM_AUTO;
        break;
    default:
        break;
    };
}

void EditCursor::set_stem_type(const Chord::StemType type)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    if (!for_each_chord_in_beam_do(*cursor, &_set_stem_type, static_cast<int>(type)))
        log_warn("Unable to find beam begin. (class: EditCursor)");
}

// set the slope-type of the current beam group
void EditCursor::set_slope_type(const Chord::SlopeType type)
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end()) throw Cursor::IllegalObjectTypeException();
    
    // search for the first note within the beam
    VoiceCursor beam_begin;
    if (!get_beam_begin(beam_begin)) return;
    
    // check slope-type
    if (static_cast<Chord&>(*beam_begin.note).stem.slope_type == type)
        return;
    
    // set slope
    const Chord::StemType stem_type = static_cast<Chord&>(*beam_begin.note).stem.type;
    for_each_chord_in_beam_do(*cursor, &_set_stem_type, static_cast<int>(Chord::STEM_CUSTOM));
    static_cast<Chord&>(*beam_begin.note).stem.slope_type = type;
    if (type == Chord::SLOPE_CUSTOM)
        static_cast<Chord&>(*beam_begin.note).stem.slope = static_cast<spohh_t>(((beam_begin.pnote->stem.top - beam_begin.pnote->beam[VALUE_BASE - 3]->end->stem.top) * 1000.0) / beam_begin.pvoice->head_height + .5);
    else if (type == Chord::SLOPE_BOUNDED)
        static_cast<Chord&>(*beam_begin.note).stem.slope = get_style().beam_slope_max;
    for_each_chord_in_beam_do(*cursor, &_set_stem_type, static_cast<int>(stem_type));
}

//  accidental control
// --------------------

// set auto accidental to current object
void EditCursor::set_accidental_auto()
{
    if (!ready()) throw NotValidException();        // check cursor
    if (at_end() || !cursor->note->is(Class::CHORD))
        throw Cursor::IllegalObjectTypeException();
    
    Chord& chord = static_cast<Chord&>(*cursor->note);
    const StaffContext& ctx = get_staff_context();
    for (HeadList::iterator head = chord.heads.begin(); head != chord.heads.end(); ++head)
    {
        (*head)->accidental.type = ctx.guess_accidental((*head)->tone, param.prefer_natural);
    };
}


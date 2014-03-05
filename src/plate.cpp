
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

#include <iostream>     // std::cout

#include "plate.hh"     // Plate
#include "undefined.hh" // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                        // this number is interpreted as an undefined value
using namespace ScorePress;

inline int _round(const double d) {return static_cast<int>(d + 0.5);}

//
//     class Plate
//    =============
//
// Absolute positions and sprite ids for the easy and fast redering of a
// score. This structure will be created from a score by the engraver.
// It can be rendered to a device by a press (see "press.hh").
//

// box constructors
Plate_GphBox::Plate_GphBox() : pos(), width(0), height(0) {}
Plate_GphBox::Plate_GphBox(Plate::Pos p, mpx_t w, mpx_t h) : pos(p), width(w), height(h) {}

// check, if a given box overlaps this box
bool Plate_GphBox::overlaps(const Plate_GphBox& box)
{
    return ((contains(box.pos) || contains(Position<mpx_t>(box.right(), box.pos.y))
                               || contains(Position<mpx_t>(box.right(), box.bottom()))
                               || contains(Position<mpx_t>(box.pos.x, box.bottom())))
        ||  (box.contains(pos) || box.contains(Position<mpx_t>(right(), pos.y))
                               || box.contains(Position<mpx_t>(right(), bottom()))
                               || box.contains(Position<mpx_t>(pos.x, bottom()))));
}

// extend box, such that the given point is covered
void Plate_GphBox::extend(const Plate::Pos& p)
{
    if (p.x < pos.x)
    {
        width += pos.x - p.x;
        pos.x = p.x;
    };
    if (p.x > pos.x + width)
    {
        width = p.x - pos.x;
    };
    if (p.y < pos.y)
    {
        height += pos.y - p.y;
        pos.y = p.y;
    };
    if (p.y > pos.y + height)
    {
        height = p.y - pos.y;
    };
}

// extend box, such that the given box is covered
void Plate_GphBox::extend(const Plate_GphBox& box)
{
    if (box.pos.x < pos.x)
    {
        width += pos.x - box.pos.x;
        pos.x = box.pos.x;
    };
    if (box.pos.x + box.width > pos.x + width)
    {
        width = box.pos.x - pos.x + box.width;
    };
    if (box.pos.y < pos.y)
    {
        height += pos.y - box.pos.y;
        pos.y = box.pos.y;
    };
    if (box.pos.y + box.height > pos.y + height)
    {
        height = box.pos.y - pos.y + box.height;
    };
}

// graphical object constructor
Plate_pGraphical::Plate_pGraphical(Plate::Pos pos, mpx_t width, mpx_t height) : gphBox(pos, width, height) {}

// attachable constructor
Plate_pAttachable::Plate_pAttachable(const AttachedObject& obj, const Plate::Pos& pos) : object(&obj), absolutePos(pos)
{
    flipped.x = flipped.y = 0;
}

// durable constructor
Plate_pDurable::Plate_pDurable(const AttachedObject& obj, const Plate::Pos& pos) : Plate_pAttachable(obj, pos) {}

// virtual note constructor
Plate_pNote::Virtual::Virtual(const StaffObject& _object, bool _inserted) : object(_object.clone()), inserted(_inserted) {}

// note object constructor
Plate_pNote::Plate_pNote(const Plate::Pos& pos, const const_Cursor& n) : note(n), virtual_obj(), stem_info(), noflag(false)
{
    absolutePos.push_back(pos); // append top pos
    stem.x = pos.x;             // initialize zero length stem at pos
    stem.top = pos.y;
    stem.base = pos.y;
    stem.beam_off = 0;
}

// add offset to all positions (except to the tie-end)
void Plate_pNote::add_offset(mpx_t offset)
{
    for (PositionList::iterator p = absolutePos.begin(); p != absolutePos.end(); ++p)
    {
        p->x += offset;
    };
    for (PositionList::iterator p = dotPos.begin(); p != dotPos.end(); ++p)
    {
        p->x += offset;
    };
    for (AttachableList::iterator a = attached.begin(); a != attached.end(); ++a)
    {
        (*a)->absolutePos.x += offset;
        (*a)->gphBox.pos.x += offset;
    };
    for (TieList::iterator t = ties.begin(); t != ties.end(); ++t)
    {
        t->pos1.x += offset;
        t->control1.x += offset;
    };
    stem.x += offset;
    for (LedgerLineList::iterator l = ledgers.begin(); l != ledgers.end(); ++l)
    {
        l->basepos.x += offset;
    };
    gphBox.pos.x += offset;
}

// add offset to tie-end positions
void Plate_pNote::add_tieend_offset(mpx_t offset)
{
    for (std::list<Tie>::iterator t = ties.begin(); t != ties.end(); ++t)
    {
        t->pos2.x += offset;
        t->control2.x += offset;
    };
}

// dump pnote info
void Plate_pNote::dump() const
{
    std::cout << "address      " << this << "\n";
    std::cout << "type         " << (at_end() ? "[E]" : classname(get_note().classtype())) << "\n";
    std::cout << "sprite       (" << sprite.setid << ", " << sprite.spriteid << ")\n";
    std::cout << "absolutePos ";
    for (PositionList::const_iterator i = absolutePos.begin(); i != absolutePos.end(); ++i)
        std::cout << " (" << i->x << ", " << i->y << ")";
    std::cout << "\n";
    if (!dotPos.empty())
    {
        std::cout << "dotPos      ";
        for (PositionList::const_iterator i = dotPos.begin(); i != dotPos.end(); ++i)
            std::cout << " (" << i->x << ", " << i->y << ")";
        std::cout << "\n";
    };
    if (!ledgers.empty())
    {
        std::cout << "ledgers    ";
        for (LedgerLineList::const_iterator i = ledgers.begin(); i != ledgers.end(); ++i)
            std::cout << "  " << i->count << " x (" << i->basepos.x << ", " << i->basepos.y << ") - " << i->length << (i->below ? " v" : " ^");
        std::cout << "\n";
    };
    std::cout << "virtual      " << (virtual_obj ? (virtual_obj->inserted ? "Yes (inserted)\n" : "Yes\n") : "No\n");
    std::cout << "stem         (" << stem.x << ", " << stem.top << ") - (" << stem.x << ", " << stem.base << ") : " << stem.beam_off << "\n";
    for (size_t i = 0; i < VALUE_BASE - 2; ++i)
    {
        if (beam[i])
        {
            std::cout << "beam " << ((i > VALUE_BASE - 7) ? " " : "") << ((i > VALUE_BASE - 4) ? " " : "") << (1 << (VALUE_BASE - i));
            std::cout << ":    " << beam[i]->end << " ~ idx " << beam[i]->end_idx << "  ";
            std::cout << (beam[i]->short_beam ? "S" : "") << (beam[i]->short_left ? "L" : "");
            std::cout << "\n";
        };
    };
    std::cout << "noflag       " << (noflag  ? "True" : "False") << "\n\n";
}

// voice constructor
Plate_pVoice::Plate_pVoice(const const_Cursor& cursor) : begin(cursor) {}

// find the given voice in this line
Plate_pLine::Iterator Plate_pLine::get_voice(const Voice& voice)
{
    for (VoiceList::iterator i = voices.begin(); i != voices.end(); ++i)    // check each on-plate voice
    {
        if (&i->begin.voice() == &voice) return i;  // if it refers to the given voice, return
    };
    return voices.end();    // if it cannot be found, return empty iterator
}

// find the given voice in this line (constant version)
Plate_pLine::const_Iterator Plate_pLine::get_voice(const Voice& voice) const
{
    for (VoiceList::const_iterator i = voices.begin(); i != voices.end(); ++i)  // check each on-plate voice
    {
        if (&i->begin.voice() == &voice) return i;  // if it refers to the given voice, return
    };
    return voices.end();    // if it cannot be found, return empty iterator
}

// find any voice of a given staff
Plate_pLine::Iterator Plate_pLine::get_staff(const Staff& staff)
{
    for (VoiceList::iterator i = voices.begin(); i != voices.end(); ++i) // check each on-plate voice
    {
        if (&i->begin.staff() == &staff) return i;      // if it refers to the given voice, return
    };
    return voices.end();    // if it cannot be found, return empty iterator
}

// find any voice of a given staff (constant version)
Plate_pLine::const_Iterator Plate_pLine::get_staff(const Staff& staff) const
{
    for (VoiceList::const_iterator i = voices.begin(); i != voices.end(); ++i) // check each on-plate voice
    {
        if (&i->begin.staff() == &staff) return i;      // if it refers to the given voice, return
    };
    return voices.end();    // if it cannot be found, return empty iterator
}

// calculate graphical boundary box
void Plate_pLine::calculate_gphBox()
{
    // setup basic reference points
    noteBox.pos = basePos;
    noteBox.width = noteBox.height = 0;
    noteBox.extend(Position<mpx_t>(line_end, basePos.y));
    
    // calculate note-box
    for (Plate::VoiceIt voice = voices.begin(); voice != voices.end(); ++voice)
        for (Plate::NoteIt note = voice->notes.begin(); note != voice->notes.end(); ++note)
            noteBox.extend(note->gphBox);
    
    // copy note-box and extend to contain all attaced objects
    gphBox = noteBox;
    for (Plate::VoiceIt voice = voices.begin(); voice != voices.end(); ++voice)
        for (Plate::NoteIt note = voice->notes.begin(); note != voice->notes.end(); ++note)
            for (Plate::pNote::AttachableList::iterator a = note->attached.begin(); a != note->attached.end(); ++a)
                gphBox.extend((*a)->gphBox);
}

// dump the plate content to stdout
void Plate::dump() const
{
    size_t i = 0;
    size_t j = 0;
    size_t k = 0;
    for (LineList::const_iterator l = lines.begin(); l != lines.end(); ++l)
    {
        std::cout << "Line " << i++ << ": (" << l->basePos.x << ", " << l->basePos.y << ")  " <<  &l->voices << "\n";
        j = 0;
        for (VoiceList::const_iterator v = l->voices.begin(); v != l->voices.end(); ++v)
        {
            std::cout << "    Voice " << j++ << ": (" << v->basePos.x << ", " << v->basePos.y << ") t=" << v->time << "  " << &*v << "\n";
            k = 0;
            for (NoteList::const_iterator n = v->notes.begin(); n != v->notes.end(); ++n)
            {
                if (n->at_end())
                    std::cout << "        Note " << k++ << " @EOV";
                else
                    std::cout << "        Note " << k++ << " @" << &n->get_note() << "\t" << classname(n->get_note().classtype()) << "\t(" << n->sprite.setid << ", " << n->sprite.spriteid << ")";
                if (n->virtual_obj)
                {
                    std::cout << " [V";
                    if (n->virtual_obj->inserted) std::cout << "I";
                    std::cout << "]";
                };
                std::cout << "\n";
                std::cout << "            ";
                for (Plate_pNote::PositionList::const_iterator p = n->absolutePos.begin(); p!= n->absolutePos.end(); ++p)
                {
                    std::cout << "(" << p->x / 1000.0 << "," << p->y / 1000.0 << ") ";
                };
                std::cout << "\n";
            };
        };
    };
}

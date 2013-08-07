
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

#include <iostream>     // std::cout

#include "plate.hh"     // Plate
#include "undefined.hh" // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                        // this number is interpreted as an undefined value
using namespace ScorePress;


//
//     class Plate
//    =============
//
// Absolute positions and sprite ids for the easy and fast redering of a
// score. This structure will be created from a score by the engraver.
// It can be rendered to a device by a press (see "press.hh").
//

// box constructors
Plate::pGraphical::Box::Box() : pos(), width(0), height(0) {}
Plate::pGraphical::Box::Box(Position<mpx_t> p, mpx_t w, mpx_t h) : pos(p), width(w), height(h) {}

// check, if a given point is inside the box
bool Plate::pGraphical::Box::contains(const Position<mpx_t>& p) const
{
    return (p.x >= pos.x && p.y >= pos.y && p.x < pos.x + width && p.y < pos.y + height);
}

// extend box, such that the given point is covered
void Plate::pGraphical::Box::extend(const Position<mpx_t>& p)
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
void Plate::pGraphical::Box::extend(const Plate::pGraphical::Box& box)
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
Plate::pGraphical::pGraphical(Position<mpx_t> pos, mpx_t width, mpx_t height) : gphBox(pos, width, height) {}

// check if a point is within a graphical object
bool Plate::pGraphical::contains(Position<mpx_t> p) const
{
    return gphBox.contains(p);
}

// attachable constructor
Plate::pAttachable::pAttachable(const Class& obj, const Position<mpx_t>& pos) : object(&obj), absolutePos(pos) {}

// durable constructor
Plate::pDurable::pDurable(const Class& obj, const Position<mpx_t>& pos) : pAttachable(obj, pos) {}

// virtual note constructor
Plate::pNote::Virtual::Virtual(const StaffObject& _object, bool _inserted) : object(_object.clone()), inserted(_inserted) {}

// note object constructor
Plate::pNote::pNote(const Position<mpx_t>& pos, const const_Cursor& n) : note(n), virtual_obj(), stem_info(), noflag(false)
{
    absolutePos.push_back(pos); // append top pos
    stem.x = pos.x;             // initialize zero length stem at pos
    stem.top = pos.y;
    stem.base = pos.y;
    stem.beam_off = 0;
}

// add offset to all positions (except to the tie-end)
void Plate::pNote::add_offset(mpx_t offset)
{
    for (PositionList::iterator p = absolutePos.begin(); p != absolutePos.end(); ++p)
    {
        p->x += offset;
    };
    for (PositionList::iterator p = dotPos.begin(); p != dotPos.end(); ++p)
    {
        p->x += offset;
    };
    for (AttachableList::iterator a = attachables.begin(); a != attachables.end(); ++a)
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
void Plate::pNote::add_tieend_offset(mpx_t offset)
{
    for (std::list<pNote::Tie>::iterator t = ties.begin(); t != ties.end(); ++t)
    {
        t->pos2.x += offset;
        t->control2.x += offset;
    };
}

// dump pnote info
void Plate::pNote::dump() const
{
    std::cout << "address      " << this << "\n";
    std::cout << "type         " << classname(get_note().classtype()) << "\n";
    std::cout << "sprite       (" << sprite.setid << ", " << sprite.spriteid << ")\n";
    std::cout << "absolutePos ";
    for (PositionList::const_iterator i = absolutePos.begin(); i != absolutePos.end(); ++i)
        std::cout << " (" << i->x << ", " << i->y << ")";
    std::cout << "\n";
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
Plate::pVoice::pVoice(const const_Cursor& cursor) : begin(cursor) {}

// find the given voice in this line
std::list<Plate::pVoice>::iterator Plate::pLine::get_voice(const Voice& voice)
{
    for (std::list<pVoice>::iterator i = voices.begin(); i != voices.end(); ++i)    // check each on-plate voice
    {
        if (&i->begin.voice() == &voice) return i;  // if it refers to the given voice, return
    };
    return voices.end();    // if it cannot be found, return empty iterator
}

// find the given voice in this line (constant version)
std::list<Plate::pVoice>::const_iterator Plate::pLine::get_voice(const Voice& voice) const
{
    for (std::list<pVoice>::const_iterator i = voices.begin(); i != voices.end(); ++i)  // check each on-plate voice
    {
        if (&i->begin.voice() == &voice) return i;  // if it refers to the given voice, return
    };
    return voices.end();    // if it cannot be found, return empty iterator
}

// find any voice of a given staff
std::list<Plate::pVoice>::iterator Plate::pLine::get_staff(const Staff& staff)
{
    for (std::list<pVoice>::iterator i = voices.begin(); i != voices.end(); ++i)    // check each on-plate voice
    {
        if (&i->begin.staff() == &staff) return i;  // if it refers to the given voice, return
    };
    return voices.end();    // if it cannot be found, return empty iterator
}

// find any voice of a given staff (constant version)
std::list<Plate::pVoice>::const_iterator Plate::pLine::get_staff(const Staff& staff) const
{
    for (std::list<pVoice>::const_iterator i = voices.begin(); i != voices.end(); ++i)  // check each on-plate voice
    {
        if (&i->begin.staff() == &staff) return i;  // if it refers to the given voice, return
    };
    return voices.end();    // if it cannot be found, return empty iterator
}

// dump the plate content to stdout
void Plate::dump() const
{
    size_t i = 0;
    size_t j = 0;
    size_t k = 0;
    for (std::list<pLine>::const_iterator l = lines.begin(); l != lines.end(); ++l)
    {
        std::cout << "Line " << i++ << ": (" << l->basePos.x << ", " << l->basePos.y << ")\n";
        j = 0;
        for (std::list<Plate::pVoice>::const_iterator v = l->voices.begin(); v != l->voices.end(); ++v)
        {
            std::cout << "    Voice " << j++ << ": (" << v->basePos.x << ", " << v->basePos.y << ") t=" << v->time << "\n";
            k = 0;
            for (std::list<Plate::pNote>::const_iterator n = v->notes.begin(); n != v->notes.end(); ++n)
            {
                std::cout << "        Note " << k++ << " @" << &n->get_note() << "\t" << classname(n->get_note().classtype()) << "\t(" << n->sprite.setid << ", " << n->sprite.spriteid << ")";
                if (n->virtual_obj)
                {
                    std::cout << " [V";
                    if (n->virtual_obj->inserted) std::cout << "I";
                    std::cout << "]";
                };
                std::cout << "\n";
                std::cout << "            ";
                for (std::list< Position<mpx_t> >::const_iterator p = n->absolutePos.begin(); p!= n->absolutePos.end(); ++p)
                {
                    std::cout << "(" << p->x / 1000.0 << "," << p->y / 1000.0 << ") ";
                };
                std::cout << "\n";
            };
        };
    };
}


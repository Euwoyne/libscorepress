
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

#include "object_cursor.hh"
#include "engraver_state.hh"    // EngraverState
#include "press_state.hh"       // PressState
#include "renderer.hh"          // Renderer
#include "edit_cursor.hh"       // EditCursor

using namespace ScorePress;

// exception classes
ObjectCursor::Error::Error(const std::string& msg) : ScorePress::Error(msg) {}
ObjectCursor::NotValidException::NotValidException()
        : Error("Unable to dereference invalid object-cursor.") {}

// virtual destructor
ObjectCursor::~ObjectCursor() {}

// sets "object" and "pobject" to the first attached object
bool ObjectCursor::setup()
{
    object = list->begin();
    pobject = plist->begin();
    if (object == list->end()) return false;
    while (pobject != plist->end() && (*pobject)->object != &**object)
        ++pobject;
    if (pobject != plist->end())
        return true;
    object = list->end();
    return false;
}

// constructor
ObjectCursor::ObjectCursor(Document& _document, Pageset& _pageset)
    : document(&_document), pageset(&_pageset), score(NULL), pageno(0), pline(NULL), pvoice(NULL), pnote(NULL), list(NULL), plist(NULL) {}
/*
// default constructor
ObjectCursor::ObjectCursor() : document(NULL), score(NULL), pageno(0), pline(NULL), pvoice(NULL), pnote(NULL), list(NULL), plist(NULL) {}

// set page as parent for objects
ObjectCursor::ObjectCursor(Document& _document, Pageset::pPage& page) :
        document(&_document), score(NULL), pageno(page.pageno-1), pline(NULL), pvoice(NULL), pnote(NULL), list(&document->attached[page.pageno-1]), plist(&page.attached)
{
    setup();
}

// set miscellaneous lists as parent data
ObjectCursor::ObjectCursor(Document& _document, Score& _score, VisibleObject& _note, Plate::pNote& _pnote, Plate::pVoice& _pvoice, Plate::pLine& _pline, unsigned int _pageno) :
        document(&_document), score(&_score), pageno(_pageno), pline(&_pline), pvoice(&_pvoice), pnote(&_pnote), list(&_note.attached), plist(&_pnote.attached)
{
    setup();
}
*/

// set page as parent for objects
bool ObjectCursor::set_parent(Pageset::pPage& _page)
{
    score  = NULL;
    pageno = _page.pageno - 1;
    pline  = NULL;
    pvoice = NULL;
    pnote  = NULL;
    list   = &document->attached[pageno];
    plist  = &_page.attached;
    return setup();
}

// set note as parent for objects
bool ObjectCursor::set_parent(EditCursor& cursor)
{
    if (&cursor.get_document() != document || &cursor.get_pageset() != pageset)
        return false;
    score  = &cursor.get_score();
    pageno = cursor.get_page().pageno - 1;
    pline  = &cursor.get_line();
    pvoice = &cursor.get_pvoice();
    pnote  = &cursor.get_platenote();
    list   = &cursor.get_attached();
    plist  = &pnote->attached;
    return setup();
}

// set miscellaneous lists as parent data
bool ObjectCursor::set_parent(Pageset::pPage& _page, Score& _score, VisibleObject& _note, Plate::pNote& _pnote, Plate::pVoice& _pvoice, Plate::pLine& _pline)
{
    score  = &_score;
    pageno = _page.pageno - 1;
    pline  = &_pline;
    pvoice = &_pvoice;
    pnote  = &_pnote;
    list   = &_note.attached;
    plist  = &_pnote.attached;
    return setup();
}

// select target object (must be within set parent)
bool ObjectCursor::select(const Movable& obj)
{
    if (!list || !plist) return false;
    object = list->begin();
    while (object != list->end() && &**object != &obj)
        ++object;
    if (object == list->end())
    {
        pobject = plist->end();
        return false;
    };
    while (pobject != plist->end() && (*pobject)->object != &**object)
        ++pobject;
    if (pobject != plist->end())
        return true;
    object = list->end();
    return false;
}

// select object by position
bool ObjectCursor::select(Position<mpx_t> pos, Pageset::pPage& page)
{
    // buffer cursor
    ObjectCursor buffer(*document, *pageset);
    
    // set on-page root (substract page margin)
    pos.x -= pageset->page_layout.margin.left;
    pos.y -= pageset->page_layout.margin.top;
    
    // search for object on page
    for (Pageset::pPage::AttachableList::iterator a = page.attached.begin(); a != page.attached.end(); ++a)
    {
        if (!(*a)->object->is(Class::MOVABLE)) continue;
        if ((*a)->contains(pos))
        {
            buffer.set_parent(page);
            buffer.select(*static_cast<const Movable*>((*a)->object));
            if (!buffer.ready() || buffer.end())
                return false;
            *this = buffer;
            return true;
        };
    };
    
    // search for object within plates
    Pageset::pPage::const_Iterator pinfo = page.get_plate_by_pos(pos);
    if (pinfo == page.plates.end())
        pinfo = --page.plates.end();
    pageno = pinfo->pageno - 1;             // set pagenumber
    pos -= pinfo->dimension.position;       // calculate position relative to plate
    
    // search the score
    Document::ScoreList::iterator score_it = document->scores.begin();
    for (; score_it != document->scores.end(); ++score_it)
        if (&score_it->score == pinfo->score)
            break;
    if (score_it == document->scores.end()) return false;
    buffer.score = &score_it->score;        // set score pointer
    
    // search the object
    for (Plate::LineIt l = pinfo->plate->lines.begin(); l != pinfo->plate->lines.end(); ++l)
    {
        if (!l->contains(pos)) continue;
        for (Plate::VoiceIt v = l->voices.begin(); v != l->voices.end(); ++v)
        {
            for (Plate::NoteIt n = v->notes.begin(); n != v->notes.end() && !n->at_end(); ++n)
            {
                if (n->is_inserted()) continue;
                if (!n->get_note().is(Class::VISIBLEOBJECT)) continue;
                for (Plate::pNote::AttachableList::iterator a = n->attached.begin(); a != n->attached.end(); ++a)
                {
                    if (!(*a)->object->is(Class::MOVABLE)) continue;
                    if ((*a)->contains(pos))
                    {
                        // (casts away const, but not unexpected, since this object got a non-const reference to the document)
                        buffer.pline  = &*l;
                        buffer.pvoice = &*v;
                        buffer.pnote  = &*n;
                        buffer.list   = &const_cast<StaffObject&>(n->get_note()).get_visible().attached;
                        buffer.plist  = &n->attached;
                        if (buffer.setup() && buffer.select(*static_cast<const Movable*>((*a)->object)))
                        {
                            *this = buffer;
                            return true;
                        };
                    };
                };
            };
        };
    };
    
    // no object found
    return false;
}

// goto next object attached to the parent note
bool ObjectCursor::next()
{
    if (object == list->end())
    {
        object = list->begin();
        pobject = plist->begin();
    }
    else ++object;
    if (object == list->end())
    {
        pobject = plist->end();
        return false;
    };
    while (pobject != plist->end() && (*pobject)->object != &**object)
        ++pobject;
    if (pobject != plist->end())
        return true;
    object = list->end();
    return false;
}

// return referenced object
Movable& ObjectCursor::get_object() const
{
    if (!list || !plist || object == list->end() || pobject == plist->end()) throw NotValidException();
    return **object;
}

// return on-plate object
Plate::pAttachable& ObjectCursor::get_pobject() const
{
    if (!list || !plist || object == list->end() || pobject == plist->end()) throw NotValidException();
    return **pobject;
}

// rendering interface
void ObjectCursor::render(Renderer& renderer, const PressState& state) const
{
    if (!list || !plist || object == list->end() || pobject == plist->end()) throw NotValidException();
    
    (**object).render_decor(renderer, **pobject, state);
    
    const Position<mpx_t> origin((/*move_offset.x +*/ state.scale((**pobject).gphBox.pos.x + (**pobject).gphBox.right())  / 2 < state.scale(pnote->gphBox.pos.x)) ? (**pobject).gphBox.right()  : (**pobject).gphBox.pos.x,
                                 (/*move_offset.y +*/ state.scale((**pobject).gphBox.pos.y + (**pobject).gphBox.bottom()) / 2 < state.scale(pnote->gphBox.pos.y)) ? (**pobject).gphBox.bottom() : (**pobject).gphBox.pos.y);
    
    renderer.set_line_width(1.0);
    renderer.move_to((state.scale(pnote->gphBox.pos.x) + state.offset.x) / 1000.0, (state.scale(pnote->gphBox.pos.y) + state.offset.y) / 1000.0);
    renderer.line_to((state.scale(origin.x)            + state.offset.x) / 1000.0, (state.scale(origin.y)            + state.offset.y) / 1000.0);
    renderer.stroke();
    renderer.move_to((state.scale(origin.x)     + state.offset.x) / 1000.0, (state.scale(origin.y)     + state.offset.y) / 1000.0);
    renderer.line_to((state.scale(origin.x + 3) + state.offset.x) / 1000.0, (state.scale(origin.y)     + state.offset.y) / 1000.0);
    renderer.line_to((state.scale(origin.x + 3) + state.offset.x) / 1000.0, (state.scale(origin.y + 3) + state.offset.y) / 1000.0);
    renderer.line_to((state.scale(origin.x)     + state.offset.x) / 1000.0, (state.scale(origin.y + 3) + state.offset.y) / 1000.0);
    renderer.fill();
}

// setup reengraving trigger
void ObjectCursor::setup_reengrave(ReengraveInfo& info)
{
    if (!pnote) throw NotValidException();
    info.setup_reengrave(**object, *this);
}

// reengraving function
Reengraveable::Status ObjectCursor::reengrave(EngraverState& state)
{
    pline   = &state.get_target_line();
    pvoice  = &state.get_target_voice();
    pnote   = &state.get_target();
    plist   = &state.get_target().attached;
    pobject = plist->begin();
    while (pobject != plist->end() && (*pobject)->object != &**object)
        ++pobject;
    if (pobject == plist->end()) setup();
    return DONE;
}

// reengrave finishing function (NOOP)
void ObjectCursor::finish_reengrave() {}

/*
// prepare movement (returns, if offset changed)
bool ObjectCursor::prepare_move(Plate::Pos offset, Grid)
{
    move_offset = offset;
    new_position.x = object->position.x + ((object->unitX == Movable::METRIC) ? viewport.pxtoum_h(offset.x) : (1000 * move_offset.x) / pvoice->head_height);
    new_position.y = object->position.y + ((object->unitY == Movable::METRIC) ? viewport.pxtoum_v(offset.y) : (1000 * move_offset.y) / pvoice->head_height);
}

// execute movement (returns, if there was any)
bool ObjectCursor::execute_move(const ViewportParam& viewport)
{
    // check if there is any movement
    if (!move_offset.x && !move_offset.y) return false;
    
    // apply offset to on-plate data (no reengave neccessary)
    (*pobject)->absolutePos += move_offset;
    if ((*pobject)->is_durable())
        static_cast<Plate::pDurable&>(**pobject).endPos += move_offset;
    (*pbject)->gphBox.pos += move_offset;
    
    // apply offset to score data
    if (object->is(Class::DURABLE))
        static_cast<Durable&>(*object).end_position += (new_position - object->position);
    object->position = new_position;
    
    // update the line's boundary box (casts away const, but not unexpected, since this object got a non-const reference to the pageset)
    pline->calculate_gphBox();
    
    // reset offsets
    move_offset.x = 0; move_offset.y = 0; real_offset.x = 0; real_offset.y = 0;
    return true;
}
*/


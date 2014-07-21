
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

#include "object_cursor.hh"
#include "engraver_state.hh" // EngraverState

using namespace ScorePress;

// exception classes
ObjectCursor::Error::Error(const std::string& msg) : ScorePress::Error(msg) {}
ObjectCursor::NotValidException::NotValidException()
        : Error("Unable to dereference invalid object-cursor.") {}

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

// default constructor
ObjectCursor::ObjectCursor() : pageno(0), pline(NULL), pvoice(NULL), pnote(NULL), list(NULL), plist(NULL) {}

// set page as parent for objects
ObjectCursor::ObjectCursor(Document& document, Pageset::pPage& page) :
        pageno(page.pageno-1), pline(NULL), pvoice(NULL), pnote(NULL), list(&document.attached[page.pageno-1]), plist(&page.attached)
{
    setup();
}

// set miscellaneous lists as parent data
ObjectCursor::ObjectCursor(VisibleObject& _note, Plate::pNote& _pnote, Plate::pVoice& _pvoice, Plate::pLine& _pline, unsigned int _pageno) :
        pageno(_pageno), pline(&_pline), pvoice(&_pvoice), pnote(&_pnote), list(&_note.attached), plist(&_pnote.attached)
{
    setup();
}

// set page as parent for objects
bool ObjectCursor::set_parent(Document& document, Pageset::pPage& page)
{
    pageno = page.pageno - 1;
    pline  = NULL;
    pvoice = NULL;
    pnote  = NULL;
    list   = &document.attached[pageno];
    plist  = &page.attached;
    return setup();
}

// set miscellaneous lists as parent data
bool ObjectCursor::set_parent(VisibleObject& _note, Plate::pNote& _pnote, Plate::pVoice& _pvoice, Plate::pLine& _pline, unsigned int _pageno)
{
    pageno = _pageno;
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


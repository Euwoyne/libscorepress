
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
ObjectCursor::ObjectCursor() : pageno(0), list(NULL), plist(NULL) {}

// set note as parent for objects
ObjectCursor::ObjectCursor(EditCursor& cursor) : pageno(cursor.get_pageno()),
                                                 list(&cursor.get_attached()),
                                                 plist(&cursor.get_pattached()),
                                                 pline(&cursor.get_line()),
                                                 parent(&cursor.get_platenote())
{
    setup();
}

// set page as parent for objects
ObjectCursor::ObjectCursor(Document& document, Pageset::pPage& page) : pageno(page.pageno-1),
                                                                       list(&document.attached[page.pageno-1]),
                                                                       plist(&page.attached),
                                                                       pline(NULL),
                                                                       parent(NULL)
{
    setup();
}

// set miscellaneous lists as parent data
ObjectCursor::ObjectCursor(VisibleObject& note, Plate::pNote& pnote, unsigned int _pageno) :
        pageno(_pageno), list(&note.attached), plist(&pnote.attached), parent(&pnote)
{
    setup();
}

// set note as parent for objects
bool ObjectCursor::set_parent(EditCursor& cursor)
{
    pageno = cursor.get_pageno();
    list = &cursor.get_attached();
    plist = &cursor.get_pattached();
    pline = &cursor.get_line();
    parent = &cursor.get_platenote();
    return setup();
}

// set page as parent for objects
bool ObjectCursor::set_parent(Document& document, Pageset::pPage& page)
{
    pageno = page.pageno - 1;
    list = &document.attached[pageno];
    plist = &page.attached;
    pline = NULL;
    parent = NULL;
    return setup();
}

// set miscellaneous lists as parent data
bool ObjectCursor::set_parent(VisibleObject& note, Plate::pNote& pnote, Plate::pLine& line, unsigned int _pageno)
{
    pageno = _pageno;
    list = &note.attached;
    plist = &pnote.attached;
    pline = &line;
    parent = &pnote;
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

// reengraving function
bool ObjectCursor::reengrave(EngraverState& state)
{
    plist = &state.get_target().attached;
    pline = &state.get_target_line();
    pobject = plist->begin();
    while (pobject != plist->end() && (*pobject)->object != &**object)
        ++pobject;
    return false;
}

// setup reengraving trigger
void ObjectCursor::setup_reengrave(ReengraveInfo& info)
{
    if (!parent) throw NotValidException();
    info.setup_reengrave(**object, *this);
}

// reengrave finishing function (NOOP)
void ObjectCursor::finish_reengrave() {}


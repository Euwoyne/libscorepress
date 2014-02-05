
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

#ifndef OBJECT_CURSOR_HH
#define OBJECT_CURSOR_HH

#include "edit_cursor.hh"   // EditCursor, Document
#include "error.hh"         // Error
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class ObjectCursor;     // cursor referencing attachable objects


//
//     class ObjectCursor
//    ====================
//
// This cursor points to the instance and corresponding on-plate data of
// Objects of type Movable.
//
class SCOREPRESS_API ObjectCursor
{
 public:
    // exception classes
    class SCOREPRESS_API Error : public ScorePress::Error {public: Error(const std::string& msg);};
    class SCOREPRESS_API NotValidException : public Error            // thrown, if an invalid cursor is dereferenced
        {public: NotValidException(); NotValidException(const std::string& msg);};
    
 protected:
    typedef Plate::pNote::AttachableList AttachableList;
    
    unsigned int             pageno;    // page number
    MovableList*             list;      // source Movable list (on a Note, page or Document)
    AttachableList*          plist;     // source Attachable list (on a Plate or pPage)
    MovableList::iterator    object;    // Movable object (as iterator on "note")
    AttachableList::iterator pobject;   // on-plate data (as iterator on "pnote")
    
 protected:
    bool setup();                       // sets "pobject" and "pobject" to the first attached object
    
 public:
    // constructors
    ObjectCursor();
    ObjectCursor(EditCursor& cursor);
    ObjectCursor(Document& document, Pageset::pPage& page);
    ObjectCursor(MovableList& list, Plate::pNote::AttachableList& plist, unsigned int pageno);
    
    // set parent note
    bool set_parent(EditCursor& cursor);                        // set note as parent for objects
    bool set_parent(Document& document, Pageset::pPage& page);  // set page as parent for objects
    bool set_parent(MovableList& list, Plate::pNote::AttachableList& plist, unsigned int pageno);
    
    // select target object (must be within set parent)
    bool select(const Movable& object);
    
    // iterator interface
    bool next();                            // goto next object attached to the parent note
    void reset();                           // reset cursor to invalid state
    bool ready() const;                     // check, if cursor is valid
    bool end() const;                       // check, if the iterator is at end
    
    // object interface
    unsigned int        get_pageno()  const;    // return target page number
    Movable&            get_object()  const;    // return referenced object
    Plate::pAttachable& get_pobject() const;    // return on-plate object
};

// inline methods
inline void         ObjectCursor::reset()            {list = NULL; plist = NULL;}
inline bool         ObjectCursor::ready()      const {return list && plist;}
inline bool         ObjectCursor::end()        const {return object == list->end() && pobject == plist->end();}
inline unsigned int ObjectCursor::get_pageno() const {return pageno;}

} // end namespace

#endif


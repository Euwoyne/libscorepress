
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

#ifndef SCOREPRESS_OBJECTCURSOR_HH
#define SCOREPRESS_OBJECTCURSOR_HH

#include "cursor_base.hh"       // CursorBase, Reengraveable, PressState
#include "pageset.hh"           // Pageset, Plate, Document, ViewportParam
#include "error.hh"             // Error
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class ObjectCursor;     // cursor referencing attachable objects

// PROTOTYPES
class Renderer;         // renderer class prototype (see "renderer.hh")
class Document;         // document class prototype (see "document.hh")
class Score;            // score class prototype (see "score.hh")
class EditCursor;       // edit-cursor prototype (see "edit_cursor.hh")


//
//     class ObjectCursor
//    ====================
//
// This cursor points to the instance and corresponding on-plate data of
// Objects of type Movable.
//
class SCOREPRESS_API ObjectCursor : public CursorBase
{
 public:
    // exception classes
    class SCOREPRESS_API Error : public ScorePress::Error {public: Error(const std::string& msg);};
    class SCOREPRESS_API NotValidException : public Error            // thrown, if an invalid cursor is dereferenced
        {public: NotValidException(); NotValidException(const std::string& msg);};
    
    // positioning mode
    class Grid
    {
     public:
        // metric grids (um, mm, cm)
        enum Metric {NO_GRID = 0, CENTI = 1, MILLI = 2, MICRO = 3};
        
        // relative grids (head-height, half head-height)
        enum Head   {NO_SNAP = 0, FULL  = 1, HALF = 2};
        
     private:
        Metric metric;
        Head   head;
        
     public:
        // constructors
        inline Grid()                   : metric(NO_GRID), head(NO_SNAP) {}
        inline Grid(Metric m)           : metric(m),       head(NO_SNAP) {}
        inline Grid(Metric m, Head   h) : metric(m),       head(h)       {}
        inline Grid(Head   h)           : metric(NO_GRID), head(h)       {}
        inline Grid(Head   h, Metric m) : metric(m),       head(h)       {}
        
        // grid manipulation
        inline void set(Metric m)           {metric = m;}
        inline void set(Metric m, Head   h) {metric = m; head = h;}
        inline void set(Head   h)           {head = h;}
        inline void set(Head   h, Metric m) {head = h; metric = m;}
        
        // grid check
        inline bool has(Metric m) const     {return metric >= m;}
        inline bool has(Head   h) const     {return head   >= h;}
    };
    
 protected:
    typedef Plate::pNote::AttachableList AttachableList;
    
    // object location data
    Document*                document;  // document
    Pageset*                 pageset;   // page-set
    Score*                   score;     // score        (if not on-page)
    size_t                   pageno;    // page number
    
    Plate::pLine*            pline;     // parent line  (if not on-page)
    Plate::pVoice*           pvoice;    // parent voice (if not on-page)
    Plate::pNote*            pnote;     // parent note  (if not on-page)
    
    MovableList*             list;      // source Movable list (on a Note, page or Document)
    AttachableList*          plist;     // source Attachable list (on a Plate or pPage)
    
    MovableList::iterator    object;    // Movable object (as iterator on "note")
    AttachableList::iterator pobject;   // on-plate data (as iterator on "pnote")
    
 private:
    // object movement data
    //Plate::Pos move_offset;             // current offset (in on-plate coordinates)
    //Position<> new_position;            // new object position (as in the Movable object)
 
 protected:
    bool setup();   // sets "pobject" and "pobject" to the first attached object
    
 public:
    // constructors
    ObjectCursor(Document& document, Pageset& pageset);
    virtual ~ObjectCursor();
    
    // set parent note
    bool set_parent(Pageset::pPage& page);  // set page as parent for objects
    bool set_parent(EditCursor& cursor);    // set note as parent for objects
    bool set_parent(Pageset::pPage& page, Score& score, VisibleObject& note, Plate::pNote& pnote, Plate::pVoice& pvoice, Plate::pLine& pline);
    
    // select target object
    bool select(const Movable& object);             // select object within parent
    bool select(Position<mpx_t>, Pageset::pPage&);  // select object by position (sets parent!)
    
    // iterator interface
    bool next();            // goto next object attached to the parent note
    void reset();           // reset cursor to invalid state
    bool ready() const;     // check, if cursor is valid
    bool end() const;       // check, if the iterator is at end
    bool has_score() const; // check, if the object is not on-page?
    
    // object interface
    const Document&     get_document() const;   // return the document
    const Pageset&      get_pageset()  const;   // return the pageset
    const Score&        get_score()    const;   // return the score-object
    const Staff&        get_staff()    const;   // return parent staff
    const Plate::pLine& get_line()     const;   // return parent line
    const Plate::pNote& get_parent()   const;   // return parent note
    
    size_t              get_pageno()   const;   // return target page number
    Movable&            get_object()   const;   // return referenced object
    Plate::pAttachable& get_pobject()  const;   // return on-plate object
    
    // rendering interface
    virtual void render(Renderer& renderer, const PressState& state) const;
    
    // reengraving interface
    virtual void   setup_reengrave(ReengraveInfo& info);    // setup reengraving triggers
    virtual Status reengrave(EngraverState& state);         // reengraving function
    virtual void   finish_reengrave();                      // reengrave finishing function (NOOP)
    /*
    // moving interface
    bool prepare_move(Plate::Pos offset, Grid grid);    // prepare movement (returns, if offset changed)
    bool execute_move(const ViewportParam& viewport);   // execute movement (returns, if there was any)
    void abort_move();                                  // remove prepared movement
    bool prepared_move() const;                         // return, if a move was prepared
    */
};

// inline methods
inline void                ObjectCursor::reset()              {list = NULL; plist = NULL;}
inline bool                ObjectCursor::ready()        const {return list && plist;}
inline bool                ObjectCursor::end()          const {return object == list->end() && pobject == plist->end();}
inline bool                ObjectCursor::has_score()    const {return pnote != NULL;}

inline const Document&     ObjectCursor::get_document() const {return *document;}
inline const Pageset&      ObjectCursor::get_pageset()  const {return *pageset;}
inline const Score&        ObjectCursor::get_score()    const {return *score;}
inline const Staff&        ObjectCursor::get_staff()    const {return pnote->note.staff();}
inline const Plate::pLine& ObjectCursor::get_line()     const {return *pline;}
inline const Plate::pNote& ObjectCursor::get_parent()   const {return *pnote;}

inline size_t              ObjectCursor::get_pageno()   const {return pageno;}

//inline void                ObjectCursor::abort_move()          {move_offset.x = move_offset.y = 0;}
//inline bool                ObjectCursor::prepared_move() const {return (move_offset.x || move_offset.y);}

} // end namespace

#endif


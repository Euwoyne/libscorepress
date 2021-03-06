
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

#ifndef SCOREPRESS_PLATE_HH
#define SCOREPRESS_PLATE_HH

#include <map>          // std::map
#include <list>         // std::list

#include "cursor.hh"    // const_Cursor, Position, Voice, SmartPtr, RefPtr
#include "stem_info.hh" // StemInfo
#include "context.hh"   // VoiceContext, StaffContext
#include "sprite_id.hh" // SpriteId
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API Plate;             // structure of absolute positions and sprites for immediate rendering
class SCOREPRESS_API Plate_GphBox;      // graphical boundary box
class SCOREPRESS_API Plate_pGraphical;  // graphical object (base class for all on-plate objects)
class SCOREPRESS_API Plate_pAttachable; // attachable object (object pointer, sprite and position)
class SCOREPRESS_API Plate_pDurable;    // durable object (attachable with additional end-position)
class SCOREPRESS_API Plate_pNote;       // note object (with attachables, stem and optional barline)
class SCOREPRESS_API Plate_pVoice;      // voice object (list of notes)
class SCOREPRESS_API Plate_pLine;       // on-plate line object (list of voices)

// typedefs
typedef Position<mpx_t> Plate_Pos;


//
//     class Plate
//    =============
//
// Absolute positions and sprite ids for the easy and fast redering of a
// score. This structure will be created from a score by the engraver.
// It can be rendered to a device by a press (see "press.hh").
// Positions are interpreted relative to the score-dimension's origin (see
// "Pageset")
//

// graphical boundary box
class SCOREPRESS_API Plate_GphBox
{
 public:
    Plate_Pos pos;      // position of the top-left corner
    mpx_t     width;    // width of the box
    mpx_t     height;   // height of the box
    
    Plate_GphBox();     // constructors
    Plate_GphBox(Plate_Pos pos, mpx_t width, mpx_t height);
    
    inline mpx_t right()  const;                // calculate the right margin of the box
    inline mpx_t bottom() const;                // calculate the bottom of the box
    
    bool contains(const Plate_Pos& p) const;    // check, if a given point is inside the box
    bool overlaps(const Plate_GphBox& box);     // check, if a given box overlaps this box
    void extend(const Plate_Pos& p);            // extend box, such that the given point is covered
    void extend(const Plate_GphBox& box);       // extend box, such that the given box is covered
};

inline mpx_t Plate_GphBox::right()  const {return pos.x + width;}
inline mpx_t Plate_GphBox::bottom() const {return pos.y + height;}

inline bool Plate_GphBox::contains(const Plate_Pos& p) const {
    return (p.x >= pos.x && p.y >= pos.y && p.x < pos.x + width && p.y < pos.y + height);}


// graphical object (base class for all on-plate objects)
class SCOREPRESS_API Plate_pGraphical
{
 public:
    Plate_GphBox gphBox;
    
    Plate_pGraphical() {}                                       // default constructor
    Plate_pGraphical(Plate_Pos pos, mpx_t width, mpx_t height); // constructor
    bool contains(Plate_Pos p) const;                           // check if a point is within the object
};

inline bool Plate_pGraphical::contains(Plate_Pos p) const {return gphBox.contains(p);}


// attachable object (object pointer, sprite and position)
class SCOREPRESS_API Plate_pAttachable : public Plate_pGraphical
{
 public:
    const AttachedObject* const object; // original object
    SpriteId sprite;                    // sprite id
    Plate_Pos absolutePos;              // position of the sprite
    
    struct {unsigned x : 1; unsigned y : 1;} flipped;   // flipped flags
    
    Plate_pAttachable(const AttachedObject& obj, const Plate_Pos& pos); // constructor
    bool is_durable();
};

inline bool Plate_pAttachable::is_durable() {return object->is(Class::DURABLE);}


// durable object (attachable with additional end-position)
class SCOREPRESS_API Plate_pDurable : public Plate_pAttachable
{
 public:
    Plate_Pos endPos;
    
    Plate_pDurable(const AttachedObject& obj, const Plate_Pos& pos);    // constructor
};


// note object (with attachables, stem and optional barline)
class SCOREPRESS_API Plate_pNote : public Plate_pGraphical
{
 public:
    // ledger line position information structure
    struct LedgerLines
    {
        Plate_Pos    basepos;   // position of the lowest ledger line
        mpx_t        length;    // length of the ledger lines
        unsigned int count;     // number of ledger lines
        bool         below;     // ledger lines below or above the staff?
        
        LedgerLines() : length(0), count(0), below(false) {}
    };
    
    // tie position information
    class Tie
    {
     public:
        Plate_Pos pos1;         // begin position
        Plate_Pos pos2;         // end position
        Plate_Pos control1;     // first control point
        Plate_Pos control2;     // second control point
    };
    
    // virtual object structure
    struct Virtual
    {
     public:
        SmartPtr<const StaffObject, CloneTrait> object;     // virtual object (i.e. non existant in score structure)
        bool inserted;                                      // inserted or replacing the original object?
        
        Virtual(const StaffObject& object, bool inserted);  // ("object" will be cloned)
    };
    
    // stem information structure
    struct Stem
    {
        mpx_t x;            // horizontal position
        mpx_t top;          // top vertical position
        mpx_t base;         // bottom vertial position (i.e. where it touches the head)
        
        size_t beam_off;    // stem length correction (internal, temporary)
        
        inline bool is_up() const {return top < base;};
    };
    
    // beam information structure
    struct Beam
    {
        const Plate_pNote* end;             // reference to the end-note
        unsigned           end_idx     : 6; // beam index on end-note
        unsigned           short_beam  : 1; // short beam? (if it is, "end" only defines the slope)
        unsigned           short_left  : 1; // short beam direction 
        
        Beam() : end(NULL), end_idx(0), short_beam(0), short_left(0) {}
    };
    
    // list and pointer typedefs
    typedef std::list< Plate_Pos >       PositionList;
    typedef RefPtr<Plate_pAttachable>    AttachablePtr;
    typedef std::list<AttachablePtr>     AttachableList;
    typedef std::list<Tie>               TieList;
    typedef std::list<LedgerLines>       LedgerLineList;
    typedef RefPtr<Virtual>              VirtualPtr;
    typedef RefPtr<Beam>                 BeamPtr;
    typedef SmartPtr<StemInfo>           StemInfoPtr;
    typedef std::list<Plate_pNote>       NoteList;
    typedef NoteList::iterator           Iterator;
    
 public:
    const_Cursor   note;                    // note object
    
    SpriteId       sprite;                  // head sprite id
    PositionList   absolutePos;             // positions for each head
    PositionList   dotPos;                  // positions for each dot
    LedgerLineList ledgers;                 // ledger lines
    TieList        ties;                    // attached ties
    AttachableList attached;                // list of attached objects
    VirtualPtr     virtual_obj;             // virtual object
    
    Stem           stem;                    // stem information
    StemInfoPtr    stem_info;               // temporary additional stem information (only there during engraving)
    BeamPtr        beam[VALUE_BASE - 2];    // beam information (VALUE_BASE - 3 being eighth)
    Iterator       beam_begin;              // beam begin note
    bool           noflag;                  // is a flag to be rendered?
    
 public:
    Plate_pNote(const Plate_Pos& pos, const const_Cursor& note);    // constructor
    
    void add_offset(mpx_t offset);          // add offset to all positions (except to the tie-end)
    void add_tieend_offset(mpx_t offset);   // add offset to tie-end positions
    
    const StaffObject& get_note() const;    // return related staff-object
    bool is_virtual() const;                // is this a virtual note?
    bool is_inserted() const;               // is this an inserted virtual note?
    bool at_end() const;                    // is this an eov indicator? (if it is, get_note() will SEGFAULT!)
    
    // dump pnote info
    void dump() const;
};

inline const StaffObject& Plate_pNote::get_note()    const {return (virtual_obj ? *virtual_obj->object : *note);}
inline       bool         Plate_pNote::is_virtual()  const {return (virtual_obj != NULL);}
inline       bool         Plate_pNote::is_inserted() const {return (virtual_obj && virtual_obj->inserted);}
inline       bool         Plate_pNote::at_end()      const {return (!virtual_obj && note.at_end());}


// voice object (list of notes)
class SCOREPRESS_API Plate_pVoice
{
 public:
    typedef std::list<Plate_pNote>   NoteList;
    typedef NoteList::iterator       Iterator;
    typedef NoteList::const_iterator const_Iterator;
    
    struct Brace : public Plate_pGraphical
    {
        SpriteId sprite;
    };
    
    struct Bracket : public Plate_pGraphical
    {
        SpriteId  sprite;
        Plate_Pos line_base;
        Plate_Pos line_end;
    };
    
 public:
    Plate_Pos     basePos;      // top-right corner of the staff
    umpx_t        head_height;  // the staff's head-height (in millipixel)
    NoteList      notes;        // notes of the voice
    const_Cursor  begin;        // cursor at the beginning of this voice (in the score object)
    const_Cursor  parent;       // cursor to the voice's parent note (in the score object)
    VoiceContext  context;      // this voice's context at the END of the line (or voice)
    value_t       time;         // time-stamp of the voice's first note
    value_t       end_time;     // time-stamp at the voice's last note
    Brace         brace;        // brace starting here
    Bracket       bracket;      // bracket starting here
    
    Plate_pVoice(const const_Cursor& cursor);       // constructor
    
    Iterator append(const Plate_Pos& pos, const const_Cursor& note);    // append new note
};


// on-plate line object (list of voices)
class SCOREPRESS_API Plate_pLine : public Plate_pGraphical
{
 public:
    typedef std::list<Plate_pVoice>              VoiceList;
    typedef VoiceList::iterator                  Iterator;
    typedef VoiceList::const_iterator            const_Iterator;
    typedef std::map<const Staff*, StaffContext> StaffContextMap;
    
 public:
    Plate_GphBox    noteBox;        // graphical boundary box (only note objects)
    Plate_Pos       basePos;        // top-right corner position
    mpx_t           line_end;       // line width
    value_t         end_time;       // time-stamp at the line's end
    
    VoiceList       voices;         // voices within this line
    ScoreContext    context;        // score context (at the end of the line)
    StaffContextMap staffctx;       // staff contexts
    
    Iterator       get_voice(const Voice& voice);       // find a voice in this line
    const_Iterator get_voice(const Voice& voice) const; // (constant version)
    Iterator       get_staff(const Staff& staff);       // find any voice of a given staff
    const_Iterator get_staff(const Staff& staff) const; // (constant version)
    
    void erase();            		// erase the line
    void calculate_gphBox();        // calculate graphical boundary box
};

inline void Plate_pLine::erase() {voices.clear();}


// plate class
class SCOREPRESS_API Plate
{
 public:
    //  CLASSES
    // ---------
    typedef Plate_Pos         Pos;          // position class (for mpx_t)
    typedef Plate_GphBox      GphBox;       // graphical object (base class for all on-plate objects)
    typedef Plate_pGraphical  pGraphical;   // graphical object (base class for all on-plate objects)
    typedef Plate_pAttachable pAttachable;  // attachable object (object pointer, sprite and position)
    typedef Plate_pDurable    pDurable;     // durable object (attachable with additional end-position)
    typedef Plate_pNote       pNote;        // note object (with attachables, stem and optional barline)
    typedef Plate_pVoice      pVoice;       // voice object (list of notes)
    typedef Plate_pLine       pLine;        // on-plate line object (list of voices)
    
    // list typedefs
    typedef std::list<pNote>    NoteList;
    typedef std::list<pVoice>   VoiceList;
    typedef std::list<pLine>    LineList;
    typedef LineList::iterator  Iterator;
    typedef LineList::iterator  LineIt;
    typedef VoiceList::iterator VoiceIt;
    typedef NoteList::iterator  NoteIt;
    
    // lines on the plate
    LineList lines;
    
    // dump the plate content to stdout
    void dump() const;
};

} // end namespace

#endif


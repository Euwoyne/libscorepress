
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

#ifndef SCOREPRESS_PICK_HH
#define SCOREPRESS_PICK_HH

#include <list>             // std::list

#include "score.hh"         // Score, Staff, LineLayout, [score classes]
#include "cursor.hh"        // const_Cursor
#include "sprites.hh"       // Sprites
#include "parameters.hh"    // EngraverParam, ViewportParam
#include "error.hh"         // Error, std::string
#include "log.hh"           // Logging
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_LOCAL Pick;     // position calculating iterator for a score object


//
//     class Pick
//    ============
//
// This class is an iterator for "Score", which, for each iteration step,
// calculates the position of the current Staff-Object.
// This is done parallely for each voice in the score to ensure, that the
// Objects are ordered according to their position along the score.
//
class SCOREPRESS_LOCAL Pick : public Logging
{
 public:
    // a special cursor containing position and time information about the note pointed to
    class VoiceCursor : public const_Cursor
    {
     public:
        mpx_t pos;      // horizontal position
        mpx_t npos;     // position of the next note
        mpx_t ypos;     // vertical position
        value_t time;   // time-stamp of the note
        value_t ntime;  // time-stamp after the note
        
        StaffObjectPtr virtual_obj; // virtual object
        bool inserted;              // inserted or replacing the original object?
        value_t remaining_duration; // duration of the part not yet engraved
        
     public:
        VoiceCursor();          // default constructor
        bool at_end() const;    // overwrite "at_end" to return "false", if virtual
        
        const StaffObject& original() const;        // return the original staff-object
        const StaffObject& operator * () const;     // return the staff-object the cursor points to
        const StaffObject* operator -> () const;    // return a pointer to the staff-object
    };
    
    // line layout (collection of newlines for each voice)
    class LineLayout
    {
     public:
        class VoiceNotFoundException : public ScorePress::Error // thrown, if layout of non-existant voice is requested
        {public: VoiceNotFoundException();};
        
     private:
        std::map<const Voice*, const Newline*> data;    // maps voices to their newline objects
        const Voice* first_voice;                       // voice with line-properties
        
     public:
        void set(const Voice& voice, const Newline& layout);                        // associate a voice with its newline object
        bool exist(const Voice& voice) const;                                       // check, if the voice has a newline object
        const Newline& get() const throw(VoiceNotFoundException);                   // get first newline object (for line-properties)
        const Newline& get(const Voice& voice) const throw(VoiceNotFoundException); // get the newline object
        void remove(const Voice& voice);                                            // remove a voice's layout
        void set_first_voice(const Voice& voice) throw(VoiceNotFoundException);     // set the voice for line-properties
        void swap(LineLayout& a);                                                   // swap contents
        void clear();                                                               // clear data
    };
    
 public:
    // return the staff-object's sprite-id
    static SpriteId sprite_id(const Sprites& spr, const StaffObject* obj);
    
    // return the accidental's sprite-id
    static SpriteId sprite_id(const Sprites& spr, const Accidental& obj);
    
    // return the staff-object's sprite-information
    inline static const SpriteInfo& sprite(const Sprites& spr, const StaffObject* obj)
        {return spr[sprite_id(spr, obj)];};
    
    // return the graphical width for the number
    static mpx_t width(const SpriteSet& spr, const unsigned int n, const mpx_t height);
    
    // return the width of the staff-object's graphic
    static mpx_t width(const Sprites& spr, const StaffObject* obj, const mpx_t height);
    
    // caluclate the width specified by the value of the note
    static mpx_t value_width(const value_t& value, const EngraverParam& param, const ViewportParam& vparam);
    
 private:
    const Score* const score;               // pointer to the score object to be engraved
    const EngraverParam* const param;       // engraving-parameters (see "parameters.hh")
    const ViewportParam* const viewport;    // viewport-paramters (see "parameters.hh")
    const Sprites* const sprites;           // pointer to the sprite-library (access to the graphical widths)
    
    typedef std::list<VoiceCursor> CList;   // list of voice cursors
    CList cursors;                          // containing cursors to the notes next to be read of all voices
                                            // ordered descending! by "time" (the last entry is the next note)
    CList next_cursors;                     // cursors for the next line (stored seperately, so that they do not
                                            // get mixed up with the currently processed newlines)
    
    const ScoreDimension* _dimension;       // dimension of the score object on the currently engraved page
    LineLayout _layout;                     // layout information for the current line
    LineLayout _next_layout;                // layout information for the next line (used during newline processing)
    bool       _newline;                    // newline indicator (set during the processing of a newline block)
    bool       _pagebreak;                  // pagebreak indicator (set during the processing of a pagebreak block)
    value_t    _newline_time;               // timestamp of the first newline object (during newline processing)
    mpx_t      _line_height;                // height of the line (during newline processing)
    
    void insert(const VoiceCursor& cursor); // insert the given cursor, maintaining the order according to "time"
    void insert(const VoiceCursor& cursor, CList&);
    void add_subvoices(VoiceCursor cursor); // add cursors for the sub-voices of a note to the stack
    void add_subvoices(VoiceCursor cursor, CList&);
    void _initialize();                     // intialize the cursors to the score's beginning
    
    void calculate_npos(VoiceCursor& nextNote);                      // calculate estimated position of the following note
    void insert_next(const VoiceCursor& engravedNote);               // insert next note of current voice
    void prepare_next(const VoiceCursor& engravedNote, mpx_t width); // prepare next note to be engraved

 public:
    Pick(const Score& score, const EngraverParam& param, const ViewportParam& viewport, const Sprites& sprites);
        
    // movement methods
    void next(mpx_t width = 0); // pop current note from stack and add next note in voice
    void reset();               // reset cursors to the beginning of the score
    
    const VoiceCursor& get_cursor(const Voice& voice) const;    // get cursor of a special voice
    
    // pick manipulation interface
    void add_distance(mpx_t dst, value_t time);       // apply additional distance to all notes at/after a given time
    void add_distance_after(mpx_t dst, value_t time); // apply additional distance to all notes after a given time
    
    void insert(const StaffObject& obj);              // insert a virtual object (after the current one)
    void insert_barline(const Barline::Style& style); // insert a virtual barline object after current object
    bool insert_before(const StaffObject& obj);       // insert a virtual object (changes current cursor)
    bool insert_before(const StaffObject& obj,        // insert a virtual object (into given voice)
                       const Voice& voice);
    
    void cut(value_t duration);                       // cut the note into two tied notes (given duration of the first)
    
    // distance calculation
    const Staff& get_staff(int idx_shift = 0) const;  // get the current staff (shifted)
    int staff_offset(int idx_shift = 0) const;        // calculate (current) staff's offset relative to the line's position
    int staff_offset(const Staff& staff) const;       //      (in micrometer)
    int line_height() const;                          // calculate the complete line height (in micrometer)
    
    // inline status reporter
    const VoiceCursor&    get_cursor()                const; // return current cursor (it is ensured, that this cursor is valid
                                                             //                        as long as the score object hasn't changed)
          bool            eos()                       const; // check if the pick is ready (or has reached the score's end)
          bool            eov()                       const; // check if the current cursor is last in voice
    const VoiceCursor*    peek(const Voice& v)        const; // peek at the next note in the voice (NULL if not there)
          bool            is_within_newline()         const; // is this a follow-up newline? (i.e. not the first)
    const ScoreDimension& get_dimension()             const; // return dimension of the currently engraved score object
          mpx_t           get_indent()                const; // return the indentation of the current line
          bool            get_justify()               const; // return the width justification for the current line
          bool            get_forced_justification()  const; // return the forced justification for the current line
          mpx_t           get_right_margin()          const; // return the distance from the right border of the score object
    const LineLayout&     get_layout()                const; // return the layout information for the current line
    const Newline&        get_layout(const Voice& v)  const; // return the layout of the given voice
    const Score&          get_score()                 const; // return the score object which is to be engraved
};

// inline method implementations
inline       bool         Pick::VoiceCursor::at_end() const
            {return (!!virtual_obj ? false : const_Cursor::at_end());}
inline const StaffObject& Pick::VoiceCursor::original() const
            {return const_Cursor::operator *();}
inline const StaffObject& Pick::VoiceCursor::operator * () const
            {return (!!virtual_obj ? *virtual_obj : const_Cursor::operator *());}
inline const StaffObject* Pick::VoiceCursor::operator -> () const
            {return (!!virtual_obj ? &*virtual_obj : const_Cursor::operator ->());}

inline void Pick::LineLayout::swap(LineLayout& a) {std::swap(data, a.data); std::swap(first_voice, a.first_voice);}
inline void Pick::LineLayout::clear()             {data.clear();}

inline void Pick::insert(const VoiceCursor& cursor) {insert(cursor, cursors);}
inline void Pick::add_subvoices(VoiceCursor cursor) {add_subvoices(cursor, cursors);}

inline const Pick::VoiceCursor& Pick::get_cursor()                const {return cursors.back();}
inline       bool               Pick::eos()                       const {return cursors.empty() && next_cursors.empty();}
inline       bool               Pick::is_within_newline()         const {return _newline;}
inline const ScoreDimension&    Pick::get_dimension()             const {return *_dimension;}
inline       mpx_t              Pick::get_indent()                const {return viewport->umtopx_h(_layout.get().indent);}
inline       bool               Pick::get_justify()               const {return _layout.get().justify;}
inline       bool               Pick::get_forced_justification()  const {return _layout.get().forced_justification;}
inline       mpx_t              Pick::get_right_margin()          const {return viewport->umtopx_h(_layout.get().right_margin);}
inline const Pick::LineLayout&  Pick::get_layout()                const {return _layout;}
inline const Newline&           Pick::get_layout(const Voice& v)  const {return _layout.get(v);}
inline const Score&             Pick::get_score()                 const {return *score;}

} // end namespace

#endif


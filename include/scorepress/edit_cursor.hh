
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

#ifndef SCOREPRESS_EDITCURSOR_HH
#define SCOREPRESS_EDITCURSOR_HH

#include "user_cursor.hh"   // UserCursor, Document, Pageset, [score classes]
#include "engraver.hh"      // Engraver
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class EditCursor;       // cursor, with graphical representation, and simple movement interface

// PROTOTYPES
class ObjectCursor;     // object-cursor prototype (see "object_cursor.hh")

//
//     class EditCursor
//    ==================
//
// This cursor inherits the behaviour of the "UserCursor" and provides an
// interface for the modification of the score data.
//
class SCOREPRESS_API EditCursor : public UserCursor
{
 public:
    // exception classes
    class SCOREPRESS_API RemoveMainException : public UserCursor::Error // thrown, if the main voice is deleted
        {public: RemoveMainException();};
    
    // note-name enumeration type
    enum SCOREPRESS_API NoteName {C, D, E, F, G, A, B};
    
    // note informtaion input structure
    struct SCOREPRESS_API InputNote
    {
        NoteName         name;         // specified name
        signed char      octave;       // octave modifier
        unsigned char    exp;          // desired exponent
        unsigned char    dots;         // dot count
        Accidental::Type accidental;   // accidental specification
        
        // constructor (with default values)
        InputNote(NoteName         name       = C,
                  signed char      octave     = 0,
                  unsigned char    exp        = 5,
                  unsigned char    dots       = 0,
                  Accidental::Type accidental = Accidental::natural);
    };
    
 private:
    // interface parameters
    const InterfaceParam& param;
    const ViewportParam&  viewport;
    
    // calculate tone from note-name (regarding input method, ignoring accidentals)
    SCOREPRESS_LOCAL tone_t get_tone(const InputNote&) const;
    
    // create the head instance (according to "relative_accidentals" parameter)
    SCOREPRESS_LOCAL HeadPtr create_head(const InputNote&) const;
    
    // execute the given function for each chord in the beam-group defined by the cursor
    typedef void (*callback_t)(Chord&, const VoiceCursor&, const int, int*);
    SCOREPRESS_LOCAL static bool for_each_chord_in_beam_do(VoiceCursor&, callback_t, const int arg, int* out = NULL);
    
    // callbacks for different tasks
    SCOREPRESS_LOCAL static void _add_stem_length(Chord&, const VoiceCursor&, const int pohh, int*) noexcept;
    SCOREPRESS_LOCAL static void _set_stem_length(Chord&, const VoiceCursor&, const int pohh, int*) noexcept;
    SCOREPRESS_LOCAL static void _set_stem_dir(   Chord&, const VoiceCursor&, const int dir,  int*);
    SCOREPRESS_LOCAL static void _set_stem_type(  Chord&, const VoiceCursor&, const int type, int*);
    
    // set the given cursor "beam_begin" to the first note within the current beam-group
    SCOREPRESS_LOCAL bool get_beam_begin(VoiceCursor& beam_begin) const;
    
 public:
    // constructors
    EditCursor(      Document&       document,
                     Pageset&        pageset,
               const InterfaceParam& param,
               const ViewportParam&  viewport);
    
    virtual ~EditCursor();
    
    // access methods (constant; from UserCursor)
    using UserCursor::get_document;
    using UserCursor::get_score;
    using UserCursor::get_staff;
    using UserCursor::get_voice;
    using UserCursor::get_cursor;
    using UserCursor::get_attached;
    
    using UserCursor::get_pageno;
    using UserCursor::get_start_page;
    using UserCursor::get_score_page;
    using UserCursor::get_pageset;
    using UserCursor::get_page;
    using UserCursor::get_plate;
    using UserCursor::get_line;
    using UserCursor::get_pvoice;
    using UserCursor::get_platenote;
    
    // layout access (constant; from UserCursor)
    using UserCursor::get_style;
    using UserCursor::get_layout;
    using UserCursor::get_dimension;
    using UserCursor::get_page_attached;
    
    // access methods (non-constant)
    Document&       get_document() noexcept;    // return the document
    Score&          get_score()    noexcept;    // return the score-object
    Staff&          get_staff();                // return the staff
    Voice&          get_voice();                // return the voice
    Cursor&         get_cursor();               // return the score-cursor
    MovableList&    get_attached();             // return objects attached to the note
    
    size_t          get_pageno()     noexcept;  // return the page-number
    size_t          get_start_page() noexcept;  // return the start-page of the score
    size_t          get_score_page() noexcept;  // return the page-number within the score
    Pageset&        get_pageset()    noexcept;  // return the pageset
    Pageset::pPage& get_page()       noexcept;  // return the page
    Plate&          get_plate()      noexcept;  // return the plate-object
    Plate::pLine&   get_line()       noexcept;  // return the on-plate line
    Plate::pVoice&  get_pvoice();               // return the on-plate voice
    Plate::pNote&   get_platenote();            // return the on-plate note
    
    // layout access (non-constant)
    StyleParam&     get_style();            // return the style parameters
    LayoutParam&    get_layout();           // return the line layout
    ScoreDimension& get_dimension();        // return the score dimension
    MovableList&    get_page_attached();    // return objects attached to the page
    
    // insert an object (inserting transfers the objects ownership to the voice-object within the score)
    void insert     (StaffObject* const);
    void insert     (const InputNote&);
    void insert_head(const InputNote&);
    void insert_rest(const unsigned char exp, const unsigned char dots = 0);
    void insert_newline();
    void insert_pagebreak();
    
    // remove an object
    void remove();              // remove a note
    void remove_voice();        // remove a voice
    void remove_newline();      // remove newline/pagebreak
    void remove_pagebreak();    // convert pagebreak to newline
    void remove_break();        // remove newlines, convert pagebreak
    
    // stem control
    void add_stem_length(spohh_t pohh);
    void set_stem_length(spohh_t pohh);
    void add_stem_slope (spohh_t pohh);
    void set_stem_slope (spohh_t pohh);
    void set_stem_dir   (bool    down);
    
    void set_stem_type (const Chord::StemType);
    void set_slope_type(const Chord::SlopeType);
    
    // accidental control
    void set_accidental_auto();
};

// inline method implementations
inline EditCursor::InputNote::InputNote(NoteName n, signed char o, unsigned char e, unsigned char d, Accidental::Type a)
            : name(n), octave(o), exp(e), dots(d), accidental(a) {}

inline Document&       EditCursor::get_document()   noexcept {return *document;}
inline Score&          EditCursor::get_score()      noexcept {return *score;}
inline size_t          EditCursor::get_pageno()     noexcept {return page->pageno;}
inline size_t          EditCursor::get_start_page() noexcept {return plateinfo->start_page;}
inline size_t          EditCursor::get_score_page() noexcept {return plateinfo->pageno;}
inline Pageset&        EditCursor::get_pageset()    noexcept {return *pageset;}
inline Pageset::pPage& EditCursor::get_page()       noexcept {return *page;}
inline Plate&          EditCursor::get_plate()      noexcept {return *plateinfo->plate;}
inline Plate::pLine&   EditCursor::get_line()       noexcept {return *line;}

} // end namespace

#endif


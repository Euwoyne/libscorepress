
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2012 Dominik Lehmann
  
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

#ifndef SCOREPRESS_EDITCURSOR_HH
#define SCOREPRESS_EDITCURSOR_HH

#include "user_cursor.hh"   // UserCursor, Document, PageSet, [score classes]
#include "engraver.hh"      // Engraver

namespace ScorePress
{
//  CLASSES
// ---------
class EditCursor;       // cursor, with graphical representation, and simple movement interface


//
//     class EditCursor
//    ==================
//
// This cursor inherits the behaviour of the "UserCursor" and provides an
// interface for the modification of the score data.
//
class EditCursor : public UserCursor
{
 public:
    // note-name enumeration type
    enum NoteName {C, D, E, F, G, A, B};
    
    // note informtaion input structure
    struct InputNote
    {
        NoteName         name;         // specified name
        signed char      octave;       // octave modifier
        unsigned char    exp;          // desired exponent
        unsigned char    dots;         // dot count
        Accidental::Type accidental;   // accidental specification
        
        // constructor (with default values)
        InputNote(NoteName n = C, signed char o = 0, unsigned char e = 5, unsigned char d = 0, Accidental::Type a = Accidental::natural)
            : name(n), octave(o), exp(e), dots(d), accidental(a) {};
    };
    
 private:
    enum MoveMode {NONE, INSERT, REMOVE, NEWLINE, RMNEWLINE};   // cursor move mode
    
    // interface parameters
    const InterfaceParam* param;
          Engraver*       engraver;
    
    // calculate tone from note-name (regarding input method, ignoring accidentals)
    tone_t get_tone(const InputNote& note) const throw(NotValidException);
    
    // create the head instance (according to "relative_accidentals" parameter)
    HeadPtr create_head(const InputNote& note) const throw(NotValidException);
    
    // calculates the automatic stem-length (uses staff reference)
    void set_auto_stem_length(Chord& chord) const throw(NotValidException);
    
 public:
    // constructor
    EditCursor(Document& doc, PageSet& pset, const InterfaceParam& param);
    EditCursor(Document& doc, PageSet& pset, const InterfaceParam& param, Engraver& engraver);
    
    // engraver interface
    bool has_engraver() const;
    void set_engraver(Engraver& engraver);
    void remove_engraver();
    Engraver& get_engraver();
    const Engraver& get_engraver() const;
    
    // reengrave the score and update this cursor (all iterators and pVoice::begin must be valid when calling this!)
    void reengrave(const MoveMode& mode = NONE) throw(NoScoreException, Error);
    
    // insert an object (inserting transfers the objects ownership to the voice-object within the score)
    void insert(StaffObject* const object) throw(NotValidException, Cursor::IllegalObjectTypeException);
    
    // insert a note
    void insert(const InputNote& note) throw(NotValidException, NoScoreException, Error);

    // insert a head
    void insert_head(const InputNote& note) throw(NotValidException, Cursor::IllegalObjectTypeException, NoScoreException, Error);
    
    // insert a rest
    void insert_rest(const unsigned char exp, const unsigned char dots = 0) throw(NotValidException, NoScoreException, Error);
    
    // insert a newline
    void insert_newline() throw(NotValidException, NoScoreException, Error);
    void insert_newline(const Newline& newline) throw(NotValidException, NoScoreException, Error);
    
    // remove an object
    void remove()       throw(NotValidException);                                       // remove a note
    //void remove_voice() throw(NotValidException, Cursor::IllegalObjectTypeException);   // remove a voice
    
    // get the line layout object (non-constant)
    //Newline& get_layout() throw(NotValidException);
    
    // set auto stem/accidental to current object
    void set_stem_length_auto() throw(Cursor::IllegalObjectTypeException);
    void set_stem_dir_auto()    throw(Cursor::IllegalObjectTypeException);
    void set_accidental_auto()  throw(Cursor::IllegalObjectTypeException);
};

inline bool            EditCursor::has_engraver() const              {return engraver;}
inline void            EditCursor::set_engraver(Engraver& _engraver) {engraver = &_engraver;}
inline void            EditCursor::remove_engraver()                 {engraver = NULL;}
inline Engraver&       EditCursor::get_engraver()                    {return *engraver;}
inline const Engraver& EditCursor::get_engraver() const              {return *engraver;}

} // end namespace

#endif


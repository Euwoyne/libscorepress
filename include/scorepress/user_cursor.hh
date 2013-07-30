
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

#ifndef SCOREPRESS_USERCURSOR_HH
#define SCOREPRESS_USERCURSOR_HH

#include <list>             // std::list

#include "cursor.hh"        // Cursor, Document
#include "pageset.hh"       // PageSet, StaffContext, Plate, value_t
#include "parameters.hh"    // ViewportParam
#include "error.hh"         // Error, std::string

namespace ScorePress
{
//  CLASSES
// ---------
class UserCursor;   // cursor, with graphical representation, and simple movement interface


//
//     class UserCursor
//    ==================
//
// This cursor class iterates simultaneously the score and the corresponding
// plate. It has a graphical representation, which can be rendered by a
// "Press" instance. It can be manipulated by directions and graphical
// coorinates. (Modification methods implemented in child-class "EditCursor".)
//
class UserCursor
{
 public:
    // exception classes
    class Error : public ScorePress::Error {public: Error(const std::string& msg) : ScorePress::Error(msg) {};};
    class NotValidException : public Error              // thrown, if an invalid cursor is dereferenced
    {public: NotValidException() : Error("Unable to dereference invalid user-cursor.") {};
             NotValidException(const std::string& msg) : Error(msg) {};};
    class NoScoreException : public NotValidException   // thrown, if the target-score is not set
    {public: NoScoreException() : NotValidException("No score is set for this user-cursor.") {};};
    class InvalidMovement : public Error                // thrown, if an invalid cursor is dereferenced
    {public: InvalidMovement() : Error("Unable to move the user-cursor in the desired direction.") {};
             InvalidMovement(const std::string& dir) : Error("Unable to move the user-cursor in the desired direction (" + dir + ").") {};};
    
 protected:
    // plate-voice iterator with score-cursor and time-information
    class VoiceCursor
    {
     public:
        Cursor note;                                // score-cursor
        Cursor layout;                              // line layout
        
        std::list<Plate::pNote>::iterator pbegin;   // on-plate voice beginning
        std::list<Plate::pNote>::iterator pend;     // on-plate voice end
        std::list<Plate::pNote>::iterator pnote;    // plate-cursor
        Plate::pVoice*                    pvoice;   // on-plate voice
        
        value_t time;       // current time-stamp
        value_t ntime;      // time after the currently referenced object
        bool    active;     // is this voice part of the current cursor?
        bool    behind;     // is this cursor behind the note?
        
     public:
        bool has_prev() const;                      // check, if there is a previous note (in line and voice)
        bool has_next() const;                      // check, if there is a next note (in line and voice)
        void prev() throw(InvalidMovement, Error);  // to the previous note (fails, if "!has_prev()")
        void next() throw(InvalidMovement, Error);  // to the next note     (fails, if "at_end()")
        
        bool empty() const;                         // check, if the voice is empty
        bool at_end() const;                        // check, if the cursor is at the end of the voice
        
        //Newline& get_layout() const;                // return a reference to the layout object
        
        // engraving time check (corresponding to "cPick::insert")
        bool is_simultaneous(const VoiceCursor& cur) const; // are the objects simultaneous (and equal type)
        bool is_during(const VoiceCursor& cur) const;       // is the given object during this one?
        bool is_after(const VoiceCursor& cur) const;        // is this object rendered after the given one?
        bool is_before(const VoiceCursor& cur) const;       // is this object rendered before the given one?
    };
    
 protected:
    // base data
    Document* document;                         // document
    PageSet* pageset;                           // page-set
    Document::Score* score;                     // score
    std::list<PageSet::pPage>::iterator page;   // the currently referenced page
    PageSet::PlateInfo* plateinfo;              // plate and score
    std::list<Plate::pLine>::iterator line;     // on-plate line
    
    // voice cursors
    std::list<VoiceCursor> vcursors;            // a cursor for each voice
    std::list<VoiceCursor>::iterator cursor;    // currently referenced voice
    
 protected:
    // find a voice's cursor
    std::list<VoiceCursor>::iterator       find(const Voice& voice);
    std::list<VoiceCursor>::const_iterator find(const Voice& voice) const;
    
    // set all voice-cursors to the beginning of the current line
    void prepare_plate(VoiceCursor& newvoice, Plate::pVoice& pvoice);   // set "VoiceCursor" plate data ("note" and "layout" must be correct)
    bool prepare_voice(VoiceCursor& newvoice, Plate::pVoice& pvoice);   // set "VoiceCursor" data (helper)
    void prepare_voices();
    
    // move the voice-cursors to the corresponding position within the currently referenced voice
    void update_voices();
    
 private:
    // calculate the horizontal position for the given cursor
    mpx_t fast_x() const throw(NotValidException);    // (position right of the referenced object; fast)
    mpx_t fast_x(const VoiceCursor& cur) const throw(NotValidException);
    mpx_t graphical_x(const VoiceCursor& cur) const throw(NotValidException); // (exact)
    
    // set the cursor near the given x coordinate
    void set_x_rough(const mpx_t x);    // rough search    (in line)
    void set_x_voice(const mpx_t x);    // fine adjustment (in voice)
    
 public:
    // initialization methods
    UserCursor(Document& document, PageSet& pageset);       // constructor
    void set_score(Document::Score& score) throw(Error);    // initialize the cursor at the beginning of a given score
    void set_pos(Position<mpx_t>, const ViewportParam&);    // set cursor to graphical position (on current page)
    
    // access methods
    const Score&         get_score()     const;                             // return the score-object
    const Plate&         get_plate()     const;                             // return the plate-object
    const Plate::pLine&  get_line()      const;                             // return the on-plate line
    const Voice&         get_voice()     const throw(NotValidException);    // return the voice
    const Plate::pVoice& get_pvoice()    const throw(NotValidException);    // return the on-plate voice
    const Cursor&        get_cursor()    const throw(NotValidException);    // return the score-cursor
    const Plate::pNote&  get_platenote() const throw(NotValidException);    // return the on-plate note
    //const Newline&       get_layout()    const throw(NotValidException);    // return the line layout
          value_t        get_time()      const throw(NotValidException);    // return the current time-stamp
    
    bool   is_behind()   const throw(NotValidException);        // check, if the cursor is behind the referenced note
    bool   at_end()      const throw(NotValidException);        // check, if the cursor is at the end of the voice
    size_t voice_index() const throw(NotValidException);        // return the index of the current voice
    size_t voice_count() const;                                 // return the number of voices
    size_t staff_voice_index() const throw(NotValidException);  // return index of current voice (in staff)
    size_t staff_voice_count() const throw(NotValidException);  // return the number of voices (in staff)
    
    // direction checkers (indicate, if the respective movement would throw an "InvalidMovement" exception)
    bool has_prev()       const throw(NotValidException);       // check, if the cursor can be decremented
    bool has_next()       const throw(NotValidException);       // check, if the cursor can be incremented
    bool has_prev_voice() const throw(NotValidException);       // check, if the voice-cursor can be decremented
    bool has_next_voice() const throw(NotValidException);       // check, if the voice-cursor can be incremented
    bool has_prev_line()  const throw(NotValidException);       // check, if there is a previous line
    bool has_next_line()  const throw(NotValidException);       // check, if there is a next line
    
    // movement methods
    void prev()       throw(NotValidException, InvalidMovement);    // to the previous note
    void next()       throw(NotValidException, InvalidMovement);    // to the next note
    void prev_voice() throw(NotValidException, InvalidMovement);    // to the previous voice
    void next_voice() throw(NotValidException, InvalidMovement);    // to the next voice
    void prev_line()  throw(NotValidException, InvalidMovement);    // to the previous line
    void next_line()  throw(NotValidException, InvalidMovement);    // to the next line
    void home()       throw(NotValidException);                     // to the beginning of the line
    void home_voice() throw(NotValidException);                     // to the beginning of the voice (in line)
    void end()        throw(NotValidException);                     // to the end of the line
    void end_voice()  throw(NotValidException);                     // to the end of the voice (in line)
    
    // calculate current contexts
    StaffContext get_staff_context() const throw(NotValidException);    // context without local accidentals
    
    // is the cursor valid?
    bool empty() const;
    bool ready() const;
    bool has_score() const;
    
    // graphical representation
    mpx_t graphical_x() const                                   throw(NotValidException);   // horizontal position
    mpx_t graphical_y(const ViewportParam& viewport) const      throw(NotValidException);   // vertical position
    mpx_t graphical_height(const ViewportParam& viewport) const throw(NotValidException);   // height of the cursor
    
    // dump cursor state to stdout
    void dump() const;
};

// inline method implementations
inline bool UserCursor::VoiceCursor::has_prev() const {return (behind && !empty()) || (&*note != &*pvoice->begin);}
inline bool UserCursor::VoiceCursor::has_next() const {return (note.has_next() && pnote != pend);}
inline bool UserCursor::VoiceCursor::empty()    const {return pvoice->begin.at_end() || pvoice->begin->is(Class::NEWLINE);}
inline bool UserCursor::VoiceCursor::at_end()   const {return (behind && (pnote == pend));}

inline const Score&         UserCursor::get_score() const {return score->score;}
inline const Plate&         UserCursor::get_plate() const {return plateinfo->plate;}
inline const Plate::pLine&  UserCursor::get_line()  const {return *line;}

inline bool   UserCursor::empty()       const {return (score == NULL || cursor == vcursors.end() || cursor->empty());}
inline bool   UserCursor::ready()       const {return (score != NULL && cursor != vcursors.end());}
inline bool   UserCursor::has_score()   const {return (score != NULL);}

inline size_t UserCursor::voice_count() const {return vcursors.size();}
inline mpx_t  UserCursor::fast_x()      const throw(NotValidException) {return fast_x(*cursor);}
inline mpx_t  UserCursor::graphical_x() const throw(NotValidException) {return graphical_x(*cursor);}

} // end namespace

#endif


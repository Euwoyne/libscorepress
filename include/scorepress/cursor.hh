
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

#ifndef SCOREPRESS_CURSOR_HH
#define SCOREPRESS_CURSOR_HH

#include <list>        // std::list

#include "classes.hh"  // Staff, Voice, SubVoice, StaffObject, VoiceObject
#include "error.hh"    // Error
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API Cursor;          // cursor with voice and staff references
class SCOREPRESS_API const_Cursor;    // cursor with voice and staff references (within a constant score)


//
//     class Cursor
//    ==============
//
// This cursor points to a note in a score by containing pointers to the
// corresponding voice and staff and an iterator pointing to the note.
// It provides a unique interface for both main- and sub-voices.
// This is the non-constant version of the class.
//
class SCOREPRESS_API Cursor
{
 public:
    // exception classes
    class SCOREPRESS_API Error : public ScorePress::Error {public: Error(const std::string& msg);};
    class SCOREPRESS_API UninitializedCursorException : public Error
        {public: UninitializedCursorException();};  // thrown, if we use an uninitialized cursor
    class SCOREPRESS_API IllegalObjectTypeException : public Error
        {public: IllegalObjectTypeException();};    // thrown, if we insert a staff-object into a sub-voice
    
 private:
    Staff* _staff;         // pointer to the staff hosting the voice
    Voice* _voice;         // pointer to the voice hosting the cursor (of type "Staff" or "SubVoice")
    
    StaffObjectList::iterator _main;   // iterator to the note (either in a main-voice or
    VoiceObjectList::iterator _sub;    //                       in a sub-voice)
    
    friend class const_Cursor;         // (for equality operators)
    
 public:
    // constructors
    Cursor();
    Cursor(Staff& staff);
    Cursor(Staff& staff, SubVoice& voice);
    
    // iterator interface
    inline StaffObject& operator * () const         // return the Staff-Object the cursor points to
        {return (_staff == _voice) ? **_main : **_sub;}
    inline StaffObject* operator-> () const         // return a pointer to the Staff-Object the cursor points to
        {return (_staff == _voice) ?  &**_main :  &**_sub;}
    inline StaffObjectPtr& get_staffobject() const  // return smart-pointer to object
        {if (_staff != _voice) throw IllegalObjectTypeException(); return *_main;}
    inline VoiceObjectPtr& get_voiceobject() const  // return smart-pointer to object
        {if (_staff == _voice) throw IllegalObjectTypeException(); return *_sub;}
    
    Cursor& operator ++ ();             // increment operator; move cursor to the next note (prefix)
    Cursor& operator -- ();             // decrement operator; move cursor to the previous note (prefix)
    Cursor  operator ++ (int);          // increment operator; move cursor to the next note (postfix)
    Cursor  operator -- (int);          // decrement operator; move cursor to the previous note (postfix)
    
    void reset() noexcept;          // reset the cursor to undefined state
    void to_end();                  // set cursor to the voice's end
    size_t index() const;           // return the note index within the voice
    size_t voice_length() const;    // return the length of the voice
    
    // equality operators
    bool operator == (const Cursor&)       const noexcept;
    bool operator != (const Cursor&)       const noexcept;
    bool operator == (const const_Cursor&) const noexcept;
    bool operator != (const const_Cursor&) const noexcept;
    
    // state reporters
    bool has_next() const;          // check, whether cursor can be incremented
    bool has_prev() const;          // check, whether cursor can be decremented
    bool at_end() const;            // check, if the cursor is at the end
    
    inline bool ready()   const {return (_staff && _voice);}
    inline bool is_main() const {return (_staff == _voice);}
    inline bool is_sub()  const {return (_staff != _voice);}
    
    // staff and voice access
    inline Staff& staff() const {return *_staff;}       // return the staff
    inline Voice& voice() const {return *_voice;}       // return the voice the cursor points to
    
    void set(Staff& staff)                  noexcept;
    void set(Staff& staff, SubVoice& voice) noexcept;
    
    // modification methods (ownership of inserted object is with the voice object thereafter!)
    void insert(StaffObject* const object);
    void remove();
};


//
//     class const_Cursor
//    ====================
//
// This cursor points to a note in a score by containing pointers to the
// corresponding voice and staff and an iterator pointing to the note.
// It provides a unique interface for both main- and sub-voices.
// This is the constant version of the class.
//
class SCOREPRESS_API const_Cursor
{
 private:
    const Staff* _staff;   // pointer to the staff hosting the voice
    const Voice* _voice;   // pointer to the voice hosting the cursor (of type "MainVoice" or "SubVoice")
    
    StaffObjectList::const_iterator _main;     // iterator to the note (either in a main-voice or
    VoiceObjectList::const_iterator _sub;      //                       in a sub-voice)
    
    friend class Cursor;   // (for equality operators)
    
 public:
    // constructors
    const_Cursor();
    const_Cursor(const Staff& staff);
    const_Cursor(const Staff& staff, const SubVoice& voice);
    const_Cursor(const Cursor& cursor);
    
    // iterator interface
    inline const StaffObject& operator * () const  // return the Staff-Object the cursor points to
        {return (_staff == _voice) ? **_main : **_sub;}
    inline const StaffObject* operator-> () const  // return a pointer to the Staff-Object the cursor points to
        {return (_staff == _voice) ?  &**_main :  &**_sub;}
    inline const StaffObjectPtr& get_staffobject() const  // return smart-pointer to object
        {if (_staff != _voice) throw Cursor::IllegalObjectTypeException(); return *_main;}
    inline const VoiceObjectPtr& get_voiceobject() const  // return smart-pointer to object
        {if (_staff == _voice) throw Cursor::IllegalObjectTypeException(); return *_sub;}
    
    const_Cursor& operator ++ ();       // increment operator; move cursor to the next note (prefix)
    const_Cursor& operator -- ();       // decrement operator; move cursor to the previous note (prefix)
    const_Cursor  operator ++ (int);    // increment operator; move cursor to the next note (postfix)
    const_Cursor  operator -- (int);    // decrement operator; move cursor to the previous note (postfix)
    
    void reset();                   // reset the cursor to undefined state
    void to_end();                  // set cursor to the voice's end
    size_t index() const;           // return the note index within the voice
    size_t voice_length() const;    // return the length of the voice
    
    // equality operators
    bool operator == (const Cursor&)       const noexcept;
    bool operator != (const Cursor&)       const noexcept;
    bool operator == (const const_Cursor&) const noexcept;
    bool operator != (const const_Cursor&) const noexcept;
    
    // state reporters
    bool has_next() const;      // check, whether cursor can be incremented
    bool has_prev() const;      // check, whether cursor can be decremented
    bool at_end() const;        // check, if the cursor is at the end
    
    inline bool ready()   const {return (_staff && _voice);}
    inline bool is_main() const {return (_staff == _voice);}
    inline bool is_sub()  const {return (_staff != _voice);}
    
    // staff and voice access
    inline const Staff& staff() const {return *_staff;}     // return the staff
    inline const Voice& voice() const {return *_voice;}     // return the voice the cursor points to
    
    void set(const Staff& staff)                        noexcept;
    void set(const Staff& staff, const SubVoice& voice) noexcept;
};

} // end namespace

#endif


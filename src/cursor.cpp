
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

#include "cursor.hh"
using namespace ScorePress;


//
//     class Cursor
//    ==============
//
// This cursor points to a note in a score by containing pointers to the
// corresponding voice and staff and an iterator pointing to the note.
// It provides a unique interface for both main- and sub-voices.
// This is the non-constant version of the class.
//

// exception classes
Cursor::Error::Error(const std::string& msg) : ScorePress::Error(msg) {}
Cursor::IllegalObjectTypeException::IllegalObjectTypeException()
      : Error("You cannot insert a Staff-Object into a sub-voice.") {}

// constructors for the "Cursor" class
Cursor::Cursor() : _staff(NULL), _voice(NULL) {}
Cursor::Cursor(Staff& s) : _staff(&s), _voice(&s), _main(s.notes.begin()) {}
Cursor::Cursor(Staff& s, SubVoice& v) : _staff(&s), _voice(&v), _sub(v.notes.begin()) {}

// increment operator; move cursor to the next note (prefix)
Cursor& Cursor::operator ++ ()
{
    if (_staff == _voice) ++_main; else ++_sub;
    return *this;
}

// decrement operator; move cursor to the previous note (prefix)
Cursor& Cursor::operator -- ()
{
    if (_staff == _voice) --_main; else --_sub;
    return *this;
}

// increment operator; move cursor to the next note (postfix)
Cursor Cursor::operator ++ (int)
{
    Cursor out(*this);
    if (_staff == _voice) ++_main; else ++_sub;
    return out;
}

// decrement operator; move cursor to the previous note (postfix)
Cursor Cursor::operator -- (int)
{
    Cursor out(*this);
    if (_staff == _voice) --_main; else --_sub;
    return out;
}

// reset the cursor to undefined state
void Cursor::reset()
{
    _staff = NULL;
    _voice = NULL;
}

// set cursor to the voice's end
void Cursor::to_end()
{
    if (_staff == _voice) _main = static_cast<MainVoice*>(_voice)->notes.end();
                          _sub  = static_cast<SubVoice*>(_voice)->notes.end();
}

// return the note index within the voice
size_t Cursor::index() const
{
    size_t out = 0;
    if (_staff == _voice)
    {
        for (StaffObjectList::const_iterator i  = static_cast<MainVoice*>(_voice)->notes.begin();
             i != static_cast<MainVoice*>(_voice)->notes.end() && i != _main;
             ++i)
            ++out;
        return out;
    }
    else
    {
        for (VoiceObjectList::const_iterator i  = static_cast<SubVoice*>(_voice)->notes.begin();
             i != static_cast<SubVoice*>(_voice)->notes.end() && i != _sub;
             ++i)
            ++out;
        return out;
    };
}

// return the length of the voice
size_t Cursor::voice_length() const
{
    if (_staff == _voice) return static_cast<MainVoice*>(_voice)->notes.size();
    else                  return static_cast<SubVoice*>(_voice)->notes.size();
}

// equality operators
bool Cursor::operator == (const Cursor& cursor) const
{
    return    cursor._staff == _staff
           && cursor._voice == _voice
           && ((_staff == _voice) ? _main == cursor._main : _sub == cursor._sub);
}

bool Cursor::operator != (const Cursor& cursor) const
{
    return    cursor._staff != _staff
           || cursor._voice != _voice
           || ((_staff == _voice) ? _main != cursor._main : _sub != cursor._sub);
}

bool Cursor::operator == (const const_Cursor& cursor) const
{
    return    cursor._staff == _staff
           && cursor._voice == _voice
           && ((_staff == _voice) ? _main == cursor._main : _sub == cursor._sub);
}

bool Cursor::operator != (const const_Cursor& cursor) const
{
    return    cursor._staff != _staff
           || cursor._voice != _voice
           || ((_staff == _voice) ? _main != cursor._main : _sub != cursor._sub);
}

// check, whether cursor can be incremented
bool Cursor::has_next() const
{
    return (_staff == _voice) ? _main != --static_cast<MainVoice*>(_voice)->notes.end()
                              : _sub  != --static_cast<SubVoice*>(_voice)->notes.end();
}

// check, whether cursor can be decremented
bool Cursor::has_prev() const
{
    return (_staff == _voice) ? _main != static_cast<MainVoice*>(_voice)->notes.begin()
                              : _sub  != static_cast<SubVoice*>(_voice)->notes.begin();
}

// check, if the cursor is at the end
bool Cursor::at_end() const
{
    return (_staff == _voice) ? _main == static_cast<MainVoice*>(_voice)->notes.end()
                              : _sub  == static_cast<SubVoice*>(_voice)->notes.end();
}

// set functions
void Cursor::set(Staff& s)
{
    _main = s.notes.begin();
    _staff = &s;
    _voice = &s;
}

void Cursor::set(Staff& s, SubVoice& v)
{
    _sub = v.notes.begin();
    _staff = &s;
    _voice = &v;
}

// insert object before the referenced one
void Cursor::insert(StaffObject* const object) throw(IllegalObjectTypeException)
{
    if (_voice->is(Class::MAINVOICE))
    {
        _main = static_cast<MainVoice*>(_voice)->notes.insert(_main, StaffObjectPtr(object));
    }
    else if (!object->is(Class::VOICEOBJECT)) throw IllegalObjectTypeException();
    else if (_voice->is(Class::SUBVOICE))
    {
        _sub = static_cast<SubVoice*>(_voice)->notes.insert(_sub, VoiceObjectPtr(static_cast<VoiceObject*>(object)));
    };
}

// remove referenced object (if any)
void Cursor::remove()
{
    if (at_end()) return;
    if (_staff == _voice) static_cast<MainVoice*>(_voice)->notes.erase(_main++);
    else                  static_cast<SubVoice*>(_voice)->notes.erase(_sub++);
}


//
//     class const_Cursor
//    ====================
//
// This cursor points to a note in a score by containing pointers to the
// corresponding voice and staff and an iterator pointing to the note.
// It provides a unique interface for both main- and sub-voices.
// This is the constant version of the class.
//

// constructors for the "const_Cursor" class
const_Cursor::const_Cursor() : _staff(NULL), _voice(NULL) {}
const_Cursor::const_Cursor(const Staff& s) : _staff(&s), _voice(&s), _main(s.notes.begin()) {}
const_Cursor::const_Cursor(const Staff& s, const SubVoice& v) : _staff(&s), _voice(&v), _sub(v.notes.begin()) {}
const_Cursor::const_Cursor(const Cursor& c) : _staff(c._staff), _voice(c._voice), _main(c._main), _sub(c._sub) {}

// increment operator; move cursor to the next note (prefix)
const_Cursor& const_Cursor::operator ++ ()
{
    if (_staff == _voice) ++_main; else ++_sub;
    return *this;
}

// decrement operator; move cursor to the previous note (prefix)
const_Cursor& const_Cursor::operator -- ()
{
    if (_staff == _voice) --_main; else --_sub;
    return *this;
}

// increment operator; move cursor to the next note (postfix)
const_Cursor const_Cursor::operator ++ (int)
{
    const_Cursor out(*this);
    if (_staff == _voice) ++_main; else ++_sub;
    return out;
}

// decrement operator; move cursor to the previous note (postfix)
const_Cursor const_Cursor::operator -- (int)
{
    const_Cursor out(*this);
    if (_staff == _voice) --_main; else --_sub;
    return out;
}

// reset the cursor to undefined state
void const_Cursor::reset()
{
    _staff = NULL;
    _voice = NULL;
}

// set cursor to the voice's end
void const_Cursor::to_end()
{
    if (_staff == _voice) _main = static_cast<const MainVoice*>(_voice)->notes.end();
                          _sub  = static_cast<const SubVoice*>(_voice)->notes.end();
}

// return the note index within the voice
size_t const_Cursor::index() const
{
    size_t out = 0;
    if (_staff == _voice)
    {
        for (StaffObjectList::const_iterator i  = static_cast<const MainVoice*>(_voice)->notes.begin();
             i != static_cast<const MainVoice*>(_voice)->notes.end() && i != _main;
             ++i, ++out);
        return out;
    }
    else
    {
        for (VoiceObjectList::const_iterator i  = static_cast<const SubVoice*>(_voice)->notes.begin();
             i != static_cast<const SubVoice*>(_voice)->notes.end() && i != _sub;
             ++i, ++out);
        return out;
    };
}

// return the length of the voice
size_t const_Cursor::voice_length() const
{
    if (_staff == _voice) return static_cast<const MainVoice*>(_voice)->notes.size();
    else                  return static_cast<const SubVoice*>(_voice)->notes.size();
}

// equality operators
bool const_Cursor::operator == (const Cursor& cursor) const
{
    return    cursor._staff == _staff
           && cursor._voice == _voice
           && ((_staff == _voice) ? _main == cursor._main : _sub == cursor._sub);
}

bool const_Cursor::operator != (const Cursor& cursor) const
{
    return    cursor._staff != _staff
           || cursor._voice != _voice
           || ((_staff == _voice) ? _main != cursor._main : _sub != cursor._sub);
}

bool const_Cursor::operator == (const const_Cursor& cursor) const
{
    return    cursor._staff == _staff
           && cursor._voice == _voice
           && ((_staff == _voice) ? _main == cursor._main : _sub == cursor._sub);
}

bool const_Cursor::operator != (const const_Cursor& cursor) const
{
    return    cursor._staff != _staff
           || cursor._voice != _voice
           || ((_staff == _voice) ? _main != cursor._main : _sub != cursor._sub);
}

// check, whether cursor can be incremented
bool const_Cursor::has_next() const
{
    return (_staff == _voice) ? (_main != --static_cast<const MainVoice*>(_voice)->notes.end() &&
                                 _main !=   static_cast<const MainVoice*>(_voice)->notes.end())
                              : (_sub  != --static_cast<const SubVoice*>(_voice)->notes.end()  &&
                                 _sub  !=   static_cast<const SubVoice*>(_voice)->notes.end());
}

// check, whether cursor can be decremented
bool const_Cursor::has_prev() const
{
    return (_staff == _voice) ? _main != static_cast<const MainVoice*>(_voice)->notes.begin()
                              : _sub  != static_cast<const SubVoice*>(_voice)->notes.begin();
}

// check, if the cursor is at the end
bool const_Cursor::at_end() const
{
    return (_staff == _voice) ? _main == static_cast<const MainVoice*>(_voice)->notes.end()
                              : _sub  == static_cast<const SubVoice*>(_voice)->notes.end();
}

// set functions
void const_Cursor::set(const Staff& s)
{
    _main = s.notes.begin();
    _staff = &s;
    _voice = &s;
}

void const_Cursor::set(const Staff& s, const SubVoice& v)
{
    _sub = v.notes.begin();
    _staff = &s;
    _voice = &v;
}


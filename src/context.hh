
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

#ifndef SCOREPRESS_CONTEXT_HH
#define SCOREPRESS_CONTEXT_HH

#include <map>          // std::map
#include "classes.hh"   // [score classes]
#include "error.hh"     // ScorePress::Error

namespace ScorePress
{
//  CLASSES
// ---------
class VoiceContext;     // voice-context with volume, measure and accidental information
class StaffContext;     // staff-context with clef and key information
class LineContext;      // line-context with newline objects
class ScoreContext;     // score-context with current tempo


//
//     class VoiceContext
//    ====================
//
// This class represents a voice-context, containing information about the
// current tempo and volume. It provides methods manipulating the instance
// according to context-changing objects and calculating information for the
// engraver dependant on the current context.
//
class VoiceContext
{
 private:
    // dynamic context
    int _volume;                    // current volume (0..127)
    unsigned int _value_modifier;   // note-length' multiplicator during playback (in promille)
    
    // measure context
    TimeSig _time_sig;              // current time-signature
    value_t _time_time;             // time-position of that signature
    unsigned int _time_bar;         // bar index of the time-signature
    
    // last object buffer
    struct Buffer
    {
        const StaffObject* object;  // last engraved object of the voice
        mpx_t xpos;                 // horizontal position of the last engraved object
    } _buffer;
    
 public:
    // constructors
    VoiceContext();     // default voice-context (full volume, 85% value modifier, 4/4 time-signature)
    VoiceContext(const VoiceContext& context);      // copy constructor
    
    // accessors
    unsigned int bar(const value_t time) const;     // calculate the index of the bar, containing the given time
    value_t beat(const value_t time) const;         // calculate the beat inside the bar (modulo operation)
    value_t restbar(const value_t time) const;      // calculate the value of the remaining bar
    const TimeSig& last_timesig() const;            // return the last time-signature
    
    // modifiers
    void modify(const ContextChanging& changer, bool vol);  // let the context-changing instance change this context
    void modify(const TimeSig& timesig, value_t time);      // set new time-signature
    
    // last object access
    void set_buffer(const StaffObject* object);     // set last engraved object
    const StaffObject* get_buffer() const;          // get last engraved object
    bool has_buffer() const;                        // has last engraved object?
    
    void set_buffer_xpos(const mpx_t xpos);         // set last object position
    mpx_t get_buffer_xpos() const;                  // get last object position
};

// inline method implementations
inline       unsigned int VoiceContext::bar(const value_t time)         const {return _time_bar + ((time - _time_time) / static_cast<value_t>(_time_sig)).i();}
inline       value_t      VoiceContext::beat(const value_t time)        const {return (time - _time_time) % static_cast<value_t>(_time_sig);}
inline       value_t      VoiceContext::restbar(const value_t time)     const {return _time_sig - beat(time);}
inline const TimeSig&     VoiceContext::last_timesig()                  const {return _time_sig;}
inline       void         VoiceContext::set_buffer(const StaffObject* object) {_buffer.object = object;}
inline const StaffObject* VoiceContext::get_buffer()                    const {return _buffer.object;}
inline       bool         VoiceContext::has_buffer()                    const {return _buffer.object != NULL;}
inline       void         VoiceContext::set_buffer_xpos(const mpx_t xpos)     {_buffer.xpos = xpos;}
inline       mpx_t        VoiceContext::get_buffer_xpos()               const {return _buffer.xpos;}


//
//     class StaffContext
//    ====================
//
// This class represents a staff-context, containing information about the
// current clef and key. It provides methods manipulating the instance
// according to clef and key objects and calculating information for the
// engraver dependant on the current context.
//
class StaffContext
{
 public:
    // exception classes
    class Error : public ScorePress::Error {public: Error(const std::string& msg) : ScorePress::Error(msg) {};};
    class IllegalBasenoteException : public Error   // thrown, if a clef's base-note is not a whole tone
    {public: IllegalBasenoteException() : Error("Found illegal base-note for clef object.") {};};
    class IllegalAccidentalException : public Error // thrown, if the tone cannot be expressed using the requested accidental
    {public: IllegalAccidentalException() : Error("Found illegal accidental on head-instance.") {};};
    class IllegalKeyException : public Error        // thrown, if the index of the key-signature symbol is out of range 0-6
    {public: IllegalKeyException() : Error("Found illegal key-signature.") {};};
    
    typedef std::map<int, Accidental::Type> AccidentalMap;
    
 private:
    // clef context
    Clef    _clef;                  // copy of the last clef instance
    Key     _key;                   // copy of the last key instance
    tone_t  _base_note;             // note in first space from top
    
    // key/accidentals context
    unsigned short _key_acc;        // current key accidentals (14 bit, one per note and accidental)
    AccidentalMap  _accidentals;    // currently used accidentals
    
 public:
    // constructors
    StaffContext();                 // default staff-context (treble clef, C major key)
    StaffContext(const StaffContext& context);      // copy constructor
    
    // member access
    const Clef&    last_clef()    const;
    const Key&     last_key()     const;
    const tone_t&  base_note()    const;
    const tone_t&  keybnd_sharp() const;
    const tone_t&  keybnd_flat()  const;
    
    // modifiers
    void modify(const Key& key);                    // set new key
    void modify(const Clef& clef) throw(IllegalBasenoteException);  // set new base-note as specified by the clef
    
    void remember_acc(const Head& head);            // remember accidental
    void reset_acc();                               // reset memorized accidentals
    
    // modificators
    mpx_t note_offset(const Head& head, mpx_t head_height)        const throw(IllegalAccidentalException);  // calculate the offset for the note
    mpx_t key_offset(Key::Type type, char idx, mpx_t head_height) const throw(IllegalKeyException);         // calculate the offset for the key
    int   ledger_count(const Head& head)                          const throw(IllegalAccidentalException);  // calculate ledger line count
    
    // state checkers
    bool             on_line(const Head& head)                          const;          // check if the head lies on a line
    bool             acc_visible(const Head& head)                      const;          // check if the accidental is visible
    static bool      check_accidental(const tone_t tone, const Accidental::Type type);  // check if the accidental is OK
    Accidental::Type get_key_accidental(const tone_t tone)              const;          // check if the tone got a key accidental
    Accidental::Type guess_accidental(const tone_t tone, bool pref_nat) const;          // calculate a nice accidental for the tone
};

// inline method implementations
inline const Clef&    StaffContext::last_clef()                   const {return _clef;}
inline const Key&     StaffContext::last_key()                    const {return _key;}
inline const tone_t&  StaffContext::base_note()                   const {return _clef.base_note;}
inline const tone_t&  StaffContext::keybnd_sharp()                const {return _clef.keybnd_sharp;}
inline const tone_t&  StaffContext::keybnd_flat()                 const {return _clef.keybnd_flat;}
inline       void     StaffContext::reset_acc()                         {_accidentals.clear();}
inline       bool     StaffContext::on_line(const Head& head)     const {return (note_offset(head, 2) % 2 == 1);}


//
//     class ScoreContext
//    ====================
//
// This class represents a global score-context, containing the current tempo.
//
class ScoreContext
{
 private:
    int _tempo;        // current tempo (in bpm)
    
 public:
    // constructor
    ScoreContext();    // default score-context (120bpm)
    
    // modifiers
    void modify(const ContextChanging& changer);   // let the context-changing instance cahnge the tempo
};

} // end namespace

#endif


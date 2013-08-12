
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

#include "context.hh"    // Context, [score classes]
using namespace ScorePress;


//                                C C#  D D#  E  F F#  G G#  A A#  B
//                                0  1  2  3  4  5  6  7  8  9 10 11
static const int wholetone[12] = {0, 0, 2, 2, 4, 5, 5, 7, 7, 9, 9,11};  // the next lower whole tone
static const int whole_off[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};  // the offset for each next lower whole tone 

static const int tone_off[7]  = {0, 2, 4, 5, 7, 9, 11};  // inverse array to whole_off (tone by offset)


//
//     class VoiceContext
//    ====================
//
// This class represents a voice-context, containing information about the
// current tempo and volume. It provides methods manipulating the instance
// according to context-changing objects and calculating information for the
// engraver dependant on the current context.
//


// default voice-context (full volume, 85% value modifier, 4/4 time-signature)
VoiceContext::VoiceContext() : _volume(127),
                               _value_modifier(850),
                               _time_sig(),
                               _time_time(0),
                               _time_bar(0) {
                               _buffer.object = NULL;
                               _buffer.xpos = 0;
                               _buffer2.object = NULL;
                               _buffer2.xpos = 0;}

// copy constructor
VoiceContext::VoiceContext(const VoiceContext& context) : 
                               _volume(context._volume),
                               _value_modifier(context._value_modifier),
                               _time_sig(context._time_sig),
                               _time_time(context._time_time),
                               _time_bar(context._time_bar),
                               _buffer(context._buffer),
                               _buffer2(context._buffer2) {}

// let the context-changing instance change this context
void VoiceContext::modify(const ContextChanging& changer, bool vol)
{
    if (vol)
    switch(changer.volume_type)    // modify the volume
    {
    case ContextChanging::NONE: break;                                 // do nothing
    case ContextChanging::ABSOLUTE: _volume = changer.volume; break;   // set to absolute value
    case ContextChanging::RELATIVE: _volume += changer.volume; break;  // add value
    case ContextChanging::PROMILLE: _volume = (_volume * changer.volume) / 1000; break;    // modify multiplicatively
    };
    
    if (changer.value_modifier != 0) _value_modifier = changer.value_modifier; // change value-modifier
}

// set new time-signature
void VoiceContext::modify(const TimeSig& timesig, value_t time)
{
    _time_bar += ((time - _time_time) / value_t(_time_sig)).i();   // calculate current bar
    _time_sig = timesig;   // save new time-signature
    _time_time = time;     // save current time
}


//
//     class StaffContext
//    ====================
//
// This class represents a staff-context, containing information about the
// current clef and key. It provides methods manipulating the instance
// according to claf and key objects and calculating information for the
// engraver dependant on the current context.
//

// exception classes
StaffContext::Error::Error(const std::string& msg) : ScorePress::Error(msg) {}
StaffContext::IllegalBasenoteException::IllegalBasenoteException()
            : Error("Found illegal base-note for clef object.") {}
StaffContext::IllegalAccidentalException::IllegalAccidentalException()
            : Error("Found illegal accidental on head-instance.") {}
StaffContext::IllegalKeyException::IllegalKeyException()
            : Error("Found illegal key-signature.") {}

// default staff-context (treble clef, C major key)
StaffContext::StaffContext() : _clef(), _key(),
                               _base_note(76),
                               _key_acc(0x0000) {}

// copy constructor
StaffContext::StaffContext(const StaffContext& context) : _clef(context._clef),
                                                          _key(context._key),
                                                          _base_note(context._base_note),
                                                          _key_acc(context._key_acc),
                                                          _accidentals(context._accidentals) {}

// set new key
void StaffContext::modify(const Key& key)
{
    _key = key;             // save key instance
    _key_acc = 0x0000;      // set key-signature
    if (key.type == Key::SHARP)
    {
        if (key.number >= 1) {_key_acc |= 0x0008;   // F#
        if (key.number >= 2) {_key_acc |= 0x0001;   // C#
        if (key.number >= 3) {_key_acc |= 0x0010;   // G#
        if (key.number >= 4) {_key_acc |= 0x0002;   // D#
        if (key.number >= 5) {_key_acc |= 0x0020;   // A#
        if (key.number >= 6) {_key_acc |= 0x0004;   // E#
        if (key.number >= 7) {_key_acc |= 0x0040;   // B#
        };};};};};};};
    }
    else if (key.type == Key::FLAT)
    {
        if (key.number >= 1) {_key_acc |= 0x4000;   // Bb
        if (key.number >= 2) {_key_acc |= 0x0400;   // Eb
        if (key.number >= 3) {_key_acc |= 0x2000;   // Ab
        if (key.number >= 4) {_key_acc |= 0x0200;   // Db
        if (key.number >= 5) {_key_acc |= 0x1000;   // Gb
        if (key.number >= 6) {_key_acc |= 0x0100;   // Cb
        if (key.number >= 7) {_key_acc |= 0x0800;   // Fb
        };};};};};};};
    };
    _accidentals.clear();
}

// set new base-note as specified by the clef
void StaffContext::modify(const Clef& clef) throw(IllegalBasenoteException)
{
    if (clef.sprite.setid > 10000) throw IllegalBasenoteException();
    if (wholetone[clef.base_note % 12] != clef.base_note % 12) // check if the base note is a whole tone
        throw IllegalBasenoteException();
    
    _clef = clef;   // save clef
    
    // calculate the base_note
    _base_note = static_cast<tone_t>(12 * ((clef.base_note / 12) + (whole_off[clef.base_note % 12] + clef.line - 1) / 7) + tone_off[(whole_off[clef.base_note % 12] + clef.line - 1) % 7]);
}

// remember accidental
void StaffContext::remember_acc(const Head& head)
{
    // get the note name and octave
    int tone_name = (static_cast<int>(head.tone) - Accidental::note_modifier[head.accidental.type]) % 12;
    if (tone_name < 0) tone_name += 12;        // ensure positive
    int tone_octave = (static_cast<int>(head.tone) - Accidental::note_modifier[head.accidental.type]) / 12;
    
    // save accidental
    _accidentals[whole_off[tone_name] + 7 * tone_octave] = head.accidental.type;
}

// calculate the vertical offset for the given note according to the current clef
mpx_t StaffContext::note_offset(const Head& head, mpx_t head_height) const throw(IllegalAccidentalException)
{
    // get the note name and octave
    int tone_name = (static_cast<int>(head.tone) - Accidental::note_modifier[head.accidental.type]) % 12;
    if (tone_name < 0) tone_name += 12;        // ensure positive
    int tone_octave = (static_cast<int>(head.tone) - Accidental::note_modifier[head.accidental.type]) / 12;
    
    if (tone_name != wholetone[tone_name])     // if the accidental is illegal, print a warning
        throw IllegalAccidentalException();
    
    // calculate the offset with the distance-array "whole_off"
    return head_height * (whole_off[_base_note % 12] - whole_off[tone_name] + 7 * (_base_note / 12 - tone_octave)) / 2;
}

// calculate the offset for the idx-th symbol of the given key-signature
mpx_t StaffContext::key_offset(Key::Type type, char idx, mpx_t head_height) const throw(IllegalKeyException)
{
    // local variables
    tone_t bound = (type == Key::SHARP) ? _clef.keybnd_sharp : _clef.keybnd_flat;    // key-signature bound
    int tone_name = 0;                         // tone name (corresponding to symbol)
    int tone_octave;                           // octave (of the symbol)
    
    // get the tone name
    switch((type == Key::SHARP) ? idx : 6 - idx)
    {
    case 0: tone_name =  5; break;
    case 1: tone_name =  0; break;
    case 2: tone_name =  7; break;
    case 3: tone_name =  2; break;
    case 4: tone_name =  9; break;
    case 5: tone_name =  4; break;
    case 6: tone_name = 11; break;
    default: throw IllegalKeyException();
    };
    
    // calculate the octave
    if (bound % 12 <= tone_name) tone_octave = bound / 12;
    else tone_octave = bound / 12 + 1;
    
    // calculate the offset with the distance-array "whole_off"
    return head_height * (whole_off[_base_note % 12] - whole_off[tone_name] + 7 * (_base_note / 12 - tone_octave)) / 2;
}

// calculate ledger line count (for a one-line staff)
int StaffContext::ledger_count(const Head& head) const throw(IllegalAccidentalException)
{
    // get the note name and octave
    int tone_name = (static_cast<int>(head.tone) - Accidental::note_modifier[head.accidental.type]) % 12;
    if (tone_name < 0) tone_name += 12;        // ensure positive
    int tone_octave = (static_cast<int>(head.tone) - Accidental::note_modifier[head.accidental.type]) / 12;
    
    if (tone_name != wholetone[tone_name])     // if the accidental is illegal, print a warning
        throw IllegalAccidentalException();
    
    // calculate the offset with the distance-array "whole_off"
    return (1 + whole_off[_base_note % 12] - whole_off[tone_name] + 7 * (_base_note / 12 - tone_octave)) / 2;
}

// check if the accidental of the given note is visible according to the current key
bool StaffContext::acc_visible(const Head& head) const
{
    if (!head.accidental.appearance.visible)    // if the accidental is marked as invisible,
        return false;                           //    return false
    
    // get the note name and octave
    int tone_name = (static_cast<int>(head.tone) - Accidental::note_modifier[head.accidental.type]) % 12;
    if (tone_name < 0) tone_name += 12;        // ensure positive
    int tone_octave = (static_cast<int>(head.tone) - Accidental::note_modifier[head.accidental.type]) / 12;
    
    // check currently used accidentals
    AccidentalMap::const_iterator i = _accidentals.find(whole_off[tone_name] + 7 * tone_octave);
    if (i != _accidentals.end())
        return (head.accidental.type != i->second);
    
    // calculate accidental flag mask
    unsigned short tone_byte = static_cast<unsigned short>(1u << whole_off[tone_name]);
    
    // check visibility
    switch (head.accidental.type)
    {
    case Accidental::sharp:   return ((_key_acc & tone_byte) == 0);
    case Accidental::flat:    return ((_key_acc & (tone_byte << 8)) == 0);
    case Accidental::natural: return (((_key_acc & tone_byte) | (_key_acc & (tone_byte << 8))) != 0);
    default: return true;
    };
}

// check if the accidental is OK
bool StaffContext::check_accidental(const tone_t tone, const Accidental::Type type)
{
    // get the note name and octave
    int tone_name = (static_cast<int>(tone) - Accidental::note_modifier[type]) % 12;
    if (tone_name < 0) tone_name += 12;            // ensure positive
    return (tone_name == wholetone[tone_name]);    // check accidental
}

// check if the tone got a key accidental
Accidental::Type StaffContext::get_key_accidental(const tone_t tone) const
{
    // calculate accidental flag mask
    unsigned short tone_byte = static_cast<unsigned short>(1u << whole_off[tone % 12]);
    
    // check current key
    if (_key_acc & tone_byte)        return Accidental::sharp;
    if (_key_acc & (tone_byte << 8)) return Accidental::flat;
    return Accidental::natural;
}

// calculate a nice accidental for the tone
Accidental::Type StaffContext::guess_accidental(const tone_t tone, bool pref_nat) const
{
    // calculate accidental flag mask
    unsigned short tone_byte = static_cast<unsigned short>(1u << whole_off[tone % 12]);
    
    // if we got a whole tone
    if (tone % 12 == wholetone[tone % 12])
    {
        if (   pref_nat
            || ((_key_acc & tone_byte) | (_key_acc & (tone_byte << 8))) == 0)
                return Accidental::natural;
        if (_key_acc & 0x4000)              // flat keys
        {
            if (tone % 12 == 4 || tone % 12 == 11)  // Fb and Cb get only one flat
                return Accidental::flat;
            return Accidental::double_flat;
        }
        else                                // sharp keys (and C-major/a-minor)
        {
            if (tone % 12 == 0 || tone % 12 == 5)  // B# and E# get only one sharp
                return Accidental::sharp;
            return Accidental::double_sharp;
        }
    }
    // if we got a semi-tone
    else
    {
        if (_key_acc & 0x4000)              // flat keys
            return Accidental::flat;
        else                                // sharp keys (and C-major/a-minor)
            return Accidental::sharp;
    };
}


//
//     class ScoreContext
//    ====================
//
// This class represents a global score-context, containing the current tempo.
//

// default score-context (120bpm)
ScoreContext::ScoreContext() : _tempo(120) {}

// let the context-changing instance change this context
void ScoreContext::modify(const ContextChanging& changer)
{
    switch(changer.tempo_type)     // modify the tempo
    {
    case ContextChanging::NONE: break;                                 // do nothing
    case ContextChanging::ABSOLUTE: _tempo = changer.tempo; break;     // set to absolute value
    case ContextChanging::RELATIVE: _tempo += changer.tempo; break;    // add value
    case ContextChanging::PROMILLE: _tempo = (_tempo * changer.tempo) / 1000; break;   // modify multiplicatively
    };
}


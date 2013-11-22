
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

#include "engrave_info.hh"     // BeamInfo

using namespace ScorePress;

//
//     class BeamInfo
//    ================
//
// Information about the beams within the currently
// engraved group of notes. This is used for the real beam
// engraving during the line-postprocessing.
//

// FIXME: Prevent short 8th beam

// set the given beam information to be ending at the given note
void BeamInfo::set(size_t beam_idx, size_t end_idx, Plate::pNote& begin, const Plate::pNote& end, const Plate::pNote& cur)
{
    if (!begin.beam[beam_idx])              // if there is no beam info
        begin.beam[beam_idx] = RefPtr<Plate::pNote::Beam>(new Plate::pNote::Beam());   // create one
    
    begin.beam[beam_idx]->end_idx = end_idx % 0x10;         // end stem index
    begin.beam[beam_idx]->short_beam = (&begin == &end);    // short beam?
    
    // set end-note reference
    if (&begin == &end)                     // if we have got a short beam
        begin.beam[beam_idx]->end = &cur;   //   let this next note be the reference
    else begin.beam[beam_idx]->end = &end;  // normal beam end note
}

// start necessary beams
void BeamInfo::start(unsigned int exp, std::list<Plate::pNote>::iterator pnote)
{
    for (size_t i = exp; i < VALUE_BASE - 2; ++i)   // add new beams
    {
        if (beam[i] != voice.notes.end()) break;    // stop, if further beams exist already
        beam[i] = pnote;                            // add beam to this note
    };
}

inline static bool equal_dir(const Plate::pNote& n1, const Plate::pNote& n2)
{
    return !((n1.stem.top < n1.stem.base) ^ (n2.stem.top < n2.stem.base));
}

// end all (for the given exponent) unnecessary beams at the previous note
void BeamInfo::set(unsigned int exp, std::list<Plate::pNote>::iterator pnote)
{
    if (pnote == voice.notes.begin()) return;
    std::list<Plate::pNote>::iterator end(pnote);
    --end;
    
    size_t s = 0;
    for (size_t i = 0; i < exp; ++i)    // check every beam we don't need...
    {
        if (beam[i] != voice.notes.end())   // ...if it exists
        {                                   //   let the beam go up to the given end-note
            if (equal_dir(*beam[i], *end))
                set(i, VALUE_BASE - 3 - i + s, *beam[i], *end, *pnote); 
            else
                set(i, (i < VALUE_BASE - 3) ? s++ : s, *beam[i], *end, *pnote);
            
            // change short beam direction on inconvienient next note
            if (   beam[i]->beam[i]->short_beam     // if there is a short beam, 
                && i < exp - 1                      // the next note does not even have the adjacent beam
                && beam[i + 1] != voice.notes.end() // but the beam exists
                && beam[i + 1] != beam[i])          // and really begins at the previous note
            {
                beam[i]->beam[i]->short_left = true;        // let the beam face left
                if (beam[i] != beam[VALUE_BASE - 3])
                    beam[i]->beam[i]->end = &*--beam[i];
                if (s) --s;
            };
            
            beam[i] = voice.notes.end();    //   and remove it from the array
        };
    };
    
    // count irregular beam positions (for stem length)
    for (size_t i = exp; i < VALUE_BASE - 3; ++i)
    {
        if (beam[i] != voice.notes.end())   // if the beam exists...
        {                                   // ...increment counter on unequal stem direction
            if (!equal_dir(*beam[i], *end))
                ++s;                        
        };
    };
    end->stem.beam_off = s;
}

// end all existing beams at this note
void BeamInfo::stop(unsigned int exp, std::list<Plate::pNote>::iterator end)
{
    size_t s = 0;
    for (size_t i = exp; i < VALUE_BASE - 2; ++i)   // check the remaining beams
    {
        if (beam[i] != voice.notes.end())           // if it exists
        {                                           //   let the beam go up to this note
            if (equal_dir(*beam[i], *end))
                set(i, VALUE_BASE - 3 - i + s, *beam[i], *end, *end); 
            else
                set(i, (i < VALUE_BASE - 3) ? s++ : s, *beam[i], *end, *end);
            
            // change short beam direction (because we are at the end)
            if (beam[i]->beam[i]->short_beam)
            {
                beam[i]->beam[i]->short_left = true;
                if (beam[i] != beam[VALUE_BASE - 3])
                    beam[i]->beam[i]->end = &*--beam[i];
                if (s) --s;
            };
            beam[i] = voice.notes.end();            //   and remove it from the array
        };
    };
    end->stem.beam_off = s;
}

// let each beam end on the previous note (indicating that the current symbol does not allow beams)
void BeamInfo::cut(std::list<Plate::pNote>::iterator pnote)
{
    if (pnote == voice.notes.begin()) return;
    std::list<Plate::pNote>::iterator end(pnote);
    --end;
    size_t s = 0;
    for (size_t i = 0; i < VALUE_BASE - 2; ++i)     // check the remaining beams
    {
        if (beam[i] != voice.notes.end())           // if it exists
        {                                           //   end the beam on the previous note
            if (equal_dir(*beam[i], *end))
                set(i, VALUE_BASE - 3 - i + s, *beam[i], *end, *pnote);
            else
                set(i, (i < VALUE_BASE - 3) ? s++ : s, *beam[i], *end, *pnote);
            
            // change short beam direction (because we are at the end of the beam group)
            if (beam[i]->beam[i]->short_beam)
            {
                beam[i]->beam[i]->short_left = true;
                if (beam[i] != beam[VALUE_BASE - 3])
                    beam[i]->beam[i]->end = &*--beam[i];
                if (s) --s;
            };
            beam[i] = voice.notes.end();            //   and remove it from the array
        };
    };
    end->stem.beam_off = s;
}

// constructor
BeamInfo::BeamInfo(Plate::pVoice& _voice) : voice(_voice), last_pnote(voice.notes.end()), last_chord(NULL)
{
    for (size_t i = 0; i < VALUE_BASE - 2; ++i) beam[i] = voice.notes.end();
}

// calculate beam information and adjust stem lengths
// (expecting the "object" to correspond to the last note in the "voice")
void BeamInfo::apply(const Chord& object, const unsigned char beam_group)
{
    if (voice.notes.empty()) return;
    apply(object, --voice.notes.end(), voice.end_time, beam_group);
}

void BeamInfo::apply(const Chord& object, std::list<Plate::pNote>::iterator pnote, const value_t end_time, const unsigned char beam_group)
{
    const bool has_beam = object.beam == Chord::FORCE_BEAM
                       || object.beam == Chord::CUT_BEAM
                       || (   object.beam == Chord::AUTO_BEAM
                           &&    end_time.i() / (1 << beam_group)
                              == (end_time - object.value()).i() / (1 << beam_group)
                          );
    
    const unsigned int& exp = object.val.exp;
    
    // if the previous note cut all beams
    if (last_chord && last_chord->beam == Chord::CUT_BEAM)
        set(VALUE_BASE - 3, pnote);     //   end all beams but one on the previous
    
    if (has_beam && exp < VALUE_BASE - 2)   // if the note has got a beam
    {
        if (beam[exp] == voice.notes.end()) // and there are not sufficient beams attached already
            start(exp, pnote);              //   add additional beams
        else if (exp != 0)                  // if there are sufficient beams (and we could need less)
            set(exp, pnote);                //   end unnecessary beams at the previous note
        
        last_chord = &object;               // remember last chord
        last_pnote = pnote;                 // and last pnote
    }
    else if (beam[VALUE_BASE - 3] != voice.notes.end()) // if the note does not have a beam
    {                                                   // but the array is not empty yet
        // erase remembered chord
        last_chord = NULL;
        
        // remove flags
        for (std::list<Plate::pNote>::iterator i = beam[VALUE_BASE - 3]; i != voice.notes.end(); ++i)
        {
            i->noflag = true;
            if (i == pnote) break;
        };
        
        // end all beams
        if (exp >= VALUE_BASE - 2)  // if this note cannot have a beam
        {
            return cut(pnote);      //   just cut and return
        }
        else    // otherwise the beams end on this note
        {
            if (beam[exp] == voice.notes.end())     // if there are not sufficient beams attached already
                start(exp, pnote);                  //   add necessary beams
            set(exp, pnote);            // end unnecessary beams
            stop(exp, pnote);           // end existing beams
        };
    }
    else last_chord = NULL;     // erase remembered chord
}

// end all beams
void BeamInfo::finish()
{
    if (!last_chord) return;
    if (beam[VALUE_BASE - 3] == voice.notes.end()) return;
    if (beam[VALUE_BASE - 3] == last_pnote) return;
    
    // remove flags
    for (std::list<Plate::pNote>::iterator i = beam[VALUE_BASE - 3]; i != last_pnote; ++i)
        i->noflag = true;
    last_pnote->noflag = true;
    
    // end existing beams
    stop(last_chord->val.exp, last_pnote);
}


//
//   class BeamGroupInfo
//  =====================
//
// Inforation about beam groups (i.e. only top beam).
// This is used during the engraving to prepare the real
// engraving of the beams during line-postprocessing.
//

// set the given beam information to be ending at the given note
void BeamGroupInfo::set(size_t beam_idx, size_t end_idx, Plate::pNote& begin, const Plate::pNote& end, const Plate::pNote& cur)
{
    if (!begin.beam[beam_idx])              // if there is no beam info
        begin.beam[beam_idx] = RefPtr<Plate::pNote::Beam>(new Plate::pNote::Beam());   // create one
    
    begin.beam[beam_idx]->end_idx = end_idx % 0x10;         // end stem index
    begin.beam[beam_idx]->short_beam = (&begin == &end);    // short beam?
    
    // set end-note reference
    if (&begin == &end)                     // if we have got a short beam
        begin.beam[beam_idx]->end = &cur;   //   let this next note be the reference
    else begin.beam[beam_idx]->end = &end;  // normal beam end note
}

// start necessary beams
void BeamGroupInfo::start(unsigned int exp)
{
    for (size_t i = exp; i < VALUE_BASE - 2; ++i)   // add new beams
    {
        if (beam[i] != voice.notes.end()) break;    // stop, if further beams exist already
        --beam[i];                                  // add beam to this note
    };
}

// end all (for the given exponent) unnecessary beams at the previous note
void BeamGroupInfo::set(unsigned int exp, std::list<Plate::pNote>::iterator pnote)
{
    if (pnote == voice.notes.begin()) return;
    std::list<Plate::pNote>::iterator end(pnote);
    --end;
    size_t s = 0;
    for (size_t i = 0; i < exp; ++i)    // check every beam we don't need...
    {
        if (beam[i] != voice.notes.end())   // ...if it exists
        {                                   //   let the beam go up to the given end-note
            if (equal_dir(*beam[i], *end))
                set(i, VALUE_BASE - 3 - i + s, *beam[i], *end, *pnote); 
            else
                set(i, (i < VALUE_BASE - 3) ? s++ : s, *beam[i], *end, *pnote);
            
            // change short beam direction on inconvienient next note
            if (   beam[i]->beam[i]->short_beam     // if there is a short beam, 
                && i < exp - 1                      // the next note does not even have the adjacent beam
                && beam[i + 1] != voice.notes.end() // but the beam exists
                && beam[i + 1] != beam[i])          // and really begins at the previous note
            {
                beam[i]->beam[i]->short_left = true;        // let the beam face left
                if (beam[i] != beam[VALUE_BASE - 3])
                    beam[i]->beam[i]->end = &*--beam[i];
                if (s) --s;
            };
            
            beam[i] = voice.notes.end();    //   and remove it from the array
        };
    };
    for (size_t i = exp; i < VALUE_BASE - 3; ++i)   // count irregular beam positions (for stem length)
    {
        if (beam[i] != voice.notes.end())   // if the beam exists...
        {                                   // ...increment counter on unequal stem direction
            if (!equal_dir(*beam[i], *end))
                ++s;                        
        };
    };
    end->stem.beam_off = s;
}

// end all existing beams at this note
void BeamGroupInfo::stop(unsigned int exp, std::list<Plate::pNote>::iterator end)
{
    size_t s = 0;
    for (size_t i = exp; i < VALUE_BASE - 2; ++i)   // check the remaining beams
    {
        if (beam[i] != voice.notes.end())           // if it exists
        {                                           //   let the beam go up to this note
            if (equal_dir(*beam[i], *end))
                set(i, VALUE_BASE - 3 - i + s, *beam[i], *end, *end); 
            else
                set(i, (i < VALUE_BASE - 3) ? s++ : s, *beam[i], *end, *end);
            
            if (beam[i]->beam[i]->short_beam)
            {
                beam[i]->beam[i]->short_left = true;
                if (beam[i] != beam[VALUE_BASE - 3])
                    beam[i]->beam[i]->end = &*--beam[i];
                if (s) --s;
            };
            beam[i] = voice.notes.end();            //   and remove it from the array
        };
    };
    end->stem.beam_off = s;
}

// let each beam end on the previous note (indicating that the current symbol does not allow beams)
void BeamGroupInfo::cut(std::list<Plate::pNote>::iterator pnote)
{
    if (pnote == voice.notes.begin()) return;
    std::list<Plate::pNote>::iterator end(pnote);
    --end;
    size_t s = 0;
    for (size_t i = 0; i < VALUE_BASE - 2; ++i)     // check the remaining beams
    {
        if (beam[i] != voice.notes.end())           // if it exists
        {                                           //   end the beam on the previous note
            if (equal_dir(*beam[i], *end))
                set(i, VALUE_BASE - 3 - i + s, *beam[i], *end, *pnote);
            else
                set(i, (i < VALUE_BASE - 3) ? s++ : s, *beam[i], *end, *pnote);
            
            if (beam[i]->beam[i]->short_beam)
            {
                beam[i]->beam[i]->short_left = true;
                if (beam[i] != beam[VALUE_BASE - 3])
                    beam[i]->beam[i]->end = &*--beam[i];
                if (s) --s;
            };
            beam[i] = voice.notes.end();            //   and remove it from the array
        };
    };
    end->stem.beam_off = s;
}

// constructor
BeamGroupInfo::BeamGroupInfo(Plate::pVoice& _voice) : voice(_voice), last_pnote(voice.notes.end()), last_chord(NULL)
{
    for (size_t i = 0; i < VALUE_BASE - 2; ++i) beam[i] = voice.notes.end();
}

// calculate beam information and adjust stem lengths
// (expecting the "object" to correspond to the last note in the "voice")
void BeamGroupInfo::apply(const Chord& object, const unsigned char beam_group, const StemInfo& info)
{
    if (voice.notes.empty()) return;
    apply(object, --voice.notes.end(), beam_group, info);
}

void BeamGroupInfo::apply(const Chord& object, std::list<Plate::pNote>::iterator pnote, const unsigned char beam_group, const StemInfo& info)
{
    const bool has_beam = object.beam == Chord::FORCE_BEAM
                       || object.beam == Chord::CUT_BEAM
                       || (   object.beam == Chord::AUTO_BEAM
                           &&    voice.end_time.i() / (1 << beam_group)
                              == (voice.end_time - object.value()).i() / (1 << beam_group)
                          );
    
    const unsigned int exp = (   object.beam == Chord::CUT_BEAM
                              && object.val.exp < VALUE_BASE - 3) ?
                                    (VALUE_BASE - 3) :
                                    object.val.exp;
    
    last_pnote = pnote;
    
    if (has_beam && exp < VALUE_BASE - 2)   // if the note has got a beam
    {
        last_chord = &object;
        pnote->stem_info = Plate::pNote::StemInfoPtr(new StemInfo(info));
        if (beam[exp] == voice.notes.end())     // and there are not sufficient beams attached already
            start(exp);                         //   add additional beams
        else if (exp != 0)      // if there are sufficient beams (and we could need less)
            set(exp, pnote);    //   end unnecessary beams at the previous note
    }
    else if (beam[VALUE_BASE - 3] != voice.notes.end()) // if the note does not have a beam
    {                                                   // but the array is not empty yet
        last_chord = NULL;
        
        // remove flags
        for (std::list<Plate::pNote>::iterator i = beam[VALUE_BASE - 3]; i != voice.notes.end(); ++i)
        {
            i->noflag = true;
            if (i == pnote) break;
        };
        
        // end all beams
        if (exp >= VALUE_BASE - 2)  // if this note cannot have a beam
            return cut(pnote);      //   just cut and return
        
        else    // otherwise the beams end on this note
        {
            if (beam[exp] == voice.notes.end())     // if there are not sufficient beams attached already
                start(exp);                         //   add necessary beams
            set(exp, pnote);        // end unnecessary beams
            stop(exp, pnote);       // end existing beams
        };
    }
    else last_chord = NULL;
}

// end all beams
void BeamGroupInfo::finish()
{
    if (!last_chord) return;
    if (beam[VALUE_BASE - 3] == voice.notes.end()) return;
    if (beam[VALUE_BASE - 3] == last_pnote) return;
    
    // remove flags
    for (std::list<Plate::pNote>::iterator i = beam[VALUE_BASE - 3]; i != last_pnote; ++i)
        i->noflag = true;
    last_pnote->noflag = true;
    
    // end existing beams
    stop(last_chord->val.exp, last_pnote);
}


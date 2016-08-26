
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

// exception class
BeamInfo::Error::Error(const std::string& msg) : ScorePress::Error(msg) {}

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
void BeamInfo::start(unsigned int exp, Plate::pVoice::Iterator pnote)
{
    for (size_t i = exp; i < VALUE_BASE - 2; ++i)   // add new beams
    {
        if (beam[i] != voice->notes.end()) break;   // stop, if further beams exist already
        beam[i] = pnote;                            // add beam to this note
    };
}

// helper for "set", "stop" and "cut"
void BeamInfo::calculate_beam(size_t i, Plate::pNote& pnote, const Plate::pNote& end, size_t& s, ShortDir short_dir)
{
    if (beam[i] != voice->notes.end())  // ...if it exists
    {                                   //   let the beam go up to the given end-note
        // set the beam
        if (beam[i]->stem.is_up() == end.stem.is_up())
            set(i, VALUE_BASE - 3 - i + s, *beam[i], end, pnote); 
        else
            set(i, (i < VALUE_BASE - 3) ? s++ : s, *beam[i], end, pnote);
        
        // short beam adjustment
        if (beam[i]->beam[i]->short_beam)
        {
            // delete short 8th beam (i.e. use flag instead)
            if (i == VALUE_BASE - 3)
            {
                free(beam[i]->beam[i]);
                beam[i]->noflag = false;
            } else
            
            // change short beam direction on inconvienient next note
            if (short_dir != RIGHT)
            {
                // check for inconvienient previous note (i.e. no beam or not in the same beam group)
                if (   beam[i]->beam_begin == voice->notes.end()
                    || beam[i]->beam_begin != beam[i]->beam[i]->end->beam_begin)
                {
                    free(beam[i]->beam[i]);
                    beam[i]->noflag = false;
                }
                else
                {
                    // get previous chord
                    Plate::pVoice::Iterator short_end(beam[i]);
                    do --short_end; while (short_end != voice->notes.begin() && !short_end->note->is(Class::CHORD));
                    if (!short_end->note->is(Class::CHORD))               // should never occur (beam has to begin at a chord)
                        throw Error("Beam does not start at a chord.");
                    
                    // let the beam face left (if the previous note did not cut the beam)
                    if (static_cast<const Chord&>(*short_end->note).beam != Chord::BEAM_CUT || short_dir == FORCE_LEFT)
                    {
                        beam[i]->beam[i]->short_left = true;
                        if (beam[i] != beam[VALUE_BASE - 3])
                            beam[i]->beam[i]->end = &*short_end;
                    };
                    
                    if (s) --s;
                };
            };
        };
        beam[i] = voice->notes.end();   //   and remove it from the array
    };
}

// end all (for the given exponent) unnecessary beams at the previous note
void BeamInfo::set(unsigned int exp, Plate::pVoice::Iterator pnote, ShortDir short_dir)
{
    if (pnote == voice->notes.begin()) return;
    
    // get beam's end note
    Plate::pVoice::Iterator end(pnote);
    do --end; while (end != voice->notes.begin() && !end->note->is(Class::CHORD));
    if (!end->note->is(Class::CHORD))               // should never occur (beam has to begin at a chord)
        throw Error("Beam does not start at a chord.");
    
    // end unnecessary beams
    size_t s = 0;                           // beam shift (due to unequal stem directions)
    for (size_t i = 0; i < exp; ++i)        // check every beam we don't need...
    {
        calculate_beam(i, *pnote, *end, s, (beam[i + 1] == beam[i]) ? RIGHT : short_dir);
    };
    
    // count irregular beam positions (for stem length)
    for (size_t i = exp; i < VALUE_BASE - 3; ++i)
    {
        if (beam[i] != voice->notes.end())  // if the beam exists...
        {                                   // ...increment counter on unequal stem direction
            if (beam[i]->stem.is_up() != end->stem.is_up())
                ++s;                        
        };
    };
    end->stem.beam_off = s;
}

// end all existing beams at this note
void BeamInfo::stop(unsigned int exp, Plate::pVoice::Iterator end)
{
    size_t s = 0;   // beam shift (due to unequal stem directions)
    for (size_t i = exp; i < VALUE_BASE - 2; ++i)   // check the existing beams
    {
        calculate_beam(i, *end, *end, s, FORCE_LEFT);
    };
    end->stem.beam_off = s;
}

// let each beam end on the previous note (indicating that the current symbol does not allow beams)
void BeamInfo::cut(Plate::pVoice::Iterator pnote)
{
    if (pnote == voice->notes.begin()) return;
    Plate::pVoice::Iterator end(pnote);             // get beam's end
    --end;
    
    size_t s = 0;   // beam shift (due to unequal stem directions)
    
    for (size_t i = 0; i < VALUE_BASE - 2; ++i)     // check all beams
    {
        calculate_beam(i, *pnote, *end, s, FORCE_LEFT);
    };
    end->stem.beam_off = s;
}

// create beam information
void BeamInfo::apply(const Chord&             chord,      // current chord
                     Plate::pVoice::Iterator& pnote,      // corresponding on-plate note
                     const StemInfo*          stem_info,  // stem-info for current note (saved to plate or erased from plate)
                     const bool               has_beam,   // does the note have a beam?
                     const unsigned int       exp,        // effective value exponent
                     const unsigned long      time)       // time stamp
{
    // short beam direction
    const ShortDir left = // FORCED LEFT, if last note had the beam cut
                          (last_chord && last_chord->beam == Chord::BEAM_CUT) ? FORCE_LEFT : 
                          
                          // LEFT, at the beginning of the corresponding beat
                          (time % (2u << exp) != 0  ? LEFT : RIGHT);
    
    // add stem-info, if present (i.e. during the first pass; used by "EngraverState::engrave_stems")
    if (stem_info)
        pnote->stem_info = Plate::pNote::StemInfoPtr(new StemInfo(*stem_info));
    
    // delete stem-info, if not present (i.e. during the second pass; see "apply2")
    else
        free(pnote->stem_info);
    
    // calculate beam
    if (has_beam && exp < VALUE_BASE - 2)   // if the note has got a beam
    {
        // if the previous note cut all beams
        if (last_chord && last_chord->beam == Chord::BEAM_CUT)
            set(VALUE_BASE - 3, pnote, FORCE_LEFT); // end all beams but one on the previous
        
        // calculate beams
        if (beam[exp] == voice->notes.end())    // if there are not sufficient beams attached already
            start(exp, pnote);                  //   add additional beams
        else if (exp != 0)                      // if there are sufficient beams (and we could need less)
            set(exp, pnote, left);              //   end unnecessary beams at the previous note
        
        // buffer
        last_chord = &chord;                    // remember last chord
        last_pnote = pnote;                     // and last pnote
    }
    else if (beam[VALUE_BASE - 3] != voice->notes.end())    // if the note does not have a beam
    {                                                       // but the array is not empty yet
        // erase remembered chord
        last_chord = NULL;
        
        // remove flags
        for (Plate::pVoice::Iterator i = beam[VALUE_BASE - 3]; i != voice->notes.end(); ++i)
        {
            i->noflag = true;
            if (i == pnote) break;
        };
        
        // end all beams
        if (exp >= VALUE_BASE - 2)  // if this note cannot have a beam
        {
            cut(pnote);                 //   just cut
        }
        else    // otherwise the beams end on this note
        {
            if (beam[exp] == voice->notes.end())    // if there are not sufficient beams attached already
                start(exp, pnote);                  //   add necessary beams
            set(exp, pnote, left);                  // end unnecessary beamsq
            stop(exp, pnote);                       // end existing beams
        };
    }
    else last_chord = NULL;     // erase remembered chord
}

// constructor
BeamInfo::BeamInfo(Plate::pVoice& _voice) : voice(&_voice), last_pnote(voice->notes.end()), last_chord(NULL)
{
    for (size_t i = 0; i < VALUE_BASE - 2; ++i) beam[i] = voice->notes.end();
}

// create beam information (first pass; only top beam)
// (first version expecting the "object" to correspond to the last note in the "voice")
void BeamInfo::apply1(const Chord& object, const unsigned char beam_group, const StemInfo& info)
{
    if (voice->notes.empty()) return;
    apply(object, --voice->notes.end(), &info,
    // has_beam =
              object.beam == Chord::BEAM_FORCED             // forced beam
          ||  object.beam == Chord::BEAM_CUT                // cut beam
          || (object.beam == Chord::BEAM_AUTO               // automatic beam
              &&    voice->end_time.i() / (1 << beam_group) //   if, this and the next note are in the same group
                 == (voice->end_time - object.value()).i() / (1 << beam_group)
             ),
    // exp =
          (   object.beam == Chord::BEAM_CUT        // if the beam is cut
           && object.val.exp < VALUE_BASE - 3) ?    //  (and there even is a beam)
                 (VALUE_BASE - 3) :                 // do, as if this was an 8th note
                 object.val.exp,                    // otherwise pass the correct value
    // time =
         (voice->end_time - object.value()).i()
         );
}

// calculate beam information (second pass; all beams)
void BeamInfo::apply2(const Chord& object, Plate::pVoice::Iterator pnote, const value_t time, const unsigned char beam_group)
{
    apply(object, pnote, NULL,
    // has_beam =
              object.beam == Chord::BEAM_FORCED     // forced beam
          ||  object.beam == Chord::BEAM_CUT        // cut beam
          || (object.beam == Chord::BEAM_AUTO       // automatic beam
              &&    time.i() / (1 << beam_group)    //   if, this and the next note are in the same group
                 == (time - object.value()).i() / (1 << beam_group)
             ),
    // exp =
          object.val.exp,
          time.i());
}

// end all beams
void BeamInfo::finish()
{
    if (!last_chord) return;
    if (beam[VALUE_BASE - 3] == voice->notes.end()) return;
    if (beam[VALUE_BASE - 3] == last_pnote) return;
    
    // remove flags
    for (Plate::pVoice::Iterator i = beam[VALUE_BASE - 3]; i != last_pnote; ++i)
        i->noflag = true;
    last_pnote->noflag = true;
    
    // end existing beams
    stop(last_chord->val.exp, last_pnote);
}


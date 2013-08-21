
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

#ifndef SCOREPRESS_ENGRAVEINFO_HH
#define SCOREPRESS_ENGRAVEINFO_HH

#include <map>          // std::map

#include "stem_info.hh" // StemInfo
#include "score.hh"     // Score, Voice, TiedHead, Durable, ScoreDimension, tone_t, value_t, mpx_t
#include "plate.hh"     // Plate
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class   SCOREPRESS_LOCAL BeamInfo;       // beam-information structure
class   SCOREPRESS_LOCAL BeamGroupInfo;  // beam-group-information structure (prepare beam engraving)
struct  SCOREPRESS_LOCAL TieInfo;        // tie-information structure
struct  SCOREPRESS_LOCAL DurableInfo;    // durable-information structure
struct  SCOREPRESS_LOCAL SpaceInfo;      // space-info structure
struct  SCOREPRESS_LOCAL LineInfo;       // line-info structure
typedef std::map<Plate::pVoice*, BeamGroupInfo> BeamInfoMap;    // beam-information for every voice
typedef std::map<tone_t, TieInfo>               TieInfoChord;   // tie-information for every tone
typedef std::map<const Voice*, TieInfoChord>    TieInfoMap;     // tie-information for every voice


//
//     class BeamInfo
//    ================
//
// Information about the beams within the currently
// engraved group of notes. This is used for the real beam
// engraving during the line-postprocessing.
//
class SCOREPRESS_LOCAL BeamInfo
{
 private:
    Plate::pVoice& voice;                                   // the host voice
    std::list<Plate::pNote>::iterator beam[VALUE_BASE - 2]; // the first note for every beam
                                                            //   (highest index being the top beam)
    std::list<Plate::pNote>::iterator last_pnote;           // the last processed note (for the finish algorithm)
    const Chord*                      last_chord;           // the last processed chord instance (for the finish)
    
    void set(size_t beam_idx, size_t end_idx, Plate::pNote& start, const Plate::pNote& end, const Plate::pNote& cur);
    
    void start(unsigned int exp, std::list<Plate::pNote>::iterator pnote);  // start necessary beams
    void set(unsigned int exp, std::list<Plate::pNote>::iterator pnote);    // end unnecessary beams
    void stop(unsigned int exp, std::list<Plate::pNote>::iterator end);     // end all beams at this note
    
 public:
    void cut(std::list<Plate::pNote>::iterator pnote);                      // end each beam on the previous note
    
 public:
    // constructor
    BeamInfo(Plate::pVoice& voice);
    
    // calculate beam information
    // (first version expecting the "object" to correspond to the last note in the "voice")
    void apply(const Chord& object, const unsigned char beam_group);
    void apply(const Chord& object, std::list<Plate::pNote>::iterator pnote, const value_t end_time, const unsigned char beam_group);
    
    // end all beams
    void finish();
};


//
//   class BeamGroupInfo
//  =====================
//
// Inforation about beam groups (i.e. only top beam).
// This is used during the engraving to prepare the real
// engraving of the beams during line-postprocessing.
//
class SCOREPRESS_LOCAL BeamGroupInfo
{
 private:
    Plate::pVoice& voice;                           // the host voice
    std::list<Plate::pNote>::iterator beam[VALUE_BASE - 2];         // the first note of the top beam
    std::list<Plate::pNote>::iterator last_pnote;   // the last processed note (for the finish algorithm)
    const Chord*                      last_chord;   // the last processed chord instance (for the finish)
    
    void set(size_t beam_idx, size_t end_idx, Plate::pNote& start, const Plate::pNote& end, const Plate::pNote& cur);
    
    void start(unsigned int exp);                                           // start necessary beams
    void set(unsigned int exp, std::list<Plate::pNote>::iterator pnote);    // end unnecessary beams
    void stop(unsigned int exp, std::list<Plate::pNote>::iterator end);     // end all existing beams at this note
    
 public:
    void cut(std::list<Plate::pNote>::iterator pnote);                      // end each beam on the previous note
    
 public:
    // constructor
    BeamGroupInfo(Plate::pVoice& voice);
    
    // calculate beam information
    // (first version expecting the "object" to correspond to the last note in the "voice")
    void apply(const Chord& object, const unsigned char beam_group, const StemInfo& info);
    void apply(const Chord& object, std::list<Plate::pNote>::iterator pnote, const unsigned char beam_group, const StemInfo& info);
    
    // end all beams
    void finish();
};


//
//     struct TieInfo
//    ================
//
// This structure carries information for incomplete ties, necessary
// for computing the missing positions.
//
struct SCOREPRESS_LOCAL TieInfo
{
    const TiedHead* source;    // reference to the head, where the tie began
    Plate::pNote::Tie* target; // corresponding on-plate tie information (NULL, if non-head anchor)
    mpx_t refPos;              // horizontal position of the non-head anchor (if present)
    
    // default constructor
    TieInfo() : source(NULL), target(NULL), refPos(0) {};
};


//
//     struct DurableInfo
//    ====================
//
// This structure contains information about a durable object, used
// for the calculation of the position of the end-node.
//
struct SCOREPRESS_LOCAL DurableInfo
{
    const Durable* source;     // the durable object
    Plate::pDurable* target;   // the respective on-plate object
    size_t durationcountdown;  // number of staff-objects remaining up to the end-node
    
    // default constructor
    DurableInfo() : source(NULL), target(NULL), durationcountdown(0) {};
};


//     struct SpaceInfo
//    ==================
//
// Structure carrying information for the engraver to decide, if accidental-
// or cluster-space has to be added.
//
struct SCOREPRESS_LOCAL SpaceInfo
{
    value_t accidental_time;       // time-stamp of the last accidental, which required spacing
    const void* leftcluster_host;  // host object of the last left-cluster, which required spacing
    value_t rightcluster_time;     // time-stamp of the last right-cluster, which required spacing
    
    SpaceInfo() : accidental_time(-1), leftcluster_host(NULL), rightcluster_time(-1) {};
};


//
//     struct LineInfo
//    =================
//
// Style information of the line currently being engraved.
// This is used to store the lines style information, which will be applied
// AFTER the line is engraved (e.g. justification), because as soon as the
// newline is recognized, the pick contains the data for the NEXT line, not
// for the one recently engraved.
// See also: "Newline" (in "classes.hh")
//
struct SCOREPRESS_LOCAL LineInfo
{
    const ScoreDimension* dimension;   // score dimension (valid for the line)
    mpx_t indent;                      // line indentation
    bool justify;                      // width justification for this line?
    bool forced_justification;         // use forced justification (do not preserve min-distance)?
    mpx_t right_margin;                // (only for justified lines)
    
    LineInfo() : dimension(NULL), indent(0), justify(false), forced_justification(false), right_margin(0) {};
};

} // end namespace

#endif


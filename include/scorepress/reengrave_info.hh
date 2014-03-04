
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

#ifndef SCOREPRESS_REENGRAVEINFO_HH
#define SCOREPRESS_REENGRAVEINFO_HH

#include <map>          // std::multimap
#include <set>          // std::set
#include "classes.hh"   // StaffObject
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API   Reengraveable;   // interface for classes supporting being reengraved
class SCOREPRESS_LOCAL ReengraveInfo;   // information about on-plate references
class SCOREPRESS_LOCAL EngraverState;   // engraver state class prototype (see "engraver_state.hh")


//
//     class Reengraveable
//    =====================
//
// Abstract interface class for structures, that can be updated by an
// EngraverState instance during engraving.
//
class SCOREPRESS_API Reengraveable
{
 public:
    // called by "EngraverState" class, after "trigger" was engraved
    virtual void setup_reengrave(ReengraveInfo& info) = 0;
    virtual bool reengrave(EngraverState& state) = 0;
    virtual void finish_reengrave() = 0;
};


//
//     class ReengraveInfo
//    =====================
//
// This class is used, to associate on-plate references with their counterparts
// within the score object. This allows for easy updating of several cursors
// during the engraving process, directly by the engraver.
//
class SCOREPRESS_LOCAL ReengraveInfo
{
 private:
    // objects registered for update
    std::multimap<const StaffObject*, Reengraveable*> on_create_note;     // map of objects updated on note creation
    std::multimap<const Voice*,       Reengraveable*> on_create_voice;    // map of objects updated on voice creation
    std::multimap<const Movable*,     Reengraveable*> on_create_movable;  // map of objects updates on movable creation
    
    // updated objects
    std::set<Reengraveable*> on_finish;          // set of objects updated after the engraving
    
 public:
    // setup update
    void setup_reengrave(const StaffObject& trigger, Reengraveable& tgt);
    void setup_reengrave(const Voice&       trigger, Reengraveable& tgt);
    void setup_reengrave(const Movable&     trigger, Reengraveable& tgt);
    
    // execute update
    void update(const StaffObject& note,   EngraverState& state);
    void update(const Voice&       voice,  EngraverState& state);
    void update(const Movable&     object, EngraverState& state);
    
    // execute finish on all updated objects
    void finish();
    
    // information access
    size_t size() const;        // number of registered objects
    bool is_empty() const;      // check, if no objects are registered (should be "true" after a successful reengrave)
    bool needs_finish() const;  // check, whether there are objects to call "finish" on
};

// inline method implementations
inline void ReengraveInfo::setup_reengrave(const StaffObject& trigger, Reengraveable& target) {
    on_create_note.insert(std::pair<const StaffObject*, Reengraveable*>(&trigger, &target));}
inline void ReengraveInfo::setup_reengrave(const Voice& trigger, Reengraveable& target) {
    on_create_voice.insert(std::pair<const Voice*, Reengraveable*>(&trigger, &target));}
inline void ReengraveInfo::setup_reengrave(const Movable& trigger, Reengraveable& target) {
    on_create_movable.insert(std::pair<const Movable*, Reengraveable*>(&trigger, &target));}

inline size_t ReengraveInfo::size()         const {return on_create_note.size() + on_create_voice.size() + on_create_movable.size();}
inline bool   ReengraveInfo::is_empty()     const {return on_create_note.empty() && on_create_voice.empty() && on_create_movable.empty();}
inline bool   ReengraveInfo::needs_finish() const {return !on_finish.empty();}

} // end namespace

#endif


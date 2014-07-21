
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

#include "reengrave_info.hh"
using namespace ScorePress;

// execute update on note engraving
void ReengraveInfo::update(const StaffObject& note, EngraverState& state)
{
    // find first reengraveable object associated with the given note
    std::multimap<const StaffObject*, Reengraveable*>::iterator it = on_create_note.find(&note);
    
    // iterate through all objects, corresponding with this note
    while (it != on_create_note.end() && it->first == &note)
    {
        switch (it->second->reengrave(state))       // execute update
        {
            case Reengraveable::RETRY:  ++it; break;                    // do nothing, retry one more time
            case Reengraveable::FINISH: on_finish.insert(it->second);   // if requested, mark for finish
            case Reengraveable::DONE:   on_create_note.erase(it++);     // if done, remove from the map
        };
    };
}

// execute update on voice engraving
void ReengraveInfo::update(const Voice& voice, EngraverState& state)
{
    // find first reengraveable object associated with the given note
    std::multimap<const Voice*, Reengraveable*>::iterator it = on_create_voice.find(&voice);
    
    // iterate through all objects, corresponding with this note
    while (it != on_create_voice.end() && it->first == &voice)
    {
        switch (it->second->reengrave(state))       // execute update
        {
            case Reengraveable::RETRY:  ++it; break;                    // do nothing, retry one more time
            case Reengraveable::FINISH: on_finish.insert(it->second);   // if requested, mark for finish
            case Reengraveable::DONE:   on_create_voice.erase(it++);    // if done, remove from the map
        };
    };
}

// execute update on movable engraving
void ReengraveInfo::update(const Movable& object, EngraverState& state)
{
    // find first reengraveable object associated with the given note
    std::multimap<const Movable*, Reengraveable*>::iterator it = on_create_movable.find(&object);
    
    // iterate through all objects, corresponding with this note
    while (it != on_create_movable.end() && it->first == &object)
    {
        switch (it->second->reengrave(state))       // execute update
        {
            case Reengraveable::RETRY:  ++it; break;                    // do nothing, retry one more time
            case Reengraveable::FINISH: on_finish.insert(it->second);   // if requested, mark for finish
            case Reengraveable::DONE:   on_create_movable.erase(it++);  // if done, remove from the map
        };
    };
}

// execute finish on all updated objects
void ReengraveInfo::finish()
{
    for (std::set<Reengraveable*>::iterator it = on_finish.begin(); it != on_finish.end(); ++it)
        (*it)->finish_reengrave();
    on_finish.clear();
}



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

#ifndef SCOREPRESS_SPRITEID_HH
#define SCOREPRESS_SPRITEID_HH

#include <cstddef>  // size_t
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API SpriteId;     // Sprite Identification Structure


// Sprite Identification Structure
class SCOREPRESS_API SpriteId
{
 public:
    size_t setid;       // id of the set containing the sprite
    size_t spriteid;    // id of the sprite within the set
    
    SpriteId();
    SpriteId(const size_t setid, const size_t spriteid);
    void set(const size_t setid, const size_t spriteid);
    bool ready() const; // ("setid" and "spriteid" not undefined)
};

} // end namespace

#endif



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

#include "sprite_id.hh"
#include "undefined.hh" // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                        // this number is interpreted as an undefined value
using namespace ScorePress;

SpriteId::SpriteId() : setid(UNDEFINED), spriteid(UNDEFINED) {}
SpriteId::SpriteId(const size_t _setid, const size_t _spriteid) : setid(_setid), spriteid(_spriteid) {}
void SpriteId::set(const size_t _setid, const size_t _spriteid) {setid = _setid; spriteid = _spriteid;}
bool SpriteId::ready() const {return setid != UNDEFINED && spriteid != UNDEFINED;}


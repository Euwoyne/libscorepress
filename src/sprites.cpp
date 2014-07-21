
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2014 Dominik Lehmann
  
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

#include "sprites.hh"   // SpriteInfo, SpriteSet, Sprites
#include "undefined.hh" // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                        // this number is interpreted as an undefined value
using namespace ScorePress;


//
//     class SpriteInfo
//    ==================
//
// Meta-information structure for one sprite graphic.
// This contains the dimension as well as all provided attributes for a single
// graphic.
//

// constructor
SpriteInfo::SpriteInfo(Type _type) : type(_type), width(0), height(0) {}

// easy access methods (with default returns for non-existing values)
const std::string& SpriteInfo::get_text(const std::string& key) const
{
    static const std::string nulltext;
    const std::map<std::string, std::string>::const_iterator i = text.find(key);
    return (i == text.end()) ? nulltext : i->second;
}

double SpriteInfo::get_real(const std::string& key) const
{
    const std::map<std::string, double>::const_iterator i = real.find(key);
    return (i == real.end()) ? 0.0 : i->second;
}

int SpriteInfo::get_integer(const std::string& key) const
{
    const std::map<std::string, int>::const_iterator i = integer.find(key);
    return (i == integer.end()) ? 0 : i->second;
}


//
//     class SpriteSet
//    =================
//
// This class is a set of sprites provided by one svg-file.
//

// default constructor (initializing ids with UNDEFINED)
SpriteSet::SpriteSet() :
            head_height(0),
            timesig_digit_space(3),
            
            heads_longa(UNDEFINED),
            heads_breve(UNDEFINED),
            heads_whole(UNDEFINED),
            heads_half(UNDEFINED),
            heads_quarter(UNDEFINED),
            
            rests_longa(UNDEFINED),
            rests_breve(UNDEFINED),
            rests_whole(UNDEFINED),
            rests_half(UNDEFINED),
            rests_quarter(UNDEFINED),
            
            flags_note(UNDEFINED),
            flags_overlay(UNDEFINED),
            flags_rest(UNDEFINED),
            flags_base(UNDEFINED),
            
            accidentals_double_flat(UNDEFINED),
            accidentals_flat_andahalf(UNDEFINED),
            accidentals_flat(UNDEFINED),
            accidentals_half_flat(UNDEFINED),
            accidentals_natural(UNDEFINED),
            accidentals_half_sharp(UNDEFINED),
            accidentals_sharp(UNDEFINED),
            accidentals_sharp_andahalf(UNDEFINED),
            accidentals_double_sharp(UNDEFINED),
            
            undefined_symbol(UNDEFINED)
{
    digits_time[0] = UNDEFINED;
    digits_time[1] = UNDEFINED;
    digits_time[2] = UNDEFINED;
    digits_time[3] = UNDEFINED;
    digits_time[4] = UNDEFINED;
    digits_time[5] = UNDEFINED;
    digits_time[6] = UNDEFINED;
    digits_time[7] = UNDEFINED;
    digits_time[8] = UNDEFINED;
    digits_time[9] = UNDEFINED;
}

// erase the vector and reset sprite-ids
void SpriteSet::clear()
{
    std::vector<SpriteInfo>::clear();
    file.clear();
    title.clear();
    info.clear();
    head_height = 0;
    timesig_digit_space = 0;
    ids.clear();
    
    heads_longa = UNDEFINED;
    heads_breve = UNDEFINED;
    heads_whole = UNDEFINED;
    heads_half = UNDEFINED;
    heads_quarter = UNDEFINED;
    
    rests_longa = UNDEFINED;
    rests_breve = UNDEFINED;
    rests_whole = UNDEFINED;
    rests_half = UNDEFINED;
    rests_quarter = UNDEFINED;
    
    flags_note = UNDEFINED;
    flags_overlay = UNDEFINED;
    flags_rest = UNDEFINED;
    flags_base = UNDEFINED;
    
    accidentals_double_flat = UNDEFINED;
    accidentals_flat_andahalf = UNDEFINED;
    accidentals_flat = UNDEFINED;
    accidentals_half_flat = UNDEFINED;
    accidentals_natural = UNDEFINED;
    accidentals_half_sharp = UNDEFINED;
    accidentals_sharp = UNDEFINED;
    accidentals_sharp_andahalf = UNDEFINED;
    accidentals_double_sharp = UNDEFINED;
    
    digits_time[0] = UNDEFINED;
    digits_time[1] = UNDEFINED;
    digits_time[2] = UNDEFINED;
    digits_time[3] = UNDEFINED;
    digits_time[4] = UNDEFINED;
    digits_time[5] = UNDEFINED;
    digits_time[6] = UNDEFINED;
    digits_time[7] = UNDEFINED;
    digits_time[8] = UNDEFINED;
    digits_time[9] = UNDEFINED;
    
    undefined_symbol = UNDEFINED;
}

// get index by sprite-id
size_t SpriteSet::index(const std::string& id) const
{
    const std::map<std::string, size_t>::const_iterator i(ids.find(id));
    return (i == ids.end()) ? UNDEFINED : i->second;
}



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

#ifndef SCOREPRESS_STEMINFO_HH
#define SCOREPRESS_STEMINFO_HH

#include "classes.hh"   // mpx_t
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
struct SCOREPRESS_LOCAL StemInfo;       // stem-information structure


//
//   struct StemInfo
//  =================
//
// Stem position information (saved on the plate) for
// postprocessing stem correction.
//
struct SCOREPRESS_LOCAL StemInfo
{
    mpx_t top_pos;              // y-position of the uppermost head
    mpx_t base_pos;             // y-position of the lowermost head
    unsigned int top_scale;     // scale of the uppermost head
    unsigned int base_scale;    // scale of the lowermost head
    bool cluster;               // set to true, if the chord contains close heads
    bool top_side;              // set to true, if the top-note got clustered
    bool base_side;             // set to true, if the base-note got clustered
    
    // default constructor
    StemInfo() : top_pos(0), base_pos(0), top_scale(1000), base_scale(1000), cluster(false), top_side(false), base_side(false) {}
};

} // end namespace

#endif


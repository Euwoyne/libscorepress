
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

#ifndef SCOREPRESS_PRESS_STATE_HH
#define SCOREPRESS_PRESS_STATE_HH

#include <string>           // std::string

#include "parameters.hh"    // PressParam, ViewportParam
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API PressState;    // state of the press (including parameters and offset data, etc.)


//
//     class PressState
//    ==================
//
// The State structure is the set of data passed to the object class on
// rendering the object by the press. It contains all necessary parameters
// and positioning information needed for successful rendering.
// (Not counting those provided by the on-plate object.)
//
class SCOREPRESS_API PressState
{
 public:
    const PressParam&    parameters;    // rendering parameters
    const StyleParam*    style;         // current style
    const ViewportParam& viewport;      // viewport parameters
    Position<mpx_t>      offset;        // offset to be applied
    umpx_t               head_height;   // current voice's head-height
    umpx_t               stem_width;    // current stem-width
    
    PressState(const PressParam&, const StyleParam&, const ViewportParam&);
    void   set_style(const StyleParam& new_style);
    double scale(const double coord) const;
};

inline void   PressState::set_style(const StyleParam& new_style) {style = &new_style;}
inline double PressState::scale(const double coord) const        {return (parameters.scale * coord) / 1000.0;}

} // end namespace

#endif


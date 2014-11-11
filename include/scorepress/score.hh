
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

#ifndef SCOREPRESS_SCORE_HH
#define SCOREPRESS_SCORE_HH

#include <list>             // std::list
#include <map>              // std::map
#include "classes.hh"       // [score classes]
#include "parameters.hh"    // StyleParam
#include "smartptr.hh"      // SmartPtr
#include "meta.hh"          // Meta
#include "error.hh"         // Error
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API Score;        // score object (one musical score on consequent pages)


// score object (one musical score on consequent pages)
class SCOREPRESS_API Score
{
 public:
    class StaffNotFound : public ScorePress::Error  // thrown by staff-finding methods, if the given staff does not exist
    {public: StaffNotFound();};
    
    struct Layout   // first page layout
    {
        // TODO: user defined brace scale parameters
        ScoreDimension dimension;   // layout information
        SpriteId brace_sprite;      // sprite for braces
        SpriteId bracket_sprite;    // sprite for brackets
        MovableList attached;       // objects attached to the page
    };
    
    typedef SmartPtr<EngraverParam> EngraverParamPtr;
    typedef SmartPtr<StyleParam>    StyleParamPtr;
    
 public:
    std::list<Staff> staves;    // the staves within the score
    Layout layout;              // layout information for the first page
                                // (consequent page information given with pagebreak-objects)
    unsigned int head_height;   // optional score-specific default head-height (when 0, inherit from document)
    StyleParamPtr style;        // optional style parameters
    EngraverParamPtr param;     // optional engraver parameters
    Meta meta;                  // meta information
    
 public:
    // default constructor
    Score() : head_height(0) {}
    
    // get an iterator for a given staff
    std::list<Staff>::const_iterator get_staff(const Staff& staff) const throw(StaffNotFound);
    
    // check, whether the given staves belong to one instrument or instrument-group respectively
    bool same_instrument(const Staff& staff1, const Staff& staff2) const throw(StaffNotFound);
    bool same_group(const Staff& staff1, const Staff& staff2) const throw(StaffNotFound);
};

} // end namespace

#endif


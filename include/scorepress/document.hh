
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

#ifndef SCOREPRESS_DOCUMENT_HH
#define SCOREPRESS_DOCUMENT_HH

#include <list>             // std::list
#include <map>              // std::map

#include "score.hh"         // Score, Movable, DocumentMeta
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API Document;     // document object (several musical scores and score-independent objects)


// document object (several musical scores and score-independent objects)
class SCOREPRESS_API Document
{
 public:
    // page dimension data
    class PageDimension
    {
     public:
        uum_t width;
        uum_t height;
        struct Margin
        {
            uum_t top;
            uum_t bottom;
            uum_t left;
            uum_t right;
        } margin;
        
        // A4: 210 x 297; NE: 231 x 303
        PageDimension() : width(210000), height(297000) {margin.top = margin.bottom = 15000; margin.left = margin.right = 10000;}
    };
    
    // score object within the document
    class Score
    {
     public:
        size_t            start_page;   // document-page number of the first score-page
        ScorePress::Score score;        // score object
        
        Score(size_t _page) : start_page(_page) {}
    };
    
    // list typedefs
    typedef std::map<size_t, MovableList> AttachedMap;
    typedef std::list<Score>              ScoreList;
    
 public:
    AttachedMap    attached;        // objects attached to the document
    PageDimension  page_layout;     // page layout
    ScoreList      scores;          // scores within the document
    DocumentMeta   meta;            // meta information
    StyleParam     style;           // default style parameters (may be overwritten by scores and staves)
    EngraverParam  param;           // default engraver parameters (may be overwritten by scores)
    
    // on-page object parameters
    uum_t head_height;              // default head-height
    uum_t stem_width;               // default stem-width
    
 public:
    // attached object interface
    void add_attached(Movable* object, size_t page);    // adds attachable (ownership transferred to this instance)
};

inline void Document::add_attached(Movable* object, size_t page) {
    attached[page].push_back(MovablePtr(object));}

} // end namespace

#endif


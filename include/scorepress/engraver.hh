
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

#ifndef SCOREPRESS_ENGRAVER_HH
#define SCOREPRESS_ENGRAVER_HH

#include "document.hh"       // Document, Score
#include "pageset.hh"        // Pageset
#include "sprites.hh"        // Sprites
#include "parameters.hh"     // EngraverParam, StyleParam, ViewportParam
#include "reengrave_info.hh" // ReengraveInfo
#include "log.hh"            // Logging
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_LOCAL Engraver;    // engraver, computing a renderable plate from an abstact score-object


//
//     class Engraver
//    ================
//
// This class is used to convert a score-object to a set of absolute-positioned
// sprites, being saved to a plate. The target plate as well as the sprite-
// library are given during the construction.
// Internally an instance of "Pick" (as part of the "EngraverState") is used to
// calculate the object positions.
//
class SCOREPRESS_LOCAL Engraver : public Logging
{
 private:
          Pageset*       pageset;           // target set of pages
    const Sprites*       sprites;           // pointer to the sprite-library (for the pick)
    const StyleParam*    style;             // default staff-style
    const ViewportParam* viewport;          // viewport-parameters (see "parameters.hh")
    
 private:
    void engrave_attachables(const Document& data);    // engrave the on-page attachables
    
 public:
    EngraverParam parameters;  // engraving-parameters (see "parameters.hh")
    
    // constructor
    Engraver(Pageset& pageset, const Sprites& sprites, const StyleParam& style, const ViewportParam& viewport);
    
    // set methods
    void set_pageset(Pageset& pageset);                 // set the pageset
    void set_sprites(const Sprites& sprites);           // set the sprite-library
    void set_style(const StyleParam& style);            // set the staff-style
    void set_viewport(const ViewportParam& viewport);   // set the viewport
    
    // get methods
    const Sprites&       get_sprites();                 // get the sprite-library
    const StyleParam&    get_style();                   // get the staff-style
    const ViewportParam& get_viewport();                // get the viewport
    
    // engrave the score
    void engrave(const Score& score, const unsigned int start_page, const unsigned int head_height);
    void engrave(const Score& score, const unsigned int start_page, const unsigned int head_height, ReengraveInfo& info);
    // (engraving deletes and recreates all affected plates!)
    
    // engrave the document
    void engrave(const Document& document);
    void engrave(const Document& document, ReengraveInfo& info);
    // (engraving deletes and recreates all affected plates!)
};

// set methods
inline void Engraver::set_pageset(Pageset& _pageset)               {pageset = &_pageset;}
inline void Engraver::set_sprites(const Sprites& _sprites)         {sprites = &_sprites;}
inline void Engraver::set_style(const StyleParam& _style)          {style = &_style;}
inline void Engraver::set_viewport(const ViewportParam& _viewport) {viewport = &_viewport;}

// get methods
inline const Sprites&       Engraver::get_sprites()  {return *sprites;}
inline const StyleParam&    Engraver::get_style()    {return *style;}
inline const ViewportParam& Engraver::get_viewport() {return *viewport;}

} // end namespace

#endif


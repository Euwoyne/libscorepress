
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

#ifndef SCOREPRESS_CURSORBASE_HH
#define SCOREPRESS_CURSORBASE_HH

#include "reengrave_info.hh"    // Reengraveable
#include "press_state.hh"       // PressState
#include "export.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API CursorBase;    // interface for cursor classes

// PROTOTYPES
class Renderer;     // renderer class prototype (see "renderer.hh")
class Document;     // document class prototype (see "document.hh")
class Score;        // score class prototype (see "score.hh")


//
//     class CursorBase
//    =====================
//
// Abstract interface class for cursors. Cursors should be reengraveable and
// they can be rendered.
//
class SCOREPRESS_API CursorBase : public Reengraveable
{
 public:
    // base data access
    virtual const Document& get_document() const = 0;           // return the document
    virtual const Score&    get_score()    const = 0;           // return the score-object
    virtual unsigned int    get_pageno()   const = 0;           // return the page-number
    virtual bool            has_score()    const = 0;           // is a score set?
    
    // rendering interface
    virtual void render(Renderer&, const PressState&) const = 0;
    
    // reengraving interface
    virtual void   setup_reengrave(ReengraveInfo& info) = 0;    // setup before reengraving takes place
    virtual Status reengrave(EngraverState& state) = 0;         // called by "EngraverState" class, after "trigger" was engraved
    virtual void   finish_reengrave() = 0;                      // executed after reengraving finished
    
    // virtual destructor
    virtual ~CursorBase() {}
};

} // end namespace
#pragma clang diagnostic pop

#endif


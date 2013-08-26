
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

#ifndef SCOREPRESS_PRESS_HH
#define SCOREPRESS_PRESS_HH

#include <string>           // std::string

#include "pageset.hh"       // PageSet, Plate, Score, List, Color
#include "renderer.hh"      // Renderer
#include "parameters.hh"    // PressParam, ViewportParam
#include "user_cursor.hh"   // UserCursor
#include "error.hh"         // Error
#include "log.hh"           // Logging
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_LOCAL Press;       // the press, renders the prepared plate object with a renderer
class SCOREPRESS_LOCAL PressState;  // state of the press (including parameters and offset data, etc.)


//
//     class PressState
//    ==================
//
// The State structure is the set of data passed to the object class on
// rendering the object by the press. It contains all necessary parameters
// and positioning information needed for successful rendering.
// (Not counting those provided by the on-plate object.)
//
class PressState
{
 public:
    const PressParam&    parameters;    // rendering parameters
    const StyleParam*    style;         // current style
    const ViewportParam& viewport;      // viewport parameters
    Position<mpx_t>      offset;        // offset to be applied
    mpx_t                head_height;   // current voice's head-height
    mpx_t                stem_width;    // current stem-width
    
    PressState(const PressParam& parameters, const StyleParam& style, const ViewportParam& viewport);
    inline void set_style(const StyleParam& new_style) {style = &new_style;};
};


//
//     class Press
//    =============
//
// The press-class exports a method, which draws a score, with the help of the
// engraver-provided Plate instance and a renderer instance.
//
class SCOREPRESS_LOCAL Press : public Logging
{
 public:
    // exception classes
    class SCOREPRESS_API Error : public ScorePress::Error {public: Error(const std::string& msg);};
    class SCOREPRESS_API InvalidRendererException : public Error   // thrown, the given renderer is not ready
        {public: InvalidRendererException();};
    
 private:
    // parameters
    PressState        state;            // current state
    const StyleParam& default_style;    // default style
    
    // scales the given coordinates
    inline double scale(const double coord) const {return (parameters.scale * coord) / 1000.0;};
    
    // set the renderer's foreground color
    inline static void set_color(Renderer& renderer, const Color& color) {renderer.set_color(color.r, color.g, color.b, color.a);};
    
    // rendering method (for on-plate note objects)
    void render(Renderer& renderer, const Plate::pNote& note);
    
    // render the (empty) staff for a plate
    void render_staff(Renderer& renderer, const Plate& plate, const Position<mpx_t> offset);
    
    // draw a little red cross
    void draw_cross(Renderer& renderer, const Position<mpx_t>& pos, const Position<mpx_t> offset);
    
 public:
    // rendering parameters
    PressParam parameters;
    
    // constructor (providing viewport parameters)
    Press(const StyleParam& style, const ViewportParam& viewport);
    
    // draw the boundary box of a graphical object
    void draw_boundaries(Renderer& renderer, const Plate::pGraphical& object, unsigned int color, const Position<mpx_t> offset) throw(InvalidRendererException);
    void draw_boundaries(Renderer& renderer, const Plate::pGraphical& object, const Color& color, const Position<mpx_t> offset) throw(InvalidRendererException);
    
    // render a plate through the given renderer
    void render(Renderer& renderer, const Plate& plate, const Position<mpx_t> offset) throw(InvalidRendererException);
    
    // render a page through the given renderer
    void render(Renderer& renderer, const PageSet::pPage& page, const PageSet& pageset, const Position<mpx_t> offset) throw(InvalidRendererException);
    
    // render page decoration
    void render_decor(Renderer& renderer, const PageSet& pageset, const Position<mpx_t> offset) throw(InvalidRendererException);
    
    // render a cursor through the given renderer
    void render(Renderer& renderer, const UserCursor& cursor, const Position<mpx_t> offset) throw (InvalidRendererException, UserCursor::NotValidException);
};

} // end namespace

#endif


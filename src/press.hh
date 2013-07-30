
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

namespace ScorePress
{
//  CLASSES
// ---------
class Press;    // the press, renders the prepared plate object with a renderer


//
//     class Press
//    =============
//
// The press-class exports a method, which draws a score, with the help of the
// engraver-provided Plate instance and a renderer instance.
//
class Press
{
 public:
    // exception classes
    class Error : public ScorePress::Error {public: Error(const std::string& msg) : ScorePress::Error(msg) {};};
    class InvalidRendererException : public Error   // thrown, the given renderer is not ready
    {public: InvalidRendererException() : Error("Unable to draw with non-ready renderer.") {};};
    
 private:
    // parameters
    const ViewportParam* viewport;      // viewport parameters
    const StyleParam* style;            // current style
    const StyleParam& default_style;    // default style
    
    // scales the given coordinates
    inline double scale(const double coord) const {return (parameters.scale * coord) / 1000.0;};
    
    // set the renderer's foreground color
    inline static void set_color(Renderer& renderer, const Color& color) {renderer.set_color(color.r, color.g, color.b, color.a);};
    
    // render a sprite transforming the position with "param.scale"
    void draw(Renderer& renderer, const SpriteId& sprite, const Position<mpx_t> pos, const Position<mpx_t> offset, const double sprite_scale);
    
    // rendering method (for attachable objects)
    void render(Renderer& renderer, const Plate::pAttachable* object, const Position<mpx_t> offset, const bool stemup, const mpx_t head_height, const mpx_t stem_width);
    
    // rendering method (for on-plate note objects)
    void render(Renderer& renderer, const Plate::pNote& note, const Position<mpx_t> offset, const mpx_t head_height, const mpx_t stem_width);
    
    // beam renderer
    void render_beam(Renderer& renderer, const Plate::pNote& note, const Chord& chord, const Position<mpx_t> offset, const mpx_t head_height, const mpx_t stem_width);
    
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
    void draw_boundaries(Renderer& renderer, const Plate::pGraphical& object, const Position<mpx_t> offset) throw(InvalidRendererException);
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


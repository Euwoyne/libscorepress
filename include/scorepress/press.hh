
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

#ifndef SCOREPRESS_PRESS_HH
#define SCOREPRESS_PRESS_HH

#include <string>           // std::string

#include "press_state.hh"   // PressState
#include "pageset.hh"       // Pageset, Plate, Score, List, Color
#include "renderer.hh"      // Renderer
#include "parameters.hh"    // PressParam, ViewportParam
#include "user_cursor.hh"   // UserCursor
#include "object_cursor.hh" // ObjectCursor
#include "error.hh"         // Error
#include "log.hh"           // Logging
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_LOCAL Press;       // the press, renders the prepared plate object with a renderer


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
    const StyleParam* default_style;    // default style
    
    // scales the given coordinates
    double scale(const double coord) const;
    
    // set the renderer's foreground color
    static void set_color(Renderer&, const Color&);
    
    // rendering method (for on-plate note objects)
    void render(Renderer&, const Plate::pNote&);
    
    // render the (empty) staff for a plate
    void render_staff(Renderer&, const Plate&, const Position<mpx_t> offset);
    
    // draw a little red cross
    void draw_cross(Renderer&, const Position<mpx_t>& pos, const Position<mpx_t> offset);
    
 public:
    // rendering parameters
    PressParam parameters;
    
    // constructor (providing viewport parameters)
    Press(const StyleParam& style, const ViewportParam& viewport);
    
    void              set_style(const StyleParam& style);
    const StyleParam& get_style();
    
    // draw the boundary box of a graphical object
    void draw_boundaries(Renderer&, const Plate::GphBox&,     unsigned int color, const Position<mpx_t> offset);
    void draw_boundaries(Renderer&, const Plate::GphBox&,     const Color& color, const Position<mpx_t> offset);
    void draw_boundaries(Renderer&, const Plate::pGraphical&, unsigned int color, const Position<mpx_t> offset);
    void draw_boundaries(Renderer&, const Plate::pGraphical&, const Color& color, const Position<mpx_t> offset);
    
    // render a plate/page/attachable through the given renderer
    void render(Renderer&, const Plate&,                              const Position<mpx_t> offset);
    void render(Renderer&, const Pageset::pPage&,     const Pageset&, const Position<mpx_t> offset);
    void render(Renderer&, const Plate::pAttachable&, const Staff&,   const Position<mpx_t> offset);
    
    // render page decoration
    void render_decor(Renderer&, const Pageset&, const Position<mpx_t> offset);
    
    // render a cursor through the given renderer
    void render(Renderer&, const CursorBase&, const Position<mpx_t> offset);
};

inline double Press::scale(const double coord) const               {return (parameters.scale * coord) / 1000.0;}
inline void   Press::set_color(Renderer& renderer, const Color& c) {renderer.set_color(c.r, c.g, c.b, c.a);}

inline void              Press::set_style(const StyleParam& style) {default_style = &style;}
inline const StyleParam& Press::get_style()                        {return *default_style;}

inline void Press::draw_boundaries(Renderer& renderer, const Plate::pGraphical& object, unsigned int color, const Position<mpx_t> offset) {
    draw_boundaries(renderer, object.gphBox, color, offset);}

inline void Press::draw_boundaries(Renderer& renderer, const Plate::pGraphical& object, const Color& color, const Position<mpx_t> offset) {
    draw_boundaries(renderer, object.gphBox, color, offset);}

} // end namespace

#endif


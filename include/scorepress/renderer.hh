
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

#ifndef SCOREPRESS_RENDERER_HH
#define SCOREPRESS_RENDERER_HH

#include <string>       // std::string
#include <map>          // std::map

#include "sprites.hh"   // Sprites, cSpriteSet, SpriteId
#include "error.hh"     // Score::Error

namespace ScorePress
{
//
//  CLASSES
// ---------
class Renderer; // abstract vector-graphics and svg-sprites-renderer interface


//
//     class Renderer
//    ================
//
// This is a (partially) abstract renderer interface.
// It is used by the engine to render the prepared score, such that the engine
// is independent of the used frontend. Furthermore this class implements a
// parser for the sprite-file's meta-information, preparing a "Sprites"
// object.
//
class Renderer
{
 public:
    // error class thrown on syntax errors within the sprites meta information
    class Error : public ScorePress::Error {public: Error(const std::string& msg) : ScorePress::Error(msg) {};};
    
    // text alignment enumeration
    enum enuAlignment {ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER};
    
 protected:
    // throwing functions (combining the given data to a error message, which is then thrown)
    void mythrow(const char* trns, const std::string& filename) throw(Error);
    void mythrow(const char* trns, const std::string& symbol, const std::string& filename, const int line, const int column) throw(Error);
    void mythrow(const char* trns, const std::string& filename, const int line, const int column) throw(Error);
    
    Sprites sprites;    // sprites collection
    
    // parser for the svg's meta-information (filename only used for error msgs)
    void parse(const char* spriteinfo, const std::string& filename, const size_t setid) throw(Error);
    
 public:
    inline void erase_sprites() {sprites.clear();};     // erase the sprites collection
    inline Sprites& get_sprites() {return sprites;};    // return the sprites collection
    void dump() const;                                  // dump sprite info to stdout
    virtual ~Renderer() {};                             // virtual destructor
    
 protected:
    // path existance method
    virtual bool exist(const std::string& path, const size_t setid) = 0;
    
 public:
    // renderer methods (to be implemented by actual renderer)
    virtual bool ready() const = 0;         // is the object ready to render?
    
    // sprite rendering
    virtual void draw_sprite(const ScorePress::SpriteId sprite_id, double x, double y) = 0;
    virtual void draw_sprite(const ScorePress::SpriteId sprite_id, double x, double y, double xscale, double yscale) = 0;
    
    // basic rendering
    virtual void set_line_width(const double width) = 0;    // set width of following lines
    virtual void set_color(const unsigned char r,           // set the current foreground color
                           const unsigned char g,
                           const unsigned char b,
                           const unsigned char a) = 0;
    virtual void move_to(const double x, const double y) = 0;   // move cursor
    virtual void line_to(const double x, const double y) = 0;   // draw line
    virtual void fill() = 0;                    // fill the drawn object
    virtual void stroke() = 0;                  // render the drawn object
    virtual void close() = 0;                   // close the drawn object
    
    // text rendering
    virtual void set_font_family(const std::string& family) = 0; // set the font family
    virtual void set_font_size(const double pt) = 0;             // set the font size (in pt)
    virtual void set_font_bold(const bool bold) = 0;             // set, if the font is bold
    virtual void set_font_italic(const bool italic) = 0;         // set, if the font is italic
    virtual void set_font_underline(const bool underline) = 0;   // set, if the font is underlined
    virtual void set_font_color(const unsigned char r,           // set the font color
                                const unsigned char g,
                                const unsigned char b) = 0;
    
    virtual void set_text_width(const double width) = 0;       // set the textbox width
    virtual void reset_text_width() = 0;                       // reset the textbox to the whole drawing area
    virtual void set_text_align(const enuAlignment align) = 0; // set the alignment
    virtual void set_text_justify(const bool justify) = 0;     // enable/disable justification
    virtual void add_text(const std::string& utf8) = 0;        // add a string to the paragraph
    virtual void render_text() = 0;                            // render the prepared text
    
    // advanced rendering
    virtual void rect_invert(double x1, double y1,
                             double x2, double y2) = 0;         // invert the given rectangle
    virtual bool has_rect_invert() const = 0;                   // check, if the invert method is available
    
    // cubic bézier algorithm
    virtual void bezier(double  x1, double  y1,         // render a cubic bézier curve
                        double cx1, double cy1,
                        double cx2, double cy2,
                        double  x2, double  y2);
    
    virtual void bezier_slur(double  x1, double  y1,    // render a cubic bézier curve with different line width
                             double cx1, double cy1,    //         in the center than at the ends
                             double cx2, double cy2,
                             double  x2, double  y2,
                             double  w0, double  w1);
};

} // end namespace

#endif


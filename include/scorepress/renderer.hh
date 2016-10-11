
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

#ifndef SCOREPRESS_RENDERER_HH
#define SCOREPRESS_RENDERER_HH

#include <string>       // std::string
#include <map>          // std::map

#include "sprites.hh"       // Sprites, SpriteSet, SpriteId
#include "file_reader.hh"   // FileReader
#include "error.hh"         // Score::Error
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API Renderer;  // abstract vector-graphics and svg-sprites-renderer interface


//
//     class Renderer
//    ================
//
// This is a (partially) abstract renderer interface.
// It is used by the engine to render the prepared score, such that the engine
// is independent of the used frontend.
//
class SCOREPRESS_API Renderer
{
 public:
    // text alignment enumeration
    enum enuAlignment {ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER};
    
 protected:
    Sprites sprites;    // sprites collection
    
 public:
    // sprite-set interface
    const Sprites&   get_sprites() const;                       // return the sprites collection
    const SpriteSet& get_spriteset(const size_t setid) const;   // return a spriteset
    void             dump() const;                              // dump sprite info to stdout
    virtual ~Renderer();                                        // virtual destructor
    
 public:
    // renderer methods (to be implemented by actual renderer)
    virtual bool   ready() const = 0;                                   // is the object ready to render?
    virtual bool   exist(const std::string& sprite) const = 0;          // does the sprite exist?
    virtual bool   exist(const std::string& sprite, const size_t setid) const = 0;
    
    // sprite-set readers
    typedef RefPtr<SpritesetReader> ReaderPtr;
    
    virtual size_t    spriteset_format_count() const         = 0;   // number of supported formats
    virtual ReaderPtr spriteset_reader(const size_t idx = 0) = 0;   // get file-reader for spriteset
    virtual size_t    add_spriteset(ReaderPtr reader)        = 0;   // read new spriteset from reader (returns index)
    
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
    
    // clipping
    virtual void clip(const int x1, const int y1, const int w, const int h) = 0;    // set rectangle clipping
    virtual void unclip() = 0;                                                      // reset the last "clip" call
    
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

// sprite-set interface
inline const Sprites&   Renderer::get_sprites()                     const {return sprites;}
inline const SpriteSet& Renderer::get_spriteset(const size_t setid) const {return sprites[setid];}

} // end namespace

#endif



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

#ifndef SCOREPRESS_ENGINE_HH
#define SCOREPRESS_ENGINE_HH

#include "engraver.hh"     // Engraver
#include "press.hh"        // Press, Plate, PageSet, ViewportParam, StyleParam, UserCursor
#include "renderer.hh"     // Renderer, Sprites
#include "edit_cursor.hh"  // EditCursor
#include "parameters.hh"   // InterfaceParam
#include "log.hh"          // Logging
#include "export.hh"

namespace ScorePress
{
class SCOREPRESS_API Engine : public Logging
{
 private:
    Document       document;
    PageSet        pageset;
    Engraver       engraver;
    Press          press;
    StyleParam     style;
    ViewportParam  viewport;
    InterfaceParam interface;
    
 public:
    EditCursor cursor;
    
 public:
    Engine(Sprites& sprites);
    
    void set_test(Sprites& sprites);
    void set_resolution(unsigned int hppm, unsigned int vppm);
    
    inline Document&       get_document()             {return document;};
    inline EngraverParam&  get_engraver_parameters()  {return engraver.parameters;};
    inline PressParam&     get_press_parameters()     {return press.parameters;};
    inline StyleParam&     get_style_parameters()     {return style;};
    inline InterfaceParam& get_interface_parameters() {return interface;};
    inline ViewportParam&  get_viewport()             {return viewport;};
    
    mpx_t        page_width()  const;
    mpx_t        page_height() const;
    size_t       page_count()  const;
    unsigned int view_width()  const;   // in pixel
    unsigned int view_height() const;   // in pixel
    
    void plate_dump() const;
    
    void engrave();
    void render(Renderer& renderer, const Position<mpx_t>& offset);
    void render_cursor(Renderer& renderer, const Position<mpx_t>& offset);
    
    void log_set(Log& log);
    void log_unset();
};

inline mpx_t  Engine::page_width()  const {return (viewport.umtopx_h(document.page_layout.width)  * press.parameters.scale) / 1000;}
inline mpx_t  Engine::page_height() const {return (viewport.umtopx_v(document.page_layout.height) * press.parameters.scale) / 1000;}
inline size_t Engine::page_count()  const {return pageset.pages.size();}

} // end namespace

#endif


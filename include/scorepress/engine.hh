
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

#ifndef SCOREPRESS_ENGINE_HH
#define SCOREPRESS_ENGINE_HH

#include "engraver.hh"      // Engraver
#include "press.hh"         // Press, Plate, Pageset, ViewportParam, StyleParam, UserCursor
#include "renderer.hh"      // Renderer, Sprites
#include "edit_cursor.hh"   // EditCursor, CursorBase
#include "parameters.hh"    // InterfaceParam
#include "error.hh"         // Error
#include "log.hh"           // Logging
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
struct SCOREPRESS_API MultipageLayout;
class  SCOREPRESS_API Engine;


//
//     struct MultipageLayout
//    ========================
//
struct SCOREPRESS_API MultipageLayout
{
    enum SCOREPRESS_API MultipageJoin        {SINGLE, DOUBLE, JOINED, FIRSTOFF};
    enum SCOREPRESS_API MultipageOrientation {VERTICAL, HORIZONTAL};

    MultipageJoin        join;
    MultipageOrientation orientation;
    mpx_t                distance;
    
    MultipageLayout(MultipageJoin join = SINGLE, MultipageOrientation ori = VERTICAL, mpx_t dist = 20000);
};


//
//     class Engine
//    ==============
//
class SCOREPRESS_API Engine : public Logging
{
 public:
    // exception class
    class SCOREPRESS_API Error : public ScorePress::Error
        {public: Error(const std::string& msg);};
    
    // page iterator
    class SCOREPRESS_API Page
    {
     private:
        size_t            idx;
        Pageset::Iterator it;
        
        friend class Engine;
        Page(size_t idx, Pageset::Iterator it);
        
     public:
        inline size_t                get_index() const {return idx;}
        inline const Pageset::pPage& get_data()  const {return *it;}
    };
    
 private:
    // cursor management typedefs
    typedef RefPtr<CursorBase>   CursorPtr;
    typedef std::list<CursorPtr> CursorList;
    
    // private data
    Document*      document;    // the document this engine operates on
    Pageset        pageset;     // target pageset
    Engraver       engraver;    // engraver instance
    Press          press;       // press instance
    ViewportParam  viewport;    // viewport parameters
    InterfaceParam interface;   // interface parameters
    CursorList     cursors;     // cursors (registered for reengrave)
    
 protected:
    // calculate page base position for the given multipage-layout
    const Position<mpx_t> page_pos(const size_t pageno, const MultipageLayout layout) const;
    
    Pageset::PlateInfo& select_plate(const Position<mpx_t>& pos, Page& page);                   // get plateinfo by position (on page)
    Pageset::PlateInfo& select_plate(const Position<mpx_t>& pos, const MultipageLayout layout); // get plateinfo by position (muti-page)
    
 public:
    // constructor (specifying the document the engine will operate on)
    Engine(Document& document, const Sprites& sprites);
    
    // setup
    void set_document(Document& document);                      // change the associated document
    void set_resolution(unsigned int hppm, unsigned int vppm);  // change screen resolution (viewport parameters)
    void engrave();                                             // engrave document (calculates pageset, invalidates cursors)
    void reengrave();                                           // engrave document (recalculate cursors)
    void reengrave(UserCursor& cursor);                         // reengrave score  (recalculate cursors)
    
    // internal data access
    Document&              get_document();                      // the document this engine operates on
    EngraverParam&         get_engraver_parameters();           // default engraver parameters
    PressParam&            get_press_parameters();              // press parameters
    StyleParam&            get_style_parameters();              // default style parameters
    InterfaceParam&        get_interface_parameters();          // interface parameters
    ViewportParam&         get_viewport();                      // viewport parameters
    
    const Document&        get_document()             const;    // the document this engine operates on
    const EngraverParam&   get_engraver_parameters()  const;    // default engraver parameters
    const PressParam&      get_press_parameters()     const;    // press parameters
    const StyleParam&      get_style_parameters()     const;    // default style parameters
    const InterfaceParam&  get_interface_parameters() const;    // interface parameters
    const ViewportParam&   get_viewport()             const;    // viewport parameters
    
    void plate_dump() const;                                    // write plate-content to stdout
    
    // dimension information
    mpx_t  page_width()  const;                                 // graphical page width
    mpx_t  page_height() const;                                 // graphical page height
    size_t page_count()  const;                                 // page count
    mpx_t  layout_width(const MultipageLayout layout)  const;   // width of complete layout
    mpx_t  layout_height(const MultipageLayout layout) const;   // height of complete layout
    
    // rendering
    void render_page(Renderer& renderer, const Page page,              const Position<mpx_t>& offset, bool decor = false);          // single page (at pos)
    void render_all( Renderer& renderer, const MultipageLayout layout, const Position<mpx_t>& offset, bool decor = false);          // all pages (with layout)
    
    void render_cursor(Renderer& renderer, const UserCursor&   cursor, const Position<mpx_t>& page_pos);                            // cursor (with page root)
    void render_cursor(Renderer& renderer, const UserCursor&   cursor, const MultipageLayout layout, const Position<mpx_t>& off);   // cursor (with layout)
    void render_cursor(Renderer& renderer, const ObjectCursor& cursor, const Position<mpx_t>& page_pos);                            // object frame (with page root)
    void render_cursor(Renderer& renderer, const ObjectCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& off);   // object frame (with layout)
    void render_object(Renderer& renderer, const ObjectCursor& cursor, const Position<mpx_t>& page_pos);                            // selected object (with page root)
    void render_object(Renderer& renderer, const ObjectCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& off);   // selected object (with layout)
    
    // cursor interface
    const Page select_page(const size_t           page);                                    // calculate page-iterator by index
    const Page select_page(      Position<mpx_t>& pos, const MultipageLayout layout);       // calculate page-iterator by position (transform pos to on-page pos)
    const Page select_page(const Position<mpx_t>& pos, const MultipageLayout layout);       // calculate page-iterator by position
    
    Document::Score& select_score(const Position<mpx_t>&, const Page&           page);      // get score by position (on page)
    Document::Score& select_score(      Position<mpx_t>,  const MultipageLayout layout);    // get score by position (muti-page)
    
    RefPtr<EditCursor> get_cursor();                                                        // get cursor (front of first score)
    RefPtr<EditCursor> get_cursor(Document::Score& score);                                  // get cursor (front of given score)
    RefPtr<EditCursor> get_cursor(Position<mpx_t> pos, const Page& page);                   // get cursor (on-page position)
    RefPtr<EditCursor> get_cursor(Position<mpx_t> pos, const MultipageLayout layout);       // get cursor (multi-page position)
    
    void set_cursor(RefPtr<EditCursor>& cursor);                                            // set cursor (does not register for reengraving)
    void set_cursor(RefPtr<EditCursor>& cursor, Document::Score& score);
    void set_cursor(RefPtr<EditCursor>& cursor, Position<mpx_t> pos, const Page& page);
    void set_cursor(RefPtr<EditCursor>& cursor, Position<mpx_t> pos, const MultipageLayout layout);
    
    RefPtr<ObjectCursor> select_object();                                                   // get object cursor (on first page)
    RefPtr<ObjectCursor> select_object(EditCursor& cur);                                    // get object cursor (at given note)
    RefPtr<ObjectCursor> select_object(Position<mpx_t> pos, const Page& page);              // get object cursor (on-page position)
    RefPtr<ObjectCursor> select_object(Position<mpx_t> pos, const MultipageLayout layout);  // get object cursor (multi-page position)
    
    bool set_cursor(RefPtr<ObjectCursor>& cursor);                                          // set cursor (does not register for reengraving)
    bool set_cursor(RefPtr<ObjectCursor>& cursor, EditCursor& cur);
    bool set_cursor(RefPtr<ObjectCursor>& cursor, Position<mpx_t> pos, const Page& page);
    bool set_cursor(RefPtr<ObjectCursor>& cursor, Position<mpx_t> pos, const MultipageLayout layout);
    
    bool register_cursor(RefPtr<CursorBase> cursor);                                        // register cursor for reengraving
                                                                                            // (cursors created by "get_cursor" are already registered)
    // logging control
    using Logging::log_set;
    void log_set(Log& log);     // set log instance
    void log_unset();           // unset log instance
};

// inline method implementations
inline MultipageLayout::MultipageLayout(MultipageJoin _join, MultipageOrientation _ori, mpx_t _dist) : join(_join), orientation(_ori), distance(_dist) {}

inline Engine::Page::Page(size_t _idx, Pageset::Iterator _it) : idx(_idx), it(_it) {}

inline Document&       Engine::get_document()             {return *document;}
inline EngraverParam&  Engine::get_engraver_parameters()  {return engraver.parameters;}
inline PressParam&     Engine::get_press_parameters()     {return press.parameters;}
inline InterfaceParam& Engine::get_interface_parameters() {return interface;}
inline ViewportParam&  Engine::get_viewport()             {return viewport;}

inline const Document&       Engine::get_document()             const {return *document;}
inline const EngraverParam&  Engine::get_engraver_parameters()  const {return engraver.parameters;}
inline const PressParam&     Engine::get_press_parameters()     const {return press.parameters;}
inline const InterfaceParam& Engine::get_interface_parameters() const {return interface;}
inline const ViewportParam&  Engine::get_viewport()             const {return viewport;}

inline void Engine::set_document(Document& _document)              {document = &_document; pageset.clear(); cursors.clear();}
inline void Engine::set_resolution(unsigned int h, unsigned int v) {viewport.hppm = h; viewport.vppm = v;}

inline mpx_t  Engine::page_width()  const {return (viewport.umtopx_h(document->page_layout.width)  * press.parameters.scale) / 1000;}
inline mpx_t  Engine::page_height() const {return (viewport.umtopx_v(document->page_layout.height) * press.parameters.scale) / 1000;}
inline size_t Engine::page_count()  const {return pageset.pages.size();}

inline const Engine::Page Engine::select_page(const size_t page) {return Page(page, pageset.get_page(page));}

} // end namespace

#endif


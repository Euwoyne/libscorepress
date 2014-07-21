
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

#ifndef SCOREPRESS_ENGINE_HH
#define SCOREPRESS_ENGINE_HH

#include "engraver.hh"      // Engraver
#include "press.hh"         // Press, Plate, Pageset, ViewportParam, StyleParam, UserCursor
#include "renderer.hh"      // Renderer, Sprites
#include "edit_cursor.hh"   // EditCursor
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
        unsigned int      idx;
        Pageset::Iterator it;
        
        friend class Engine;
        Page(unsigned int idx, Pageset::Iterator it);
        
     public:
        inline unsigned int          get_index() const {return idx;};
        inline const Pageset::pPage& get_data()  const {return *it;};
    };
    
 public:    // TODO: make private
    typedef std::map<const Score*, EditCursor> CursorMap;
    
    Document*      document;    // the document this engine operates on
    Pageset        pageset;     // target pageset
    Engraver       engraver;    // engraver instance
    Press          press;       // press instance
    StyleParam     style;       // default style parameters (usually overwritten by document)
    ViewportParam  viewport;    // viewport parameters
    InterfaceParam interface;   // interface parameters
    CursorMap      cursors;     // cursor map (one cursor per score-object)
    ObjectCursor   object_cur;  // object cursor
    
 protected:
    // calculate page base position for the given multipage-layout
    const Position<mpx_t> page_pos(const unsigned int pageno, const MultipageLayout layout);
    
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
    
    // redering
    void render_page(Renderer& renderer, const Page page, const Position<mpx_t>& offset, bool decor = false);                         // single page (at pos)
    void render_all(Renderer& renderer, const MultipageLayout layout, const Position<mpx_t>& offset, bool decor = false);             // all pages (with layout)
    void render_cursor(Renderer& renderer, const UserCursor& cursor, const Position<mpx_t>& page_pos);                                // cursor (with page root)
    void render_cursor(Renderer& renderer, const UserCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& offset);    // cursor (with layout)
    void render_selection(Renderer& renderer, const Position<mpx_t>& page_pos, const Position<mpx_t>& move_offset);                                          // object frame (with page root)
    void render_selection(Renderer& renderer, const MultipageLayout layout, const Position<mpx_t>& offset, const Position<mpx_t>& move_offset);              // object frame (with layout)
    void render_selection(Renderer& renderer, const ObjectCursor& cursor, const Position<mpx_t>& page_pos, const Position<mpx_t>& move_offset);              // object frame (with page root)
    void render_selection(Renderer& renderer, const ObjectCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& o, const Position<mpx_t>& m); // object frame (with layout)
    void render_object(Renderer& renderer, const ObjectCursor& cursor, const Position<mpx_t>& page_pos);                              // selected object (with page root)
    void render_object(Renderer& renderer, const ObjectCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& o);       // selected object (with layout)
    
    // internal data access
    Document&        get_document();                                    // the document this engine operates on
    EngraverParam&   get_engraver_parameters();                         // default engraver parameters
    PressParam&      get_press_parameters();                            // press parameters
    StyleParam&      get_style_parameters();                            // default style parameters
    InterfaceParam&  get_interface_parameters();                        // interface parameters
    ViewportParam&   get_viewport();                                    // viewport parameters
    
    // dimension information
    mpx_t            page_width()  const;                               // graphical page width  (in millipixel)
    mpx_t            page_height() const;                               // graphical page height (in millipixel)
    unsigned int     page_count()  const;                               // page count
    unsigned int     layout_width(const MultipageLayout layout)  const; // width of complete layout  (in pixel)
    unsigned int     layout_height(const MultipageLayout layout) const; // height of complete layout (in pixel)
    
    // plate access
    void             plate_dump() const;                                // write plate-content to stdout
    
    // cursor interface
    const Page select_page(const unsigned int page);                                    // calculate page-iterator by index
    const Page select_page(Position<mpx_t>& pos, const MultipageLayout layout);         // calculate page-iterator by position (transform pos to on-page pos)
    const Page select_page(const Position<mpx_t>& pos, const MultipageLayout layout);   // calculate page-iterator by position
    
    Document::Score& select_score(const Position<mpx_t>& pos, const Page& page);        // get score by position (on page)
    Document::Score& select_score(Position<mpx_t> pos, const MultipageLayout layout);   // get score by position (muti-page)
    
    EditCursor& get_cursor()                                                  throw(Error, UserCursor::Error); // get cursor (front of first score)
    EditCursor& get_cursor(Document::Score& score)                            throw(Error, UserCursor::Error); // get cursor (front of given score)
    EditCursor& get_cursor(Position<mpx_t> pos, const Page& page)             throw(Error, UserCursor::Error); // get cursor (on-page position)
    EditCursor& get_cursor(Position<mpx_t> pos, const MultipageLayout layout) throw(Error, UserCursor::Error); // get cursor (multi-page position)
    
    const ObjectCursor& selected_object()                                 throw(Error); // get currently selected object
    bool has_selected_object() const;                                                   // check, if there is a selected object
    bool select_object(Position<mpx_t> pos, const Page& page)             throw(Error); // get object by position (on page)
    bool select_object(Position<mpx_t> pos, const MultipageLayout layout) throw(Error); // get object by position (multi-page)
    void deselect_object();                                                             // deselect selected object (if any)
    // NOTE: Ownership of the returned cursors stays with the engine. Any copies, that are not owned by the engine, might behave strangely on reengrave.
    
    // object interface
    void move_object(const ObjectCursor& cursor, const Position<mpx_t> offset);         // move object by "offset"
    
    // logging control
    void             log_set(Log& log);
    void             log_unset();
};

// inline method implementations
inline MultipageLayout::MultipageLayout(MultipageJoin _join, MultipageOrientation _ori, mpx_t _dist) : join(_join), orientation(_ori), distance(_dist) {}

inline Engine::Page::Page(unsigned int _idx, Pageset::Iterator _it) : idx(_idx), it(_it) {}

inline void Engine::set_document(Document& _document)              {document = &_document; pageset.clear(); cursors.clear();}
inline void Engine::set_resolution(unsigned int h, unsigned int v) {viewport.hppm = h; viewport.vppm = v;}

inline void Engine::render_selection(Renderer& r, const Position<mpx_t>& p, const Position<mpx_t>& m)                          {render_selection(r, object_cur, p, m);}
inline void Engine::render_selection(Renderer& r, const MultipageLayout l, const Position<mpx_t>& o, const Position<mpx_t>& m) {render_selection(r, object_cur, l, o, m);}

inline Document&       Engine::get_document()             {return *document;}
inline EngraverParam&  Engine::get_engraver_parameters()  {return engraver.parameters;}
inline PressParam&     Engine::get_press_parameters()     {return press.parameters;}
inline StyleParam&     Engine::get_style_parameters()     {return style;}
inline InterfaceParam& Engine::get_interface_parameters() {return interface;}
inline ViewportParam&  Engine::get_viewport()             {return viewport;}

inline mpx_t           Engine::page_width()  const        {return (viewport.umtopx_h(document->page_layout.width)  * press.parameters.scale) / 1000;}
inline mpx_t           Engine::page_height() const        {return (viewport.umtopx_v(document->page_layout.height) * press.parameters.scale) / 1000;}
inline size_t          Engine::page_count()  const        {return pageset.pages.size();}

inline const Engine::Page Engine::select_page(const unsigned int page) {return Page(page, pageset.get_page(page));}
inline bool  Engine::has_selected_object() const {return (object_cur.ready() && !object_cur.end());}
inline const ObjectCursor& Engine::selected_object() throw(Error) {return object_cur;}
inline bool  Engine::select_object(Position<mpx_t> pos, const MultipageLayout layout) throw(Error) {
                                return select_object(pos, select_page(pos, layout));}
inline void  Engine::deselect_object() {object_cur.reset();}

} // end namespace

#endif


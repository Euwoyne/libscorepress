
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

#include <iostream>

#include "engine.hh"
#include "log.hh"               // Log

using namespace ScorePress;

inline unsigned int _abs(const int x) {return static_cast<unsigned int>((x<0)?-x:x);}
inline int _round(const double d) {return static_cast<mpx_t>(d + 0.5);}

// exception class
Engine::Error::Error(const std::string& msg) : ScorePress::Error(msg) {}

// calculate page base position for the given multipage-layout
const Position<mpx_t> Engine::page_pos(const unsigned int pageno, const MultipageLayout layout) const
{
    switch (layout.join)
    {
    case MultipageLayout::DOUBLE:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? Position<mpx_t>((pageno % 2) ? page_width() + layout.distance : 0, (pageno >> 1) * (page_height() + layout.distance))
            : Position<mpx_t>(pageno * (page_width() + layout.distance), 0);
    case MultipageLayout::JOINED:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? Position<mpx_t>((pageno % 2) ? page_width() : 0, (pageno >> 1) * (page_height() + layout.distance))
            : Position<mpx_t>(pageno * page_width() + (pageno >> 1) * layout.distance, 0);
    case MultipageLayout::FIRSTOFF:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? Position<mpx_t>((pageno % 2) ? 0 : page_width(), ((pageno + 1) >> 1) * (page_height() + layout.distance))
            : Position<mpx_t>(pageno * page_width() + ((pageno + 1) >> 1) * layout.distance, 0);
    default:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? Position<mpx_t>(0, pageno * (page_height() + layout.distance))
            : Position<mpx_t>(pageno * (page_width() + layout.distance), 0);
    };
}

// get plateinfo by position (on page)
Pageset::PlateInfo& Engine::select_plate(const Position<mpx_t>& pos, Page& page)
{
    // seach the plate
    Pageset::pPage::Iterator pinfo = page.it->get_plate_by_pos(pos);
    return (pinfo == page.it->plates.end()) ? page.it->plates.back() : *pinfo;
}

// get plateinfo by position (muti-page)
Pageset::PlateInfo& Engine::select_plate(const Position<mpx_t>& pos, const MultipageLayout layout)
{
    // get page
    Pageset::Iterator page;
    unsigned int i = 0;
    Position<mpx_t> pagepos;
    const Position<mpx_t> pagedim(page_width(), page_height());
    for (page = pageset.pages.begin(); page != pageset.pages.end(); ++page, ++i)
    {
        pagepos = page_pos(i, layout);
        if (   pos.x >= pagepos.x             && pos.y >= pagepos.y
            && pos.x <  pagepos.x + pagedim.x && pos.y <  pagepos.y + pagedim.y)
                break;
    };
    if (page == pageset.pages.end()) --page;
    
    // seach the plate
    Pageset::pPage::Iterator pinfo = page->get_plate_by_pos(pos);
    return (pinfo == page->plates.end()) ? page->plates.back() : *pinfo;
}

// constructor (specifying the document the engine will operate on)
Engine::Engine(Document& _document, const Sprites& sprites) : document(&_document),
                                                              engraver(pageset, sprites, style, viewport),
                                                              press(style, viewport) {}

// engrave document (calculates pageset)
void Engine::engrave()
{
    engraver.engrave(*document);
    cursors.clear();
}

// engrave document (recalculate cursors)
void Engine::reengrave()
{
    // setup reengrave info
    ReengraveInfo info;
    for (CursorList::iterator cur = cursors.begin(); cur != cursors.end();)
    {
        if (getRefCount(*cur) == 1)
        {
            cur = cursors.erase(cur);
        }
        else
        {
            (*cur)->setup_reengrave(info);
            ++cur;
        };
    };
    
    // reengrave
    engraver.engrave(*document, info);
    info.finish();
    if (!info.is_empty())
        log_error("Some cursors could not be updated. (class: Engine)");
}

// engrave single score (recalculate cursors)
void Engine::reengrave(UserCursor& cursor)
{
    // setup reengrave info
    ReengraveInfo info;
    for (CursorList::iterator cur = cursors.begin(); cur != cursors.end();)
    {
        if (getRefCount(*cur) == 1)
        {
            cur = cursors.erase(cur);
        }
        else
        {
            if (&(*cur)->get_score() == &cursor.get_score())
                (*cur)->setup_reengrave(info);
            ++cur;
        };
    };
    
    // reengrave
    engraver.engrave(cursor.get_score(), cursor.get_start_page(), document->head_height, info);
    info.finish();
    if (!info.is_empty())
        log_error("Some cursors could not be updated. (class: Engine)");
}

// render a single page at the given offset
void Engine::render_page(Renderer& renderer, const Page page, const Position<mpx_t>& offset, bool decor)
{
    if (pageset.pages.empty()) engrave();
    Position<mpx_t> margin_offset(_round(press.parameters.do_scale(pageset.page_layout.margin.left)),
                                  _round(press.parameters.do_scale(pageset.page_layout.margin.top)));
    
    if (decor) press.render_decor(renderer, pageset, offset);
    press.render(renderer, *page.it, pageset, offset + margin_offset);
}

// render all pages according to the given layout
void Engine::render_all(Renderer& renderer, const MultipageLayout layout, const Position<mpx_t>& offset, bool decor)
{
    if (pageset.pages.empty()) engrave();
    Position<mpx_t> margin_offset(_round(press.parameters.do_scale(pageset.page_layout.margin.left)),
                                  _round(press.parameters.do_scale(pageset.page_layout.margin.top)));
    
    Position<mpx_t> off;
    unsigned int pageno = 0;
    for (std::list<Pageset::pPage>::iterator i = pageset.pages.begin(); i != pageset.pages.end(); ++i)
    {
        off = offset + page_pos(pageno++, layout);
        if (decor) press.render_decor(renderer, pageset, off);
        press.render(renderer, *i, pageset, off + margin_offset);
    };
}

// render the cursor, assuming the given page root position
void Engine::render_cursor(Renderer& renderer, const UserCursor& cursor, const Position<mpx_t>& _page_pos)
{
    Position<mpx_t> margin_offset(_round(press.parameters.do_scale(pageset.page_layout.margin.left)),
                                  _round(press.parameters.do_scale(pageset.page_layout.margin.top)));
    
    renderer.set_color(0, 0, 0, 255);
    press.render(renderer, cursor, _page_pos + margin_offset);
}

// render the cursor, calculating page positions according to the given layout
void Engine::render_cursor(Renderer& renderer, const UserCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& offset)
{
    Position<mpx_t> margin_offset(_round(press.parameters.do_scale(pageset.page_layout.margin.left)),
                                  _round(press.parameters.do_scale(pageset.page_layout.margin.top)));
    
    renderer.set_color(0, 0, 0, 255);
    press.render(renderer, cursor, offset + page_pos(cursor.get_pageno(), layout) + margin_offset);
}

// render object frame, assuming the given page root position
void Engine::render_cursor(Renderer& renderer, const ObjectCursor& cursor, const Position<mpx_t>& _page_pos)
{
    if (!cursor.ready() || cursor.end()) return;
    Position<mpx_t> margin_offset(_round(press.parameters.do_scale(pageset.page_layout.margin.left)),
                                  _round(press.parameters.do_scale(pageset.page_layout.margin.top)));
    press.render(renderer, cursor, _page_pos + margin_offset);
}

// render object frame, calculating page positions according to the given layout
void Engine::render_cursor(Renderer& renderer, const ObjectCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& offset)
{
    if (!cursor.ready() || cursor.end()) return;
    Position<mpx_t> margin_offset(_round(press.parameters.do_scale(pageset.page_layout.margin.left)),
                                  _round(press.parameters.do_scale(pageset.page_layout.margin.top)));
    press.render(renderer, cursor, offset + page_pos(cursor.get_pageno(), layout) + margin_offset);
}

// render selected object, assuming the given page root position
void Engine::render_object(Renderer& renderer, const ObjectCursor& cursor, const Position<mpx_t>& _page_pos)
{
    if (!cursor.ready() || cursor.end()) return;
    Position<mpx_t> margin_offset(_round(press.parameters.do_scale(pageset.page_layout.margin.left)),
                                  _round(press.parameters.do_scale(pageset.page_layout.margin.top)));
    press.render(renderer, cursor.get_pobject(), cursor.get_staff(), _page_pos + margin_offset);
}

// render selected object, calculating page positions according to the given layout
void Engine::render_object(Renderer& renderer, const ObjectCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& offset)
{
    if (!cursor.ready() || cursor.end()) return;
    Position<mpx_t> margin_offset(_round(press.parameters.do_scale(pageset.page_layout.margin.left)),
                                  _round(press.parameters.do_scale(pageset.page_layout.margin.top)));
    press.render(renderer, cursor.get_pobject(), cursor.get_staff(), offset + page_pos(cursor.get_pageno(), layout) + margin_offset);
}

// width of complete layout
mpx_t Engine::layout_width(const MultipageLayout layout) const
{
    if (pageset.pages.empty()) return 0;
    switch (layout.join)
    {
    case MultipageLayout::DOUBLE:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? ((pageset.pages.size() > 1) ? 2 * page_width() + layout.distance : page_width())
            : pageset.pages.size() * page_width() + (pageset.pages.size() - 1) * layout.distance;
    case MultipageLayout::JOINED:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? ((pageset.pages.size() > 1) ? 2 * page_width() : page_width())
            : pageset.pages.size() * page_width() + ((pageset.pages.size() - 1) >> 1) * layout.distance;
    case MultipageLayout::FIRSTOFF:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? 2 * page_width()
            : pageset.pages.size() * page_width() + (pageset.pages.size() >> 1) * layout.distance;
    default:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? page_width()
            : pageset.pages.size() * page_width() + (pageset.pages.size() - 1) * layout.distance;
    };
}

// height of complete layout
mpx_t Engine::layout_height(const MultipageLayout layout) const
{
    if (pageset.pages.empty()) return 0;
    switch (layout.join)
    {
    case MultipageLayout::DOUBLE:
    case MultipageLayout::JOINED:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? ((pageset.pages.size() + 1) >> 1) * page_height() + ((pageset.pages.size() - 1) >> 1) * layout.distance
            : page_height();
    case MultipageLayout::FIRSTOFF:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? ((pageset.pages.size() + 2) >> 1) * page_height() + (pageset.pages.size() >> 1) * layout.distance
            : page_height();
    default:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? pageset.pages.size() * page_height() + (pageset.pages.size() - 1) * layout.distance
            : page_height();
    };
}

// calculate page-iterator by position  (transform pos to on-page pos)
const Engine::Page Engine::select_page(Position<mpx_t>& pos, const MultipageLayout layout)
{
    unsigned int i = 0;
    Position<mpx_t> pagepos;
    const Position<mpx_t> pagedim(page_width(), page_height());
    for (Pageset::Iterator p = pageset.pages.begin(); p != pageset.pages.end(); ++p, ++i)
    {
        pagepos = page_pos(i, layout);
        if (   pos.x >= pagepos.x             && pos.y >= pagepos.y
            && pos.x <  pagepos.x + pagedim.x && pos.y <  pagepos.y + pagedim.y)
        {
                pos -= pagepos;
                return Page(i, p);
        };
    };
    pos -= pagepos;
    return Page(--i, --pageset.pages.end());
}

// calculate page-iterator by position
const Engine::Page Engine::select_page(const Position<mpx_t>& pos, const MultipageLayout layout)
{
    unsigned int i = 0;
    Position<mpx_t> pagepos;
    const Position<mpx_t> pagedim(page_width(), page_height());
    for (Pageset::Iterator p = pageset.pages.begin(); p != pageset.pages.end(); ++p, ++i)
    {
        pagepos = page_pos(i, layout);
        if (   pos.x >= pagepos.x             && pos.y >= pagepos.y
            && pos.x <  pagepos.x + pagedim.x && pos.y <  pagepos.y + pagedim.y)
                return Page(i, p);
    };
    return Page(--i, --pageset.pages.end());
}

// get score by position (on page)
Document::Score& Engine::select_score(const Position<mpx_t>& pos, const Page& page)
{
    // seach the plate
    Pageset::pPage::const_Iterator pinfo = page.it->get_plate_by_pos(pos);
    if (pinfo == page.it->plates.end())
        pinfo = --page.it->plates.end();
    
    // search the score
    for (Document::ScoreList::iterator score = document->scores.begin(); score != document->scores.end(); ++score)
        if (&score->score == pinfo->score)
            return *score;
    
    // if no score is found, throw error
    throw Error("Unable to find the selected score (on-plate) in the document.");
}

// get score by position (muti-page)
Document::Score& Engine::select_score(Position<mpx_t> pos, const MultipageLayout layout)
{
    // get page
    Pageset::Iterator page(select_page(pos, layout).it);
    
    // seach the plate
    Pageset::pPage::const_Iterator pinfo = page->get_plate_by_pos(pos);
    if (pinfo == page->plates.end())
        pinfo = --page->plates.end();
    
    // search the score
    for (Document::ScoreList::iterator score = document->scores.begin(); score != document->scores.end(); ++score)
        if (&score->score == pinfo->score)
            return *score;
    
    // if no score is found, throw error
    throw Error("Unable to find the selected score (on-plate) in the document.");
}

// create cursor (front of first score)
RefPtr<EditCursor> Engine::get_cursor()
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    RefPtr<EditCursor> cursor(new EditCursor(*document, pageset, interface, style, viewport));
    cursor->set_score(document->scores.front());
    cursor->log_set(*this);
    cursors.push_back(cursor);
    return cursor;
}

// create cursor (front of given score)
RefPtr<EditCursor> Engine::get_cursor(Document::Score& score)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    RefPtr<EditCursor> cursor(new EditCursor(*document, pageset, interface, style, viewport));
    cursor->set_score(score);
    cursor->log_set(*this);
    cursors.push_back(cursor);
    return cursor;
}

// create cursor (on-page position)
RefPtr<EditCursor> Engine::get_cursor(Position<mpx_t> pos, const Page& page)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    Document::Score* const score(&select_score(pos, page));
    RefPtr<EditCursor> cursor(new EditCursor(*document, pageset, interface, style, viewport));
    cursor->set_score(*score);
    cursor->log_set(*this);
    cursor->set_pos((pos * 1000) / static_cast<int>(press.parameters.scale), page.it, viewport);
    cursors.push_back(cursor);
    return cursor;
}

// create cursor (multi-page position)
RefPtr<EditCursor> Engine::get_cursor(Position<mpx_t> pos, const MultipageLayout layout)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    const Page page(select_page(pos, layout));
    Document::Score* const score(&select_score(pos, page));
    RefPtr<EditCursor> cursor(new EditCursor(*document, pageset, interface, style, viewport));
    cursor->set_score(*score);
    cursor->log_set(*this);
    cursor->set_pos((pos * 1000) / static_cast<int>(press.parameters.scale), page.it, viewport);
    cursors.push_back(cursor);
    return cursor;
}

// set cursor (front of first score)
void Engine::set_cursor(RefPtr<EditCursor>& cursor)
{
    cursor->set_score(document->scores.front());
}

// set cursor (front of given score)
void Engine::set_cursor(RefPtr<EditCursor>& cursor, Document::Score& score)
{
    cursor->set_score(score);
}

// set cursor (on-page position)
void Engine::set_cursor(RefPtr<EditCursor>& cursor, Position<mpx_t> pos, const Page& page)
{
    cursor->set_score(select_score(pos, page));
    cursor->set_pos((pos * 1000) / static_cast<int>(press.parameters.scale), page.it, viewport);
}

// set cursor (multi-page position)
void Engine::set_cursor(RefPtr<EditCursor>& cursor, Position<mpx_t> pos, const MultipageLayout layout)
{
    cursor->set_score(select_score(pos, select_page(pos, layout)));
    cursor->set_pos((pos * 1000) / static_cast<int>(press.parameters.scale), select_page(pos, layout).it, viewport);
}

// get object cursor (on first page)
RefPtr<ObjectCursor> Engine::select_object()
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    RefPtr<ObjectCursor> cursor(new ObjectCursor(*document, pageset));
    if (!cursor->set_parent(pageset.pages.front()))
        return RefPtr<ObjectCursor>();
    cursors.push_back(cursor);
    return cursor;
}

// get object cursor (at given note)
RefPtr<ObjectCursor> Engine::select_object(EditCursor& cur)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    RefPtr<ObjectCursor> cursor(new ObjectCursor(*document, pageset));
    if (!cursor->set_parent(cur))
        return RefPtr<ObjectCursor>();
    cursors.push_back(cursor);
    return cursor;
}

// get object cursor (on-page position)
RefPtr<ObjectCursor> Engine::select_object(Position<mpx_t> pos, const Page& page)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    RefPtr<ObjectCursor> cursor(new ObjectCursor(*document, pageset));
    if (!cursor->select(pos, *page.it))
        return RefPtr<ObjectCursor>();
    cursors.push_back(cursor);
    return cursor;
}

// get object cursor (multi-page position)
RefPtr<ObjectCursor> Engine::select_object(Position<mpx_t> pos, const MultipageLayout layout)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    const Page page(select_page(pos, layout));
    RefPtr<ObjectCursor> cursor(new ObjectCursor(*document, pageset));
    if (!cursor->select(pos, *page.it))
        return RefPtr<ObjectCursor>();
    cursors.push_back(cursor);
    return cursor;
}

// set object cursor (on first page)
bool Engine::set_cursor(RefPtr<ObjectCursor>& cursor)
{
    if (&cursor->get_document() != document || &cursor->get_pageset() != &pageset) return false;
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    return cursor->set_parent(pageset.pages.front());
}

// set object cursor (at given note)
bool Engine::set_cursor(RefPtr<ObjectCursor>& cursor, EditCursor& cur)
{
    if (&cursor->get_document() != document || &cursor->get_pageset() != &pageset) return false;
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    return cursor->set_parent(cur);
}

// set object cursor (on-page position)
bool Engine::set_cursor(RefPtr<ObjectCursor>& cursor, Position<mpx_t> pos, const Page& page)
{
    if (&cursor->get_document() != document || &cursor->get_pageset() != &pageset) return false;
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    return cursor->select(pos, *page.it);
}

// set object cursor (multi-page position)
bool Engine::set_cursor(RefPtr<ObjectCursor>& cursor, Position<mpx_t> pos, const MultipageLayout layout)
{
    if (&cursor->get_document() != document || &cursor->get_pageset() != &pageset) return false;
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    const Page page(select_page(pos, layout));
    return cursor->select(pos, *page.it);
}

// register cursor for reengraving
bool Engine::register_cursor(RefPtr<CursorBase> cursor)
{
    for (CursorList::iterator cur = cursors.begin(); cur != cursors.end(); ++cur)
        if (*cur == cursor) return false;
    cursors.push_back(cursor);
    return true;
}

// logging control
void Engine::log_set(Log& log)
{
    this->Logging::log_set(log);
    engraver.log_set(log);
    press.log_set(log);
}

void Engine::log_unset()
{
    this->Logging::log_unset();
    engraver.log_unset();
    press.log_unset();
}

// write plate-content to stdout
void Engine::plate_dump() const
{
    size_t idx = 0;
    for (Pageset::const_Iterator p = pageset.pages.begin(); p != pageset.pages.end(); ++p)
    {
        std::cout << "=== PAGE " << ++idx << " ===\n";
        p->plates.front().plate->dump();
    };
}


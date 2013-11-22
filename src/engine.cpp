
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

#include <iostream>
#include "engine.hh"
#include "log.hh"

using namespace ScorePress;

// exception class
Engine::Error::Error(const std::string& msg) : ScorePress::Error(msg) {}

// calculate page base position for the given multipage-layout
const Position<mpx_t> Engine::page_pos(const unsigned int pageno, const MultipageLayout layout)
{
    switch (layout.join)
    {
    case MultipageLayout::SINGLE:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? Position<mpx_t>(0, pageno * (page_height() + layout.distance))
            : Position<mpx_t>(pageno * (page_width() + layout.distance), 0);
    case MultipageLayout::DOUBLE:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? Position<mpx_t>((pageno % 2) ? page_width() + layout.distance : 0, (pageno / 2) * (page_height() + layout.distance))
            : Position<mpx_t>(pageno * (page_width() + layout.distance), 0);
    case MultipageLayout::JOINED:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? Position<mpx_t>((pageno % 2) ? page_width() : 0, (pageno / 2) * (page_height() + layout.distance))
            : Position<mpx_t>(pageno * page_width() + (pageno / 2) * layout.distance, 0);
    case MultipageLayout::FIRSTOFF:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? Position<mpx_t>((pageno % 2) ? 0 : page_width(), ((pageno + 1) / 2) * (page_height() + layout.distance))
            : Position<mpx_t>(pageno * page_width() + ((pageno + 1) / 2) * layout.distance, 0);
    };
    return Position<mpx_t>();
}

// constructor (specifying the document the engine will operate on)
Engine::Engine(Document& _document, Sprites& sprites) : document(&_document),
                                                        engraver(pageset, sprites, style, viewport),
                                                        press(style, viewport) {}

// engrave document (calculates pageset)
void Engine::engrave()
{
    engraver.engrave(*document);
}

// render a single page at the given offset
void Engine::render_page(Renderer& renderer, const Page page, const Position<mpx_t>& offset, bool decor)
{
    if (pageset.pages.empty()) engrave();
    Position<mpx_t> margin_offset((press.parameters.scale * pageset.page_layout.margin.left) / 1000,
                                  (press.parameters.scale * pageset.page_layout.margin.top) / 1000);
    
    if (decor) press.render_decor(renderer, pageset, offset);
    press.render(renderer, *page.it, pageset, offset + margin_offset);
}

// render all pages according to the given layout
void Engine::render_all(Renderer& renderer, const MultipageLayout layout, const Position<mpx_t>& offset, bool decor)
{
    if (pageset.pages.empty()) engrave();
    Position<mpx_t> margin_offset((press.parameters.scale * pageset.page_layout.margin.left) / 1000,
                                  (press.parameters.scale * pageset.page_layout.margin.top) / 1000);
    
    Position<mpx_t> off;
    unsigned int pageno = 0;
    for (std::list<Pageset::pPage>::const_iterator i = pageset.pages.begin(); i != pageset.pages.end(); ++i)
    {
        off = offset + page_pos(pageno++, layout);
        if (decor) press.render_decor(renderer, pageset, off);
        press.render(renderer, *i, pageset, off + margin_offset);
    };
}

// render the cursor, assuming the given page root position
void Engine::render_cursor(Renderer& renderer, const UserCursor& cursor, const Position<mpx_t>& _page_pos)
{
    Position<mpx_t> margin_offset((press.parameters.scale * pageset.page_layout.margin.left) / 1000,
                                  (press.parameters.scale * pageset.page_layout.margin.top) / 1000);
    
    renderer.set_color(0, 0, 0, 255);
    press.render(renderer, cursor, _page_pos + margin_offset);
}

// render the cursor, calculating page positions according to the given layout
void Engine::render_cursor(Renderer& renderer, const UserCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& offset)
{
    Position<mpx_t> margin_offset((press.parameters.scale * pageset.page_layout.margin.left) / 1000,
                                  (press.parameters.scale * pageset.page_layout.margin.top) / 1000);
    
    renderer.set_color(0, 0, 0, 255);
    press.render(renderer, cursor, offset + page_pos(cursor.get_pageno(), layout) + margin_offset);
}

// width of complete layout  (in pixel)
unsigned int Engine::layout_width(const MultipageLayout layout) const
{
    if (pageset.pages.empty()) return 0;
    switch (layout.join)
    {
    case MultipageLayout::SINGLE:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? page_width()
            : pageset.pages.size() * page_width() + (pageset.pages.size() - 1) * layout.distance;
    case MultipageLayout::DOUBLE:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? ((pageset.pages.size() > 1) ? 2 * page_width() + layout.distance : page_width())
            : pageset.pages.size() * page_width() + (pageset.pages.size() - 1) * layout.distance;
    case MultipageLayout::JOINED:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? ((pageset.pages.size() > 1) ? 2 * page_width() : page_width())
            : pageset.pages.size() * page_width() + ((pageset.pages.size() - 1) / 2) * layout.distance;
    case MultipageLayout::FIRSTOFF:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? 2 * page_width()
            : pageset.pages.size() * page_width() + (pageset.pages.size() / 2) * layout.distance;
    };
    return 0;
}

// height of complete layout  (in pixel)
unsigned int Engine::layout_height(const MultipageLayout layout) const
{
    if (pageset.pages.empty()) return 0;
    switch (layout.join)
    {
    case MultipageLayout::SINGLE:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? pageset.pages.size() * page_height() + (pageset.pages.size() - 1) * layout.distance
            : page_height();
    case MultipageLayout::DOUBLE:
    case MultipageLayout::JOINED:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? ((pageset.pages.size() + 1) / 2) * page_height() + ((pageset.pages.size() - 1) / 2) * layout.distance
            : page_height();
    case MultipageLayout::FIRSTOFF:
        return (layout.orientation == MultipageLayout::VERTICAL)
            ? ((pageset.pages.size() + 2) / 2) * page_height() + (pageset.pages.size() / 2) * layout.distance
            : page_height();
    };
    return 0;
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

// calculate page-iterator by position
const Engine::Page Engine::select_page(const Position<mpx_t>& pos, const MultipageLayout layout)
{
    unsigned int i = 0;
    Position<mpx_t> pagepos;
    const Position<mpx_t> pagedim(page_width(), page_height());
    for (std::list<Pageset::pPage>::iterator p = pageset.pages.begin(); p != pageset.pages.end(); ++p, ++i)
    {
        pagepos = page_pos(i, layout);
        if (   pos.x >= pagepos.x             && pos.y >= pagepos.y
            && pos.x <  pagepos.x + pagedim.x && pos.y <  pagepos.y + pagedim.y)
                return Page(i, p);
    };
    return Page(--i, --pageset.pages.end());
}

// create cursor (front of first score)
EditCursor Engine::create_cursor() throw(Error, UserCursor::Error)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    EditCursor cursor(*document, pageset, interface, engraver);
    cursor.set_score(document->scores.front());
    cursor.log_set(*this);
    return cursor;
}

// create cursor (front of given score)
EditCursor Engine::create_cursor(Document::Score& score) throw(Error, UserCursor::Error)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    EditCursor cursor(*document, pageset, interface, engraver);
    cursor.set_score(score);
    cursor.log_set(*this);
    return cursor;
}

// create cursor (multi-page position)
EditCursor Engine::create_cursor(const Position<mpx_t>& pos, const MultipageLayout layout) throw(Error, UserCursor::Error)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    EditCursor cursor(*document, pageset, interface, engraver);
    Page page(select_page(pos, layout));
    cursor.set_pos(page.it, ((pos - page_pos(page.idx, layout)) * 1000) / static_cast<int>(press.parameters.scale), viewport);
    cursor.log_set(*this);
    return cursor;
}

// create cursor (on-page position)
EditCursor Engine::create_cursor(const Position<mpx_t>& pos, Page& page) throw(Error, UserCursor::Error)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    EditCursor cursor(*document, pageset, interface, engraver);
    cursor.set_pos(page.it, (pos * 1000) / static_cast<int>(press.parameters.scale), viewport);
    cursor.log_set(*this);
    return cursor;
}

// set cursor (multi-page position)
EditCursor& Engine::set_cursor(EditCursor& cursor, const Position<mpx_t>& pos, const MultipageLayout layout) throw(Error, UserCursor::Error)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    Page page(select_page(pos, layout));
    cursor.set_pos(page.it, ((pos - page_pos(page.idx, layout)) * 1000) / static_cast<int>(press.parameters.scale), viewport);
    return cursor;
}

// set cursor (on-page position)
EditCursor& Engine::set_cursor(EditCursor& cursor, const Position<mpx_t>& pos, Page& page) throw(Error, UserCursor::Error)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    cursor.set_pos(page.it, (pos * 1000) / static_cast<int>(press.parameters.scale), viewport);
    return cursor;
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


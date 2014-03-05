
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2014 Dominik Lehmann
  
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
#include "log.hh"               // Log

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
    object_cur.reset();
}

// engrave document (recalculate cursors)
void Engine::reengrave()
{
    ReengraveInfo info;
    for (CursorMap::iterator cur = cursors.begin(); cur != cursors.end(); ++cur)
        cur->second.setup_reengrave(info);
    if (object_cur.ready() && !object_cur.end())
        object_cur.setup_reengrave(info);
    engraver.engrave(*document, info);
    info.finish();
    if (!info.is_empty())
        log_error("Some cursors could not be updated. (class: Engine)");
}

// engrave single score (recalculate cursors)
void Engine::reengrave(UserCursor& cursor)
{
    ReengraveInfo info;
    CursorMap::iterator cur = cursors.find(&cursor.get_score());
    if (cur != cursors.end() && &cur->second != &cursor)
        cur->second.setup_reengrave(info);
    cursor.setup_reengrave(info);
    if (object_cur.ready() && !object_cur.end())
        object_cur.setup_reengrave(info);
    engraver.engrave(cursor.get_score(), cursor.get_start_page(), document->head_height, info);
    info.finish();
    if (!info.is_empty())
        log_error("Some cursors could not be updated. (class: Engine)");
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

// render object frame, assuming the given page root position
void Engine::render_selection(Renderer& renderer, const ObjectCursor& cursor, const Position<mpx_t>& _page_pos, const Position<mpx_t>& move_offset)
{
    if (!cursor.ready() || cursor.end()) return;
    Position<mpx_t> margin_offset((press.parameters.scale * pageset.page_layout.margin.left) / 1000,
                                  (press.parameters.scale * pageset.page_layout.margin.top) / 1000);
    press.render(renderer, cursor, _page_pos + margin_offset, move_offset);
}

// render object frame, calculating page positions according to the given layout
void Engine::render_selection(Renderer& renderer, const ObjectCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& offset, const Position<mpx_t>& move_offset)
{
    if (!cursor.ready() || cursor.end()) return;
    Position<mpx_t> margin_offset((press.parameters.scale * pageset.page_layout.margin.left) / 1000,
                                  (press.parameters.scale * pageset.page_layout.margin.top) / 1000);
    press.render(renderer, cursor, offset + page_pos(cursor.get_pageno(), layout) + margin_offset, move_offset);
}

// render selected object, assuming the given page root position
void Engine::render_object(Renderer& renderer, const ObjectCursor& cursor, const Position<mpx_t>& _page_pos)
{
    if (!cursor.ready() || cursor.end()) return;
    Position<mpx_t> margin_offset((press.parameters.scale * pageset.page_layout.margin.left) / 1000,
                                  (press.parameters.scale * pageset.page_layout.margin.top) / 1000);
    press.render(renderer, cursor.get_pobject(), cursor.get_staff(), _page_pos + margin_offset);
}

// render selected object, calculating page positions according to the given layout
void Engine::render_object(Renderer& renderer, const ObjectCursor& cursor, const MultipageLayout layout, const Position<mpx_t>& offset)
{
    if (!cursor.ready() || cursor.end()) return;
    Position<mpx_t> margin_offset((press.parameters.scale * pageset.page_layout.margin.left) / 1000,
                                  (press.parameters.scale * pageset.page_layout.margin.top) / 1000);
    press.render(renderer, cursor.get_pobject(), cursor.get_staff(), offset + page_pos(cursor.get_pageno(), layout) + margin_offset);
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
EditCursor& Engine::get_cursor() throw(Error, UserCursor::Error)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    CursorMap::iterator cur = cursors.find(&document->scores.front().score);
    if (cur != cursors.end()) return cur->second;
    cur = cursors.insert(CursorMap::value_type(&document->scores.front().score, EditCursor(*document, pageset, interface, style, viewport))).first;
    cur->second.set_score(document->scores.front());
    cur->second.log_set(*this);
    return cur->second;
}

// create cursor (front of given score)
EditCursor& Engine::get_cursor(Document::Score& score) throw(Error, UserCursor::Error)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    CursorMap::iterator cur = cursors.find(&score.score);
    if (cur != cursors.end()) return cur->second;
    cur = cursors.insert(CursorMap::value_type(&score.score, EditCursor(*document, pageset, interface, style, viewport))).first;
    cur->second.set_score(score);
    cur->second.log_set(*this);
    return cur->second;
}

// create cursor (on-page position)
EditCursor& Engine::get_cursor(Position<mpx_t> pos, const Page& page) throw(Error, UserCursor::Error)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    Document::Score* const score(&select_score(pos, page));
    const std::pair<CursorMap::iterator,bool> ret(cursors.insert(CursorMap::value_type(&score->score, EditCursor(*document, pageset, interface, style, viewport))));
    if (ret.second)
    {
        ret.first->second.log_set(*this);
        ret.first->second.set_score(*score);
    };
    ret.first->second.set_pos(page.it, (pos * 1000) / static_cast<int>(press.parameters.scale), viewport);
    return ret.first->second;
}

// create cursor (multi-page position)
EditCursor& Engine::get_cursor(Position<mpx_t> pos, const MultipageLayout layout) throw(Error, UserCursor::Error)
{
    if (document->scores.empty()) throw Error("Cannot create a cursor for an empty document.");
    if (pageset.pages.empty()) engrave();
    Page page(select_page(pos, layout));
    Document::Score* const score(&select_score(pos, page));
    const std::pair<CursorMap::iterator,bool> ret(cursors.insert(CursorMap::value_type(&score->score, EditCursor(*document, pageset, interface, style, viewport))));
    if (ret.second)
    {
        ret.first->second.log_set(*this);
        ret.first->second.set_score(*score);
    };
    ret.first->second.set_pos(page.it, (pos * 1000) / static_cast<int>(press.parameters.scale), viewport);
    return ret.first->second;
}

// get object by position (on page)
bool Engine::select_object(Position<mpx_t> pos, const Page& page) throw(Error)
{
    ObjectCursor buffer;
    
    // scale pos
    pos = (pos * 1000) / static_cast<int>(press.parameters.scale);
    
    // set on-page root (substract page margin)
    pos.x -= pageset.page_layout.margin.left;
    pos.y -= pageset.page_layout.margin.top;
    
    // search for object on page
    for (Pageset::pPage::AttachableList::iterator a = page.it->attached.begin(); a != page.it->attached.end(); ++a)
    {
        if (!(*a)->object->is(Class::MOVABLE)) continue;
        if ((*a)->contains(pos))
        {
            buffer.set_parent(*document, *page.it);
            buffer.select(*static_cast<const Movable*>((*a)->object));
            if (!buffer.ready() || buffer.end())
                return false;
            object_cur = buffer;
            return true;
        };
    };
    
    // search for object within plates
    Pageset::pPage::const_Iterator pinfo = page.it->get_plate_by_pos(pos);
    if (pinfo == page.it->plates.end())
        pinfo = --page.it->plates.end();
    pos -= pinfo->dimension.position;       // calculate position relative to plate
    
    // search the score
    Document::ScoreList::iterator score = document->scores.begin();
    for (; score != document->scores.end(); ++score)
        if (&score->score == pinfo->score)
            break;
    if (score == document->scores.end()) return false;
    
    // search the object
    for (Plate::LineIt l = pinfo->plate->lines.begin(); l != pinfo->plate->lines.end(); ++l)
    {
        if (!l->contains(pos)) continue;
        for (Plate::VoiceIt v = l->voices.begin(); v != l->voices.end(); ++v)
        {
            for (Plate::NoteIt n = v->notes.begin(); n != v->notes.end() && !n->at_end(); ++n)
            {
                if (n->is_inserted()) continue;
                if (!n->get_note().is(Class::VISIBLEOBJECT)) continue;
                for (Plate::pNote::AttachableList::iterator a = n->attached.begin(); a != n->attached.end(); ++a)
                {
                    if (!(*a)->object->is(Class::MOVABLE)) continue;
                    if ((*a)->contains(pos))
                    {
                        // (casts away const, but not unexpected, since this object got a non-const reference to the document)
                        if (buffer.set_parent(const_cast<StaffObject&>(n->get_note()).get_visible(), *n, *v, *l, pinfo->pageno))
                        {
                            if (buffer.select(*static_cast<const Movable*>((*a)->object)))
                            {
                                if (!buffer.ready() || buffer.end())
                                    return false;
                                object_cur = buffer;
                                return true;
                            };
                        };
                    };
                };
            };
        };
    };
    
    // no object found
    return false;
}

// move object by "offset"
void Engine::move_object(const ObjectCursor& cursor, const Position<mpx_t> offset)
{
    // apply offset to on-plate data (no reengave neccessary)
    const Position<mpx_t> scaled_offset(static_cast<mpx_t>((1000.0 * offset.x) / press.parameters.scale),
                                        static_cast<mpx_t>((1000.0 * offset.y) / press.parameters.scale));
    cursor.get_pobject().absolutePos += scaled_offset;
    if (cursor.get_pobject().is_durable())
        static_cast<Plate::pDurable&>(cursor.get_pobject()).endPos += scaled_offset;
    cursor.get_pobject().gphBox.pos += scaled_offset;
    
    // calculate offset for score data
    Movable& object(cursor.get_object());
    const Position<> real_offset(
        (object.typeX == Movable::PAGE || object.typeX == Movable::LINE)
            ? viewport.pxtoum_h(offset.x)
            : (1000 * offset.x) / viewport.umtopx_h(cursor.get_staff().head_height),
        (object.typeY == Movable::PAGE || object.typeY == Movable::LINE)
            ? viewport.pxtoum_v(offset.y)
            : (1000 * offset.y) / viewport.umtopx_v(cursor.get_staff().head_height)
    );
    
    // apply offset to score data
    object.position += real_offset;
    if (object.is(Class::DURABLE)) static_cast<Durable&>(object).end_position += real_offset;
    
    // update the line's boundary box (casts away const, but not unexpected, since this object got a non-const reference to the pageset)
    const_cast<Plate::pLine&>(cursor.get_line()).calculate_gphBox();
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


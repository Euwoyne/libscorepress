
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

#include <cstdlib>
#include <set>          // std::set

#include "press.hh"
#include "undefined.hh" // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                        // this number is interpreted as an undefined value
using namespace ScorePress;

inline int _round(const double d) {return static_cast<mpx_t>(d + 0.5);}


//
//     class PressState
//    ==================
//
// The State structure is the set of data passed to the object class on
// rendering the object by the press. It contains all necessary parameters
// and positioning information needed for successful rendering.
// (Not counting those provided by the on-plate object.)
//

// constructor
PressState::PressState(const PressParam& p, const StyleParam& s, const ViewportParam& v)
    : parameters(p), style(&s), viewport(v), head_height(0), stem_width(0) {}


//     class Press
//    =============
//
// The press-class exports a method, which draws a score, with the help of the
// engraver-provided Plate instance and a renderer instance.
//

// exception classes
Press::Error::Error(const std::string& msg) : ScorePress::Error(msg) {}
Press::InvalidRendererException::InvalidRendererException() : Error("Unable to draw with non-ready renderer.") {}

// rendering method (for on-plate note objects)
void Press::render(Renderer& renderer, const Plate::pNote& note)
{
    // draw eov boundaries
    if (note.at_end())
    {
        if (parameters.draw_eov)
            draw_boundaries(renderer, note, parameters.eov_color, state.offset);
        return;
    };
    
    // draw boundaries
    if (parameters.draw_notebounds)
        draw_boundaries(renderer, note, note.is_virtual() ? parameters.virtualbounds_color : parameters.notebounds_color, state.offset);
    
    // render object
    note.get_note().render(renderer, note, state);
    
    // render ties
    for (std::list<Plate::pNote::Tie>::const_iterator i = note.ties.begin(); i != note.ties.end(); ++i)
    {
        renderer.bezier_slur((scale(i->pos1.x)     + state.offset.x) / 1000.0, (scale(i->pos1.y)     + state.offset.y) / 1000.0,
                             (scale(i->control1.x) + state.offset.x) / 1000.0, (scale(i->control1.y) + state.offset.y) / 1000.0,
                             (scale(i->control2.x) + state.offset.x) / 1000.0, (scale(i->control2.y) + state.offset.y) / 1000.0,
                             (scale(i->pos2.x)     + state.offset.x) / 1000.0, (scale(i->pos2.y)     + state.offset.y) / 1000.0,
                             0,
                             scale(state.viewport.umtopx_h(state.style->tie_thickness)) / 1000.0);
    };
    
    // render attachables
    for (Plate::pNote::AttachableList::const_iterator i = note.attachables.begin(); i != note.attachables.end(); ++i)
    {
        (*i)->object->render(renderer, **i, state);
        if (parameters.draw_attachbounds) draw_boundaries(renderer, **i, parameters.attachbounds_color, state.offset);
    };
}

// render the staff-lines for a plate
void Press::render_staff(Renderer& renderer, const Plate& plate, const Position<mpx_t> offset)
{
    std::set<const Staff*> staves;      // set of already drawn staves
    renderer.set_color(0, 0, 0, 255);   // set color for all lines
    
    // iterate through the lines
    for (std::list<Plate::pLine>::const_iterator line = plate.lines.begin(); line != plate.lines.end(); ++line)
    {
        // initialize max-/min-pos (for front line rendering)
        mpx_t max_pos = line->voices.front().basePos.y;
        mpx_t min_pos = line->voices.front().basePos.y;
        
        // iterate through the on-plate voices
        for (std::list<Plate::pVoice>::const_iterator pvoice = line->voices.begin(); pvoice != line->voices.end(); ++pvoice)
        {
            // render the brace
            if (pvoice->brace.sprite.ready())
            {
                renderer.draw_sprite(pvoice->brace.sprite,
                                     (offset.x + scale(pvoice->brace.gphBox.pos.x)) / 1000.0,
                                     (offset.y + scale(pvoice->brace.gphBox.pos.y)) / 1000.0,
                                     scale(pvoice->brace.gphBox.width) / (1000.0 * renderer.get_sprites()[pvoice->brace.sprite].width),
                                     scale(pvoice->brace.gphBox.height) / (1000.0 * renderer.get_sprites()[pvoice->brace.sprite].height));
                
                if (parameters.draw_attachbounds)
                {
                    draw_boundaries(renderer, pvoice->brace, parameters.attachbounds_color, offset);
                    renderer.set_color(0, 0, 0, 255);   // reset color
                };
            };
            
            // render the bracket
            if (pvoice->bracket.sprite.ready())
            {
                renderer.draw_sprite(pvoice->bracket.sprite,
                                     (offset.x + scale(pvoice->bracket.gphBox.pos.x)) / 1000.0,
                                     (offset.y + scale(pvoice->bracket.gphBox.pos.y)) / 1000.0,
                                     scale(pvoice->bracket.gphBox.width) / (1000.0 * renderer.get_sprites()[pvoice->bracket.sprite].width),
                                     scale(pvoice->bracket.gphBox.width) / (1000.0 * renderer.get_sprites()[pvoice->bracket.sprite].width));
                renderer.draw_sprite(pvoice->bracket.sprite,
                                     (offset.x + scale(pvoice->bracket.gphBox.pos.x)) / 1000.0,
                                     (offset.y + scale(pvoice->bracket.line_end.y)) / 1000.0,
                                     scale(pvoice->bracket.gphBox.width) / (1000.0 * renderer.get_sprites()[pvoice->bracket.sprite].width),
                                    -scale(pvoice->bracket.gphBox.width) / (1000.0 * renderer.get_sprites()[pvoice->bracket.sprite].width));
                renderer.set_line_width(parameters.scale / 1000.0);                         // set line width
                renderer.move_to((offset.x + scale(pvoice->bracket.line_base.x)) / 1000.0,
                                 (offset.y + scale(pvoice->bracket.line_base.y)) / 1000.0);
                renderer.line_to((offset.x + scale(pvoice->bracket.line_base.x)) / 1000.0,
                                 (offset.y + scale(pvoice->bracket.line_end.y)) / 1000.0);
                renderer.line_to((offset.x + scale(pvoice->bracket.line_end.x)) / 1000.0,
                                 (offset.y + scale(pvoice->bracket.line_end.y)) / 1000.0);
                renderer.line_to((offset.x + scale(pvoice->bracket.line_end.x)) / 1000.0,
                                 (offset.y + scale(pvoice->bracket.line_base.y)) / 1000.0);
                renderer.fill();
                if (parameters.draw_attachbounds)
                {
                    draw_boundaries(renderer, pvoice->bracket, parameters.attachbounds_color, offset);
                    renderer.set_color(0, 0, 0, 255);   // reset color
                };
            };
            
            // check, if we already had this staff
            if (staves.find(&pvoice->begin.staff()) != staves.end()) continue;
            staves.insert(&pvoice->begin.staff());
            
            // set line width
            renderer.set_line_width(
                scale(state.viewport.umtopx_h(
                    (pvoice->begin.staff().style) ?
                        pvoice->begin.staff().style->line_thickness :
                        default_style.line_thickness
                ) / 1000.0));
            
            // render the staff lines
            for (size_t i = 0; i < pvoice->begin.staff().line_count; i++)   // for each line of the staff
            {
                // prepare the line
                renderer.move_to(
                    (offset.x + scale(line->basePos.x)) / 1000.0,
                    (offset.y + scale(pvoice->basePos.y +
                                    i * state.viewport.umtopx_v(pvoice->begin.staff().head_height))
                    ) / 1000.0
                );
                
                renderer.line_to(
                        (offset.x + scale((line->line_end <= 0) ? line->basePos.x + 1e6 : line->line_end)
                        ) / 1000.0,
                        (offset.y + scale(pvoice->basePos.y +
                                    i * state.viewport.umtopx_v(pvoice->begin.staff().head_height))
                        ) / 1000.0
                );
            };
            renderer.stroke();  // render all lines
            
            // set max-/min-pos (for front line rendering)
            if (min_pos > pvoice->basePos.y) min_pos = pvoice->basePos.y;
            if (max_pos < pvoice->basePos.y + state.viewport.umtopx_v(pvoice->begin.staff().head_height * (pvoice->begin.staff().line_count - 1)))
                max_pos = pvoice->basePos.y + state.viewport.umtopx_v(pvoice->begin.staff().head_height * (pvoice->begin.staff().line_count - 1));
        };
        
        staves.clear(); // erase remembered staves
        
        // render the front line
        renderer.set_line_width(scale(state.viewport.umtopx_h(default_style.bar_thickness)) / 1000.0);
        renderer.move_to((offset.x + scale(line->basePos.x)) / 1000.0,
                         (offset.y + scale(min_pos)) / 1000.0);
        renderer.line_to((offset.x + scale(line->basePos.x)) / 1000.0,
                         (offset.y + scale(max_pos)) / 1000.0);
        renderer.stroke();
        
        // render the line's boundary box
        if (parameters.draw_linebounds)
        {
            draw_boundaries(renderer, *line, parameters.linebounds_color, offset);
            renderer.set_color(0, 0, 0, 255);   // reset color
        };
    };
}

// constructor (providing viewport parameters)
Press::Press(const StyleParam& style, const ViewportParam& viewport) : state(parameters, style, viewport),
                                                                       default_style(style) {}

// draw the boundary box of a graphical object
void Press::draw_boundaries(Renderer& renderer, const Plate::pGraphical& object, unsigned int color, const Position<mpx_t> offset) throw(InvalidRendererException)
{
    // check if the renderer is ready
    if (!renderer.ready()) throw InvalidRendererException();
    
    // render the box
    renderer.set_line_width(1.0);
    renderer.set_color(static_cast<unsigned char>(color & 0xFF),
                       static_cast<unsigned char>((color >> 8) & 0xFF),
                       static_cast<unsigned char>((color >> 16) & 0xFF),
                       static_cast<unsigned char>((color >> 24) & 0xFF));
    renderer.move_to((scale(object.gphBox.pos.x) + offset.x) / 1000.0, (scale(object.gphBox.pos.y) + offset.y) / 1000.0);
    renderer.line_to((scale(object.gphBox.pos.x + object.gphBox.width) + offset.x) / 1000.0, (scale(object.gphBox.pos.y) + offset.y) / 1000.0);
    renderer.line_to((scale(object.gphBox.pos.x + object.gphBox.width) + offset.x) / 1000.0, (scale(object.gphBox.pos.y + object.gphBox.height) + offset.y) / 1000.0);
    renderer.line_to((scale(object.gphBox.pos.x) + offset.x) / 1000.0, (scale(object.gphBox.pos.y + object.gphBox.height) + offset.y) / 1000.0);
    renderer.line_to((scale(object.gphBox.pos.x) + offset.x) / 1000.0, (scale(object.gphBox.pos.y) + offset.y) / 1000.0);
    renderer.stroke();
}

// draw the boundary box of a graphical object (customized color)
void Press::draw_boundaries(Renderer& renderer, const Plate::pGraphical& object, const Color& color, const Position<mpx_t> offset) throw(InvalidRendererException)
{
    // check if the renderer is ready
    if (!renderer.ready()) throw InvalidRendererException();
    
    // render the box
    renderer.set_line_width(1.0);
    renderer.set_color(color.r, color.g, color.b, color.a);
    renderer.move_to((scale(object.gphBox.pos.x) + offset.x) / 1000.0, (scale(object.gphBox.pos.y) + offset.y) / 1000.0);
    renderer.line_to((scale(object.gphBox.pos.x + object.gphBox.width) + offset.x) / 1000.0, (scale(object.gphBox.pos.y) + offset.y) / 1000.0);
    renderer.line_to((scale(object.gphBox.pos.x + object.gphBox.width) + offset.x) / 1000.0, (scale(object.gphBox.pos.y + object.gphBox.height) + offset.y) / 1000.0);
    renderer.line_to((scale(object.gphBox.pos.x) + offset.x) / 1000.0, (scale(object.gphBox.pos.y + object.gphBox.height) + offset.y) / 1000.0);
    renderer.line_to((scale(object.gphBox.pos.x) + offset.x) / 1000.0, (scale(object.gphBox.pos.y) + offset.y) / 1000.0);
    renderer.stroke();
}

// draw a little red cross
void Press::draw_cross(Renderer& renderer, const Position<mpx_t>& pos, const Position<mpx_t> offset)
{
    renderer.set_line_width(1.0);
    renderer.set_color(static_cast<unsigned char>(parameters.attachbounds_color & 0xFF),
                       static_cast<unsigned char>((parameters.attachbounds_color >> 8) & 0xFF),
                       static_cast<unsigned char>((parameters.attachbounds_color >> 16) & 0xFF),
                       static_cast<unsigned char>((parameters.attachbounds_color >> 24) & 0xFF));
    renderer.move_to((scale(pos.x) + offset.x) / 1000.0 - 3.0, (scale(pos.y) + offset.y) / 1000.0 - 3.0);
    renderer.line_to((scale(pos.x) + offset.x) / 1000.0 + 3.0, (scale(pos.y) + offset.y) / 1000.0 + 3.0);
    renderer.move_to((scale(pos.x) + offset.x) / 1000.0 - 3.0, (scale(pos.y) + offset.y) / 1000.0 + 3.0);
    renderer.line_to((scale(pos.x) + offset.x) / 1000.0 + 3.0, (scale(pos.y) + offset.y) / 1000.0 - 3.0);
    renderer.stroke();
}

// render a plate through the given renderer
void Press::render(Renderer& renderer, const Plate& plate, const Position<mpx_t> offset) throw(InvalidRendererException)
{
    // check if the renderer is ready
    if (!renderer.ready()) throw InvalidRendererException();
    
    // set state
    state.offset = offset;
    
    // render the lines
    render_staff(renderer, plate, offset);
    
    size_t l = 0;
    size_t v = 0;
    // iterate through the lines
    for (std::list<Plate::pLine>::const_iterator line = plate.lines.begin(); line != plate.lines.end(); ++line)
    {
        ++l;
        v = 0;
        // iterate through the on-plate voices
        for (std::list<Plate::pVoice>::const_iterator pvoice = line->voices.begin(); pvoice != line->voices.end(); ++pvoice)
        {
            ++v;
            
            // setup style parameters
            state.set_style((!!pvoice->begin.staff().style) ? *pvoice->begin.staff().style : default_style);
            state.head_height = state.viewport.umtopx_v(pvoice->begin.staff().head_height);
            state.stem_width  = state.viewport.umtopx_h(state.style->stem_width);
            
            // iterate the voice
            for (std::list<Plate::pNote>::const_iterator it = pvoice->notes.begin(); it != pvoice->notes.end(); ++it)
            {
                render(renderer, *it);
            };
        };
    };
    
    // reset style
    state.set_style(default_style);
}

// render a page through the given renderer
void Press::render(Renderer& renderer, const PageSet::pPage& page, const PageSet& pageset, const Position<mpx_t> offset) throw(InvalidRendererException)
{
    // render scores
    for (std::list<PageSet::PlateInfo>::const_iterator i = page.plates.begin(); i != page.plates.end(); ++i)
    {
        render(renderer, i->plate, Position<mpx_t>(_round(scale(i->dimension.position.x)) + offset.x,
                                                   _round(scale(i->dimension.position.y)) + offset.y));
    };
    
    // set state
    state.offset = offset;
    state.head_height = pageset.head_height;
    
    // render attachables
    for (std::list<Plate::pAttachable>::const_iterator i = page.attachables.begin(); i != page.attachables.end(); ++i)
    {
        i->object->render(renderer, *i, state);
        if (parameters.draw_attachbounds) draw_boundaries(renderer, *i, parameters.attachbounds_color, offset);
    };
}

// render page decoration
void Press::render_decor(Renderer& renderer, const PageSet& pageset, const Position<mpx_t> offset) throw(InvalidRendererException)
{
    // draw shadow
    if (parameters.draw_shadow)
    {
    renderer.set_color(static_cast<unsigned char>(parameters.shadow_color & 0xFF),
                       static_cast<unsigned char>((parameters.shadow_color >> 8) & 0xFF),
                       static_cast<unsigned char>((parameters.shadow_color >> 16) & 0xFF),
                       static_cast<unsigned char>((parameters.shadow_color >> 24) & 0xFF));
    renderer.move_to((offset.x + parameters.shadow_offset) / 1000.0,
                     (offset.y + parameters.shadow_offset) / 1000.0);
    renderer.line_to((offset.x + parameters.shadow_offset + scale(pageset.page_layout.width)) / 1000.0,
                     (offset.y + parameters.shadow_offset) / 1000.0);
    renderer.line_to((offset.x + parameters.shadow_offset + scale(pageset.page_layout.width)) / 1000.0,
                     (offset.y + parameters.shadow_offset + scale(pageset.page_layout.height)) / 1000.0);
    renderer.line_to((offset.x + parameters.shadow_offset) / 1000.0,
                     (offset.y + parameters.shadow_offset + scale(pageset.page_layout.height)) / 1000.0);
    renderer.fill();
    };
    
    // draw page
    renderer.set_color(255,255,255,255);
    renderer.move_to(offset.x / 1000.0, offset.y / 1000.0);
    renderer.line_to((offset.x + scale(pageset.page_layout.width)) / 1000.0, offset.y / 1000.0);
    renderer.line_to((offset.x + scale(pageset.page_layout.width)) / 1000.0, (offset.y + scale(pageset.page_layout.height)) / 1000.0);
    renderer.line_to(offset.x / 1000.0, (offset.y + scale(pageset.page_layout.height)) / 1000.0);
    renderer.fill();
    
    // draw margin
    renderer.set_color(0,0,0,255);
    renderer.set_line_width(1.0);
    renderer.move_to(offset.x / 1000.0, offset.y / 1000.0);
    renderer.line_to((offset.x + scale(pageset.page_layout.width)) / 1000.0, offset.y / 1000.0);
    renderer.line_to((offset.x + scale(pageset.page_layout.width)) / 1000.0, (offset.y + scale(pageset.page_layout.height)) / 1000.0);
    renderer.line_to(offset.x / 1000.0, (offset.y + scale(pageset.page_layout.height)) / 1000.0);
    renderer.close();
    renderer.stroke();
    
    // draw page margins
    if (parameters.draw_margin)
    {
    renderer.set_color(static_cast<unsigned char>(parameters.margin_color & 0xFF),
                       static_cast<unsigned char>((parameters.margin_color >> 8) & 0xFF),
                       static_cast<unsigned char>((parameters.margin_color >> 16) & 0xFF),
                       static_cast<unsigned char>((parameters.margin_color >> 24) & 0xFF));
    renderer.move_to((offset.x + scale(pageset.page_layout.margin.left)) / 1000.0, (offset.y + scale(pageset.page_layout.margin.top)) / 1000.0);
    renderer.line_to((offset.x + scale(pageset.page_layout.width - pageset.page_layout.margin.right)) / 1000.0, (offset.y + scale(pageset.page_layout.margin.top)) / 1000.0);
    renderer.line_to((offset.x + scale(pageset.page_layout.width - pageset.page_layout.margin.right)) / 1000.0, (offset.y + scale(pageset.page_layout.height - pageset.page_layout.margin.bottom)) / 1000.0);
    renderer.line_to((offset.x + scale(pageset.page_layout.margin.left)) / 1000.0, (offset.y + scale(pageset.page_layout.height - pageset.page_layout.margin.bottom)) / 1000.0);
    renderer.close();
    renderer.stroke();
    }
}

// render a cursor through the given renderer
void Press::render(Renderer& renderer, const UserCursor& cursor, const Position<mpx_t> offset) throw (InvalidRendererException, UserCursor::NotValidException)
{
    // get horizontal position
    mpx_t x = cursor.graphical_x();
    x -= _round(state.viewport.umtopx_h(parameters.cursor_distance));
    
    // get vertical position and height
    mpx_t y = cursor.graphical_y(state.viewport);
    mpx_t h = cursor.graphical_height(state.viewport);
    
    // render the cursor
    if (renderer.has_rect_invert())
    {
        renderer.rect_invert(
            (scale(x) + offset.x) / 1000.0 - parameters.cursor_width / 2000.0, (scale(y) + offset.y) / 1000.0,
            (scale(x) + offset.x) / 1000.0 + parameters.cursor_width / 2000.0, (scale(y + h) + offset.y) / 1000.0);
    }
    else
    {
        renderer.set_line_width(parameters.cursor_width / 1000.0);
        renderer.move_to((scale(x) + offset.x) / 1000.0, (scale(y) + offset.y) / 1000.0);
        renderer.line_to((scale(x) + offset.x) / 1000.0, (scale(y + h) + offset.y) / 1000.0);
        renderer.stroke();
    };
}


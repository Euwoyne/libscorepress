
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

#include <set>          // std::set

#include "press.hh"
#include "log.hh"       // Log
#include "undefined.hh" // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                        // this number is interpreted as an undefined value
using namespace ScorePress;

inline int _round(const double d) {return static_cast<mpx_t>(d + 0.5);}


//     class Press
//    =============
//
// The press-class exports a method, which draws a score, with the help of the
// engraver-provided Plate instance and a renderer instance.
//

// render a sprite transforming the position with "parameters.scale"
void Press::draw(Renderer& renderer, const SpriteId& sprite, const Position<mpx_t> pos, const Position<mpx_t> offset, const double sprite_scale)
{
    renderer.draw_sprite(sprite,
                         (scale(pos.x) + offset.x) / 1000.0,
                         (scale(pos.y) + offset.y) / 1000.0,
                         scale(sprite_scale),
                         scale(sprite_scale));
}

// rendering method (for attachable objects)
void Press::render(Renderer& renderer, const Plate::pAttachable* object, const Position<mpx_t> offset, const bool stemup, const mpx_t head_height, const mpx_t stem_width)
{
    if (object->object->is(Class::ACCIDENTAL))
    {
        const Appearance& appearance = static_cast<const Accidental*>(object->object)->appearance;
        if (!appearance.visible) return;
        set_color(renderer, appearance.color);  // set color
        draw(renderer,                          // render sprite
             object->sprite,
             object->absolutePos, offset,
             (head_height * appearance.scale)
                          / (1.0e6 * renderer.get_sprites().head_height(object->sprite)));
    }
    else if (object->object->is(Class::ARTICULATION))
    {
        const Appearance& appearance = static_cast<const Articulation*>(object->object)->appearance;
        if (!appearance.visible) return;
        set_color(renderer, appearance.color);  // set color
        draw(renderer,                          // render sprite
             object->sprite,
             object->absolutePos, offset,
             ((stemup ^ static_cast<const Articulation*>(object->object)->far ? -1.0 : 1.0)
                      * head_height * appearance.scale)
                                    / (1.0e6 * renderer.get_sprites().head_height(object->sprite)));
    }
    else if (object->object->is(Class::TEXTAREA))
    {
        const TextArea& textarea = *static_cast<const TextArea*>(object->object);
        const Appearance& appearance = textarea.appearance;
        if (!appearance.visible) return;
        
        // prepare textbox
        renderer.move_to((scale(object->absolutePos.x) + offset.x) / 1000.0,
                         (scale(object->absolutePos.y) + offset.y) / 1000.0);
        renderer.set_text_width(scale(object->gphBox.width) / 1000.0);
        
        // render text
        for (std::list<Paragraph>::const_iterator p = textarea.text.begin(); p != textarea.text.end(); ++p)
        {   // for each paragraph
            // set the text align
            switch (p->align)
            {
            case Paragraph::LEFT:   renderer.set_text_align(Renderer::ALIGN_LEFT);   break;
            case Paragraph::CENTER: renderer.set_text_align(Renderer::ALIGN_CENTER); break;
            case Paragraph::RIGHT:  renderer.set_text_align(Renderer::ALIGN_RIGHT);  break;
            };
            renderer.set_text_justify(p->justify);  // and justification
            
            // prepare all chunks of plain-text
            for (std::list<PlainText>::const_iterator i = p->text.begin(); i != p->text.end(); ++i)
            {
                // setup the font
                renderer.set_font_family(i->font.family);
                renderer.set_font_size(scale(i->font.size * appearance.scale) / 1000.0);
                renderer.set_font_bold(i->font.bold);
                renderer.set_font_italic(i->font.italic);
                renderer.set_font_underline(i->font.underline);
                renderer.set_font_color(i->font.color.r, i->font.color.g, i->font.color.b);
                
                // push the chunk of text
                renderer.add_text(i->text);
            };
            
            // render the paragraph
            renderer.render_text();
        };
    }
    else if (object->object->is(Class::SLUR))
    {
        const Appearance& appearance = static_cast<const Slur*>(object->object)->appearance;
        if (!appearance.visible) return;
        
        // cast slur object
        const Slur& slur = *static_cast<const Slur*>(object->object);
        set_color(renderer, appearance.color);  // set color
        
        // calculate base node positions
        Position<double> pos1, pos2, ctrl1, ctrl2;
        pos1.x = object->absolutePos.x / 1000.0;
        pos1.y = object->absolutePos.y / 1000.0;
        pos2.x = static_cast<const Plate::pDurable*>(object)->endPos.x / 1000.0;
        pos2.y = static_cast<const Plate::pDurable*>(object)->endPos.y / 1000.0;
        
        // calculate control-point positions
        if (slur.typeX == Movable::PAGE || slur.typeX == Movable::LINE)
        {
            // coordinates given in millipixel
            ctrl1.x = pos1.x + viewport->umtopx_h(slur.control1.x) / 1000.0;
            ctrl2.x = pos2.x + viewport->umtopx_h(slur.control2.x) / 1000.0;
        }
        else
        {
            // coordinates given in promille of head-height
            ctrl1.x = pos1.x + (static_cast<int>(head_height) * slur.control1.x) / 1.0e6;
            ctrl2.x = pos2.x + (static_cast<int>(head_height) * slur.control2.x) / 1.0e6;
        };
        
        if (slur.typeY == Movable::PAGE || slur.typeY == Movable::LINE)
        {
            // coordinates given in millipixel
            ctrl1.y = pos1.y + viewport->umtopx_v(slur.control1.y) / 1000.0;
            ctrl2.y = pos2.y + viewport->umtopx_v(slur.control2.y) / 1000.0;
        }
        else
        {
            // coordinates given in promille of head-height
            ctrl1.y = pos1.y + (static_cast<int>(head_height) * slur.control1.y) / 1.0e6;
            ctrl2.y = pos2.y + (static_cast<int>(head_height) * slur.control2.y) / 1.0e6;
        };
        
        // apply scaling
        ctrl1 = pos1 + (static_cast<double>(appearance.scale) * (ctrl1 - pos1)) / 1000.0;
        ctrl2 = pos1 + (static_cast<double>(appearance.scale) * (ctrl2 - pos1)) / 1000.0;
        
        // apply rendering scale and offset
        pos1.x  = scale(pos1.x) + offset.x / 1000.0;  pos1.y  = scale(pos1.y) + offset.y / 1000.0;
        pos2.x  = scale(pos2.x) + offset.x / 1000.0;  pos2.y  = scale(pos2.y) + offset.y / 1000.0;
        ctrl1.x = scale(ctrl1.x) + offset.x / 1000.0; ctrl1.y = scale(ctrl1.y) + offset.y / 1000.0;
        ctrl2.x = scale(ctrl2.x) + offset.x / 1000.0; ctrl2.y = scale(ctrl2.y) + offset.y / 1000.0;
        
        // render slur
        renderer.bezier_slur(pos1.x, pos1.y, ctrl1.x, ctrl1.y, ctrl2.x, ctrl2.y, pos2.x, pos2.y,
                             (slur.thickness1 * stem_width) / 1.0e6 * (parameters.scale / 1000.0),
                             (slur.thickness2 * stem_width) / 1.0e6 * (parameters.scale / 1000.0));
    }
    else if (object->object->is(Class::HAIRPIN))
    {
        const Appearance& appearance = static_cast<const Hairpin*>(object->object)->appearance;
        if (!appearance.visible) return;
        set_color(renderer, appearance.color);  // set color
        
        // calculate positions (in mpx)
        Position<double> posPoint, posStart, posEnd;
        
        if (static_cast<const Hairpin*>(object->object)->crescendo)
        {
            posPoint.x = object->absolutePos.x;
            posPoint.y = object->absolutePos.y;
            posStart.x = static_cast<const Plate::pDurable*>(object)->endPos.x;
            posStart.y = static_cast<const Plate::pDurable*>(object)->endPos.y + 500.0
                         - (static_cast<const Hairpin*>(object->object)->height * head_height) / 2000.0 * (appearance.scale / 1000.0);
            posEnd.x = static_cast<const Plate::pDurable*>(object)->endPos.x;
            posEnd.y = static_cast<const Plate::pDurable*>(object)->endPos.y - 500.0
                       + (static_cast<const Hairpin*>(object->object)->height * head_height) / 2000.0 * (appearance.scale / 1000.0);
        }
        else
        {
            posPoint.x = static_cast<const Plate::pDurable*>(object)->endPos.x;
            posPoint.y = static_cast<const Plate::pDurable*>(object)->endPos.y;
            posStart.x = object->absolutePos.x;
            posStart.y = object->absolutePos.y
                         - (static_cast<const Hairpin*>(object->object)->height * head_height * appearance.scale) / 1.0e6;
            posEnd.x = object->absolutePos.x;
            posEnd.y = object->absolutePos.y
                       + (static_cast<const Hairpin*>(object->object)->height * head_height * appearance.scale) / 1.0e6;
        };
        
        // set line-width
        renderer.set_line_width(scale(static_cast<const Hairpin*>(object->object)->thickness * stem_width) / 1.0e6);
        
        // render hairpin symbol
        renderer.move_to((scale(posStart.x) + offset.x) / 1000.0, (scale(posStart.y) + offset.y) / 1000.0);
        renderer.line_to((scale(posPoint.x) + offset.x) / 1000.0, (scale(posPoint.y) + offset.y) / 1000.0);
        renderer.line_to((scale(posEnd.x) + offset.x) / 1000.0,   (scale(posEnd.y) + offset.y) / 1000.0);
        renderer.stroke();
    }
    else if (object->object->is(Class::CUSTOMSYMBOL))
    {
        const Appearance& appearance = static_cast<const CustomSymbol*>(object->object)->appearance;
        if (!appearance.visible) return;
        set_color(renderer, appearance.color);  // set color
        draw(renderer,                          // render sprite
             object->sprite,
             object->absolutePos, offset,
             (head_height * appearance.scale)
                      / (1000.0 * renderer.get_sprites().head_height(object->sprite)));
    }
    else {Log::warn("Unable to render attachable-object. (class: Press)");};
}

// rendering method (for on-plate note objects)
void Press::render(Renderer& renderer, const Plate::pNote& note, const Position<mpx_t> offset, const mpx_t head_height, const mpx_t stem_width)
{
    const StaffObject& object = note.get_note();
    if (!object.is(Class::VISIBLEOBJECT)) return;
    const VisibleObject& visible = object.get_visible();
    
    // set color
    set_color(renderer, visible.appearance.color);
    
    // calculate scale
    const double sprite_scale = head_height / (1000.0 * renderer.get_sprites().head_height(note.sprite));
    
    // check object type
    if (object.is(Class::CHORD))
    {
        // render heads (count from 1, because the first position is the offset of the whole chord)
        HeadList::const_iterator h = static_cast<const Chord*>(&object)->heads.begin();
        for (std::list< Position<mpx_t> >::const_iterator p = ++note.absolutePos.begin(); p != note.absolutePos.end(); ++p)
        {
            set_color(renderer, (*h)->appearance.color);
            draw(renderer,
                 note.sprite,
                 *p, offset,
                 (sprite_scale * (*h)->appearance.scale * visible.appearance.scale) / 1.0e6);
            ++h;
        };
        
        // reset color
        set_color(renderer, visible.appearance.color);
        
        // render dots
        for (std::list< Position<mpx_t> >::const_iterator p = note.dotPos.begin(); p != note.dotPos.end(); ++p)
        {
            draw(renderer,
                 SpriteId(note.sprite.setid,
                           (renderer.get_sprites()[note.sprite.setid].dot != UNDEFINED) ?
                                  renderer.get_sprites()[note.sprite.setid].dot :
                                  renderer.get_sprites()[note.sprite.setid].undefined_symbol),
                 *p, offset,
                 (sprite_scale * visible.appearance.scale) / 1000);
        };
        
        // render stem
        set_color(renderer, static_cast<const Chord*>(&object)->stem_color);
        renderer.set_line_width(scale(stem_width) / 1000.0);    // set line width
        renderer.move_to((scale(note.stem.x) + offset.x) / 1000.0, (scale(note.stem.base) + offset.y) / 1000.0);
        renderer.line_to((scale(note.stem.x) + offset.x) / 1000.0, (scale(note.stem.top) + offset.y) / 1000.0);
        renderer.stroke();
        
        // render beam
        render_beam(renderer, note, *static_cast<const Chord*>(&object), offset, head_height, stem_width);
        
        // render ledger lines
        renderer.set_line_width(scale(viewport->umtopx_v(style->ledger_thickness)) / 1000.0);
        for (Plate::pNote::LedgerLineList::const_iterator i = note.ledgers.begin(); i != note.ledgers.end(); ++i)
        {
            for (size_t l = 0; l <= i->count; ++l)
            {
                if (i->below)
                {
                    renderer.move_to((scale(i->basepos.x) + offset.x) / 1000.0,
                                     (scale(i->basepos.y + l * head_height) + offset.y) / 1000.0);
                    renderer.line_to((scale(i->basepos.x + i->length) + offset.x) / 1000.0,
                                     (scale(i->basepos.y + l * head_height) + offset.y) / 1000.0);
                }
                else
                {
                    renderer.move_to((scale(i->basepos.x) + offset.x) / 1000.0,
                                     (scale(i->basepos.y - l * head_height) + offset.y) / 1000.0);
                    renderer.line_to((scale(i->basepos.x + i->length) + offset.x) / 1000.0,
                                     (scale(i->basepos.y - l * head_height) + offset.y) / 1000.0);
                };
            };
            renderer.stroke();
        };
    }
    else if (object.is(Class::REST) || object.is(Class::CLEF) || object.is(Class::CUSTOMTIMESIG))
    {
        // if we have a short rest, that is flagged
        if (object.is(Class::NOTEOBJECT) && static_cast<const NoteObject*>(&object)->val.exp < VALUE_BASE - 2)
        {
            // draw all flags
            for (std::list< Position<mpx_t> >::const_iterator p = note.absolutePos.begin(); p != --note.absolutePos.end(); ++p)
            {
                draw(renderer, note.sprite, *p, offset, (sprite_scale * visible.appearance.scale) / 1000.0);
            };
            
            // draw base piece
            if (renderer.get_sprites()[note.sprite.setid].flags_base != UNDEFINED)
            {
                draw(renderer,
                     SpriteId(note.sprite.setid,
                               renderer.get_sprites()[note.sprite.setid].flags_base),
                     note.absolutePos.back(),
                     offset,
                     (sprite_scale * visible.appearance.scale) / 1000);
            };
            
            // draw stem
            const double app_scale = scale(sprite_scale * visible.appearance.scale) / 1000.0;
            SpriteInfo& sprite = renderer.get_sprites()[note.sprite];
            renderer.set_line_width(stem_width * sprite_scale);         // set line width
            renderer.move_to((scale(note.absolutePos.front().x) + offset.x) / 1000.0 + sprite.real["stem.top.x1"]    * app_scale,
                             (scale(note.absolutePos.front().y) + offset.y) / 1000.0 + sprite.real["stem.top.y1"]    * app_scale);
            renderer.line_to((scale(note.absolutePos.front().x) + offset.x) / 1000.0 + sprite.real["stem.top.x2"]    * app_scale,
                             (scale(note.absolutePos.front().y) + offset.y) / 1000.0 + sprite.real["stem.top.y2"]    * app_scale);
            renderer.line_to((scale(note.absolutePos.back().x)  + offset.x) / 1000.0 + sprite.real["stem.bottom.x2"] * app_scale,
                             (scale(note.absolutePos.back().y)  + offset.y) / 1000.0 + sprite.real["stem.bottom.y2"] * app_scale);
            renderer.line_to((scale(note.absolutePos.back().x)  + offset.x) / 1000.0 + sprite.real["stem.bottom.x1"] * app_scale,
                             (scale(note.absolutePos.back().y)  + offset.y) / 1000.0 + sprite.real["stem.bottom.y1"] * app_scale);
            renderer.fill();
            renderer.stroke();
        }
        // if the rest does not feature any flags (but consists of one single sprite)
        else
        {
            // just draw the sprite
            draw(renderer, note.sprite, note.absolutePos.front(), offset,
                 (sprite_scale * visible.appearance.scale) / 1000);
        };
        
        // render dots
        for (std::list< Position<mpx_t> >::const_iterator p = note.dotPos.begin(); p != note.dotPos.end(); ++p)
        {
            draw(renderer,
                 SpriteId(note.sprite.setid,
                           (renderer.get_sprites()[note.sprite.setid].dot != UNDEFINED) ?
                                   renderer.get_sprites()[note.sprite.setid].dot :
                                   renderer.get_sprites()[note.sprite.setid].undefined_symbol),
                 *p, offset,
                 (sprite_scale * visible.appearance.scale) / 1000);
        };
    }
    else if (object.is(Class::KEY))
    {
        // render key accidentals (count from 1, because the first position is the offset of the whole key)
        for (std::list< Position<mpx_t> >::const_iterator p = ++note.absolutePos.begin(); p != note.absolutePos.end(); ++p)
        {
            draw(renderer, note.sprite, *p, offset, (sprite_scale * visible.appearance.scale) / 1000);
        };
    }
    else if (object.is(Class::TIMESIG)) // normal time signature (custom time signatures are already processed!)
    {
        unsigned int n = static_cast<const TimeSig*>(&object)->number;
        for (std::list< Position<mpx_t> >::const_iterator p = ++note.absolutePos.begin(); p != note.absolutePos.end(); ++p)
        {
            draw(renderer,
                 SpriteId(note.sprite.setid, 
                           (renderer.get_sprites()[note.sprite.setid].digits_time[n % 10] == UNDEFINED) ?
                                   renderer.get_sprites()[note.sprite.setid].undefined_symbol :
                                   renderer.get_sprites()[note.sprite.setid].digits_time[n % 10]),
                 *p, offset,
                 (sprite_scale * visible.appearance.scale) / 1000);
            
            n /= 10;
            if (n == 0) n = static_cast<const TimeSig*>(&object)->beat;
        };
    }
    else if (object.is(Class::BARLINE))
    {
        renderer.set_line_width(0.0);
        for (std::list< Position<mpx_t> >::const_iterator p = note.absolutePos.begin(); p != note.absolutePos.end(); ++p)
        {
            mpx_t x_offset = 0;
            for (std::string::const_iterator i  = static_cast<const Barline*>(&object)->style.begin();
                                             i != static_cast<const Barline*>(&object)->style.end();
                                             ++i)
            {
                renderer.move_to((scale(x_offset * viewport->umtopx_h(style->bar_thickness) + p->x) + offset.x) / 1000.0,
                                 (scale(p->y) + offset.y) / 1000.0);
                renderer.line_to((scale((x_offset + *i) * viewport->umtopx_h(style->bar_thickness) + p->x) + offset.x) / 1000.0,
                                 (scale(p->y) + offset.y) / 1000.0);
                ++p;
                renderer.line_to((scale((x_offset + *i) * viewport->umtopx_h(style->bar_thickness) + p->x) + offset.x) / 1000.0,
                                 (scale(p->y) + offset.y) / 1000.0);
                renderer.line_to((scale(x_offset * viewport->umtopx_h(style->bar_thickness) + p->x) + offset.x) / 1000.0,
                                 (scale(p->y) + offset.y) / 1000.0);
                --p;
                renderer.fill();
                renderer.stroke();
                
                x_offset += *i;
                if (++i != static_cast<const Barline*>(&object)->style.end()) x_offset += *i;
                else break;
            };
            ++p;
        };
        renderer.stroke();
    }
    else {Log::warn("Unable to render plate-object. (class: Press)");};
    
    // render ties
    for (std::list<Plate::pNote::Tie>::const_iterator i = note.ties.begin(); i != note.ties.end(); ++i)
    {
        renderer.bezier_slur((scale(i->pos1.x)     + offset.x) / 1000.0, (scale(i->pos1.y)     + offset.y) / 1000.0,
                             (scale(i->control1.x) + offset.x) / 1000.0, (scale(i->control1.y) + offset.y) / 1000.0,
                             (scale(i->control2.x) + offset.x) / 1000.0, (scale(i->control2.y) + offset.y) / 1000.0,
                             (scale(i->pos2.x)     + offset.x) / 1000.0, (scale(i->pos2.y)     + offset.y) / 1000.0,
                             0,
                             scale(viewport->umtopx_h(style->tie_thickness)) / 1000.0);
    };
    
    // render attachables
    for (Plate::pNote::AttachableList::const_iterator i = note.attachables.begin(); i != note.attachables.end(); ++i)
    {
        render(renderer, &**i, offset, note.stem.top < note.stem.base, head_height, stem_width);
        if (parameters.draw_attachbounds) draw_boundaries(renderer, **i, offset);
    };
    
    if (parameters.draw_notebounds) draw_boundaries(renderer, note, offset);
    
    // reset color
    set_color(renderer, visible.appearance.color);
}

inline bool abs_less(const double& x, const double& y) {return (((x<0)?-x:x)<((y<0)?-y:y));}

// beam renderer
void Press::render_beam(Renderer& renderer, const Plate::pNote& note, const Chord& chord, const Position<mpx_t> offset, const mpx_t head_height, const mpx_t stem_width)
{
    // local variables
    double yoffset;                 // vertical beam position (in mpx)
    double yoffset_end;             // vertical beam position on end note (in mpx)
    const double beam_height = scale(style->beam_height * head_height) / 1000.0;    // height of beam (in mpx)
    const double head_width = (head_height * renderer.get_sprites().head_width(note.sprite)) / renderer.get_sprites().head_height(note.sprite);
    
    // render beams
    renderer.set_line_width(parameters.scale / 1000.0); // set line width
    for (size_t i = 0; i < VALUE_BASE - 2; i++)         // iterate through the beams, beginning at this note
    {
        if (note.beam[VALUE_BASE - 3 - i] == NULL) continue;    // check if a beam is to be drawn
        
        // calculate vertical offset (front)
        yoffset = scale(style->beam_height + style->beam_distance) * i * head_height / 1000.0; // in mpx
        
        if (note.beam[VALUE_BASE - 3 - i]->short_beam)  // if the beam is short
        {
            // calculate vertical offset (back)
            if ((note.beam[VALUE_BASE - 3 - i]->end->stem.top < note.beam[VALUE_BASE - 3 - i]->end->stem.base) ^ (note.stem.top < note.stem.base))
                yoffset_end = scale((style->beam_height + style->beam_distance)
                            * (static_cast<double>(note.stem.beam_off + note.beam[VALUE_BASE - 3 - i]->end->stem.beam_off) - note.beam[VALUE_BASE - 3 - i]->end_idx)
                            + style->beam_height) * head_height / 1000.0;
            else
                yoffset_end = scale(style->beam_height + style->beam_distance) * note.beam[VALUE_BASE - 3 - i]->end_idx * head_height / 1000.0;
            
            // render the short beam
            double length = head_width * style->shortbeam_length;           // calculate the length (i.e. width of the short beam)
            if (abs_less((note.beam[VALUE_BASE - 3 - i]->end->stem.x - note.stem.x) * static_cast<int>(style->shortbeam_short), length))
                length = (note.beam[VALUE_BASE - 3 - i]->end->stem.x - note.stem.x) * static_cast<int>(style->shortbeam_short);
            if ((length < 0) ^ note.beam[VALUE_BASE - 3 - i]->short_left) length = -length;
            length = scale(length) / 1000.0;
            
            const double x0 = scale(note.beam[VALUE_BASE - 3 - i]->end->stem.x) + offset.x;
            const double y0 = scale(note.beam[VALUE_BASE - 3 - i]->end->stem.top) + offset.y
                            + (  (note.beam[VALUE_BASE - 3 - i]->end->stem.top < note.beam[VALUE_BASE - 3 - i]->end->stem.base)
                               ? yoffset_end : -yoffset_end);
            const double x1 = scale(note.stem.x) + offset.x;                // in mpx
            const double y1 = scale(note.stem.top) + offset.y + ((note.stem.top < note.stem.base) ? yoffset : -yoffset);    // in mpx
            const double x2 = scale(note.stem.x) + offset.x + length;       // in mpx
            const double y2 = y1 + (length * (y1 - y0)) / (x1 - x0);
            
            if (note.stem.top < note.stem.base) // upward stem
            {
                renderer.move_to(x1 / 1000.0,  y1 / 1000.0);
                renderer.line_to(x1 / 1000.0, (y1 + beam_height) / 1000.0);
                renderer.line_to(x2 / 1000.0, (y2 + beam_height) / 1000.0);
                renderer.line_to(x2 / 1000.0,  y2 / 1000.0);
            }
            else                                // downward stem
            {
                renderer.move_to(x1 / 1000.0, (y1 - beam_height) / 1000.0);
                renderer.line_to(x1 / 1000.0,  y1 / 1000.0);
                renderer.line_to(x2 / 1000.0,  y2 / 1000.0);
                renderer.line_to(x2 / 1000.0, (y2 - beam_height) / 1000.0);
            };
            
            renderer.fill();    // close and fill the path
        }
        else                                            // else, we have got a normal beam
        {
            // calculate vertical offset (back)
            yoffset_end = scale(style->beam_height + style->beam_distance) * note.beam[VALUE_BASE - 3 - i]->end_idx * head_height / 1000.0; // in mpx
            
            // render beam
            if (note.stem.top < note.stem.base) // upward stem
            {
                renderer.move_to((scale(note.stem.x) + offset.x) / 1000.0,
                                 (scale(note.stem.top) + offset.y + yoffset) / 1000.0);
                renderer.line_to((scale(note.stem.x) + offset.x) / 1000.0,
                                 (scale(note.stem.top) + offset.y + yoffset + beam_height) / 1000.0);
            }
            else                                // downward stem
            {
                renderer.move_to((scale(note.stem.x) + offset.x) / 1000.0,
                                 (scale(note.stem.top) + offset.y - yoffset - beam_height) / 1000.0);
                renderer.line_to((scale(note.stem.x) + offset.x) / 1000.0,
                                 (scale(note.stem.top) + offset.y - yoffset) / 1000.0);
            };
            
            if (note.beam[VALUE_BASE - 3 - i]->end->stem.top < note.beam[VALUE_BASE - 3 - i]->end->stem.base)   // upward stem
            {
                renderer.line_to((scale(note.beam[VALUE_BASE - 3 - i]->end->stem.x) + offset.x) / 1000.0,
                                 (scale(note.beam[VALUE_BASE - 3 - i]->end->stem.top) + offset.y + yoffset_end + beam_height) / 1000.0);
                renderer.line_to((scale(note.beam[VALUE_BASE - 3 - i]->end->stem.x) + offset.x) / 1000.0,
                                 (scale(note.beam[VALUE_BASE - 3 - i]->end->stem.top) + offset.y + yoffset_end) / 1000.0);
            }
            else                                                                                                // downward stem
            {
                renderer.line_to((scale(note.beam[VALUE_BASE - 3 - i]->end->stem.x) + offset.x) / 1000.0,
                                 (scale(note.beam[VALUE_BASE - 3 - i]->end->stem.top) + offset.y - yoffset_end) / 1000.0);
                renderer.line_to((scale(note.beam[VALUE_BASE - 3 - i]->end->stem.x) + offset.x) / 1000.0,
                                 (scale(note.beam[VALUE_BASE - 3 - i]->end->stem.top) + offset.y - yoffset_end - beam_height) / 1000.0);
            }
            
            renderer.fill();    // close and fill the path
        };
    };
    
    // render flags
    if (!note.noflag)   // if a flag should be rendered
    {
        if (chord.val.exp >= VALUE_BASE - 2) return;    // if the note doesn't have any flags, abort
        
        // calculate set-dependent sprite scaling
        const double sprite_scale = head_height / renderer.get_sprites().head_height(note.sprite);  // promille
        
        // prepare flag sprite
        SpriteId flag_id = SpriteId(    // get flag sprite
            note.sprite.setid,
            renderer.get_sprites()[note.sprite.setid].flags_note);
        if (flag_id.spriteid == UNDEFINED) flag_id.set(0, renderer.get_sprites().front().flags_note);
        
        SpriteInfo& sprite = renderer.get_sprites()[flag_id];   // get sprite instance
        mpx_t flag_height = sprite.height * 1000;               // get sprite height
        
        // calculate sprite position offset
        Position<mpx_t> flag_anchor;            // flag-anchor position
        if (sprite.real.find("anchor.x") != sprite.real.end())  // get "anchor" attribute
            flag_anchor.x = _round(sprite.real.find("anchor.x")->second * 1000);
        if (sprite.real.find("anchor.y") != sprite.real.end())
            flag_anchor.y = _round(sprite.real.find("anchor.y")->second * 1000);
        
        // get sprite for overlayed flag
        SpriteId overlay_flag_id = SpriteId(    // get sprite id
            note.sprite.setid,
            renderer.get_sprites()[note.sprite.setid].flags_overlay);
        
        // get flag distance
        mpx_t flag_distance = 0;        // flag-distance
        if (sprite.real.find("distance") != sprite.real.end())
            flag_distance = _round(sprite.real.find("distance")->second * 1000);
        
        // render flags
        for (int i = 0; i < VALUE_BASE - 2 - chord.val.exp; i++)
        {
            renderer.draw_sprite(
                // get sprite
                (i == VALUE_BASE - 3 - chord.val.exp) ?
                    flag_id :           // the last flag is rendered normal
                    overlay_flag_id,    // each other flag is overlaid by the next
                
                // calculate position
                (scale(note.stem.x - flag_anchor.x + (stem_width * sprite_scale) / 1000.0) + offset.x) / 1000.0,
                
                (scale((note.stem.top < note.stem.base) ?
                               note.stem.top        // upward stem
                             + ((flag_distance * i - flag_anchor.y) * sprite_scale) / 1000.0 :
                               note.stem.top        // downward stem
                             - ((flag_distance * i - flag_anchor.y + flag_height) * sprite_scale) / 1000.0)
                + offset.y) / 1000.0,
                
                // caluclate scale
                scale(sprite_scale) / 1000.0,
                scale((note.stem.top < note.stem.base) ? sprite_scale : -sprite_scale) / 1000.0
            );
        };
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
                    draw_boundaries(renderer, pvoice->brace, offset);
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
                                     (offset.y + scale(pvoice->bracket.gphBox.pos.y + pvoice->bracket.gphBox.height)) / 1000.0,
                                     scale(pvoice->bracket.gphBox.width) / (1000.0 * renderer.get_sprites()[pvoice->bracket.sprite].width),
                                    -scale(pvoice->bracket.gphBox.width) / (1000.0 * renderer.get_sprites()[pvoice->bracket.sprite].width));
                
                if (parameters.draw_attachbounds)
                {
                    draw_boundaries(renderer, pvoice->bracket, offset);
                    renderer.set_color(0, 0, 0, 255);   // reset color
                };
            };
            
            // check, if we already had this staff
            if (staves.find(&pvoice->begin.staff()) != staves.end()) continue;
            staves.insert(&pvoice->begin.staff());
            
            // set line width
            renderer.set_line_width(
                scale(viewport->umtopx_h(
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
                                    i * viewport->umtopx_v(pvoice->begin.staff().head_height))
                    ) / 1000.0
                );
                
                renderer.line_to(
                        (offset.x + scale((line->line_end <= 0) ? line->basePos.x + 1e6 : line->line_end)
                        ) / 1000.0,
                        (offset.y + scale(pvoice->basePos.y +
                                    i * viewport->umtopx_v(pvoice->begin.staff().head_height))
                        ) / 1000.0
                );
            };
            renderer.stroke();  // render all lines
            
            // set max-/min-pos (for front line rendering)
            if (min_pos > pvoice->basePos.y) min_pos = pvoice->basePos.y;
            if (max_pos < pvoice->basePos.y + viewport->umtopx_v(pvoice->begin.staff().head_height * (pvoice->begin.staff().line_count - 1)))
                max_pos = pvoice->basePos.y + viewport->umtopx_v(pvoice->begin.staff().head_height * (pvoice->begin.staff().line_count - 1));
        };
        
        staves.clear(); // erase remembered staves
        
        // render the front line
        renderer.set_line_width(scale(viewport->umtopx_h(default_style.bar_thickness)) / 1000.0);
        renderer.move_to((offset.x + scale(line->basePos.x)) / 1000.0,
                         (offset.y + scale(min_pos)) / 1000.0);
        renderer.line_to((offset.x + scale(line->basePos.x)) / 1000.0,
                         (offset.y + scale(max_pos)) / 1000.0);
        renderer.stroke();
        
        // render the line's boundary box
        if (parameters.draw_linebounds)
        {
            draw_boundaries(renderer, *line, offset);
            renderer.set_color(0, 0, 0, 255);   // reset color
        };
    };
}

// constructor (providing viewport parameters)
Press::Press(const StyleParam& _style, const ViewportParam& _viewport) : viewport(&_viewport),
                                                                         style(&_style),
                                                                         default_style(_style) {}

// draw the boundary box of a graphical object
void Press::draw_boundaries(Renderer& renderer, const Plate::pGraphical& object, const Position<mpx_t> offset) throw(InvalidRendererException)
{
    // check if the renderer is ready
    if (!renderer.ready()) throw InvalidRendererException();
    
    // render the box
    renderer.set_line_width(1.0);
    renderer.set_color(static_cast<unsigned char>(parameters.boundary_color & 0xFF),
                       static_cast<unsigned char>((parameters.boundary_color >> 8) & 0xFF),
                       static_cast<unsigned char>((parameters.boundary_color >> 16) & 0xFF),
                       static_cast<unsigned char>((parameters.boundary_color >> 24) & 0xFF));
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
    renderer.set_color(static_cast<unsigned char>(parameters.boundary_color & 0xFF),
                       static_cast<unsigned char>((parameters.boundary_color >> 8) & 0xFF),
                       static_cast<unsigned char>((parameters.boundary_color >> 16) & 0xFF),
                       static_cast<unsigned char>((parameters.boundary_color >> 24) & 0xFF));
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
            style = (!!pvoice->begin.staff().style) ? &*pvoice->begin.staff().style : &default_style;
            
            // iterate the voice
            for (std::list<Plate::pNote>::const_iterator it = pvoice->notes.begin(); it != pvoice->notes.end(); ++it)
            {
                render(renderer, *it, offset, viewport->umtopx_v(pvoice->begin.staff().head_height), viewport->umtopx_h(style->stem_width));
            };
        };
    };
    
    // reset style
    style = &default_style;
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
    
    // render attachables
    for (std::list<Plate::pAttachable>::const_iterator i = page.attachables.begin(); i != page.attachables.end(); ++i)
    {
        render(renderer, &*i, offset, true, pageset.head_height, pageset.stem_width);
        if (parameters.draw_attachbounds) draw_boundaries(renderer, *i, offset);
    };
}

// render page decoration
void Press::render_decor(Renderer& renderer, const PageSet& pageset, const Position<mpx_t> offset) throw(InvalidRendererException)
{
    // draw shadow
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
}

// render a cursor through the given renderer
void Press::render(Renderer& renderer, const UserCursor& cursor, const Position<mpx_t> offset) throw (InvalidRendererException, UserCursor::NotValidException)
{
    // get horizontal position
    mpx_t x = cursor.graphical_x();
    if (!cursor.is_behind())
        x -= _round(viewport->umtopx_h(parameters.cursor_distance));
    else
        x += _round(viewport->umtopx_h(parameters.cursor_distance));
    
    // get vertical position and height
    mpx_t y = cursor.graphical_y(*viewport);
    mpx_t h = cursor.graphical_height(*viewport);
    
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


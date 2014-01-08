
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

#include "engraver.hh"         // Engraver
#include "engraver_state.hh"   // EngraverState
#include "undefined.hh"        // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                               // this number is interpreted as an undefined value
using namespace ScorePress;

inline int _round(const double d) {return static_cast<int>(d + 0.5);}

//
//     class Engraver
//    ================
//
// This class is used to convert a score-object to a set of absolute-positioned
// sprites, being saved to a plate. The target plate as well as the sprite-
// library are given during the construction.
// An instance of "EngraverState" is used to iterate and engrave the score.
//

// engrave the on-page attachables
void Engraver::engrave_attachables(const Document& data)
{
    std::list<Pageset::pPage>::iterator p = pageset->pages.begin();
    unsigned int pageno = 0;
    const std::list<Document::Attached> attached = data.get_attached();
    for (std::list<Document::Attached>::const_iterator a = attached.begin(); a != attached.end(); ++a)
    {
        while (pageno < a->page) {++pageno; ++p;};
        p->attachables.push_back(Plate::pAttachable(*a->object,
                                                 Position<mpx_t>(viewport->umtopx_h(a->object->position.x),
                                                                 viewport->umtopx_v(a->object->position.y))));
        if (a->object->is(Class::TEXTAREA))
        {
            const TextArea& obj = *static_cast<const TextArea*>(a->object);
            p->attachables.back().gphBox.pos = p->attachables.back().absolutePos;
            p->attachables.back().gphBox.width = _round((viewport->umtopx_h(obj.width) * obj.appearance.scale) / 1000.0);
            p->attachables.back().gphBox.height = _round((viewport->umtopx_h(obj.height) * obj.appearance.scale) / 1000.0);
        }
        else if (a->object->is(Class::CUSTOMSYMBOL))
        {
            const CustomSymbol& obj = *static_cast<const CustomSymbol*>(a->object);
            const SpriteId sprite_id =
                (obj.sprite.setid == UNDEFINED) ?
                    SpriteId(0, sprites->front().undefined_symbol) :
                    ((obj.sprite.spriteid == UNDEFINED) ?
                        SpriteId(obj.sprite.setid, (*sprites)[obj.sprite.setid].undefined_symbol) :
                        obj.sprite);
            const double scale =  (data.head_height / sprites->head_height(sprite_id))
                                * (obj.appearance.scale / 1000.0);
            
            p->attachables.back().gphBox.pos    = p->attachables.back().absolutePos;
            p->attachables.back().gphBox.width  = _round(scale * (*sprites)[sprite_id].width);
            p->attachables.back().gphBox.height = _round(scale * (*sprites)[sprite_id].height);
        }
        else log_warn(("On-page object type \"" + classname(a->object->classtype()) + "\" not implemented yet. (class: Engraver)").c_str());
    };
}

// constructor (given target-plate and sprite-library)
Engraver::Engraver(Pageset& _pageset, const Sprites& _sprites, const StyleParam& _style, const ViewportParam& _viewport) :
    pageset(&_pageset),
    sprites(&_sprites),
    style(&_style),
    viewport(&_viewport) {}

// engrave the whole score
void Engraver::engrave(const Score& score, const unsigned int start_page)
{
    EngraverState state(score, start_page, *pageset, *sprites, parameters, *style, *viewport);
    if (logging_log) state.log_set(*logging_log);
    while (state.engrave_next());
}

// engrave the document
void Engraver::engrave(const Document& document)
{
    pageset->clear();
    pageset->page_layout.set(document.page_layout, *viewport);
    pageset->head_height = viewport->umtopx_v(document.head_height);
    pageset->stem_width = viewport->umtopx_h(document.stem_width);
    
    for (std::list<Document::Score>::const_iterator i = document.scores.begin(); i != document.scores.end(); ++i)
    {
        engrave(i->score, i->start_page);
    };
    engrave_attachables(document);
}


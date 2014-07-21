
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
    for (Document::AttachedMap::const_iterator apage = data.attached.begin(); apage != data.attached.end(); ++apage)
    {
        for (MovableList::const_iterator a = apage->second.begin(); a != apage->second.end(); ++a)
        {
            while (pageno < apage->first) {++pageno; ++p;};
            p->attached.push_back(Plate::pNote::AttachablePtr(new Plate::pAttachable(
                            **a, Position<mpx_t>(viewport->umtopx_h((*a)->position.x),
                                                viewport->umtopx_v((*a)->position.y)))));
            if ((*a)->is(Class::TEXTAREA))
            {
                const TextArea& obj = *static_cast<const TextArea*>(&**a);
                p->attached.back()->gphBox.pos = p->attached.back()->absolutePos;
                p->attached.back()->gphBox.width = _round((viewport->umtopx_h(obj.width) * obj.appearance.scale) / 1000.0);
                p->attached.back()->gphBox.height = _round((viewport->umtopx_h(obj.height) * obj.appearance.scale) / 1000.0);
            }
            else if ((*a)->is(Class::CUSTOMSYMBOL))
            {
                const CustomSymbol& obj = *static_cast<const CustomSymbol*>(&**a);
                const SpriteId sprite_id =
                    (obj.sprite.setid == UNDEFINED) ?
                        SpriteId(0, sprites->front().undefined_symbol) :
                        ((obj.sprite.spriteid == UNDEFINED) ?
                            SpriteId(obj.sprite.setid, (*sprites)[obj.sprite.setid].undefined_symbol) :
                            obj.sprite);
                const double scale =  (data.head_height / sprites->head_height(sprite_id))
                                    * (obj.appearance.scale / 1000.0);
                
                p->attached.back()->gphBox.pos    = p->attached.back()->absolutePos;
                p->attached.back()->gphBox.width  = _round(scale * (*sprites)[sprite_id].width);
                p->attached.back()->gphBox.height = _round(scale * (*sprites)[sprite_id].height);
            }
            else log_warn(("On-page object type \"" + classname((*a)->classtype()) + "\" not implemented yet. (class: Engraver)").c_str());
        };
    };
}

// constructor (given target-plate and sprite-library)
Engraver::Engraver(Pageset& _pageset, const Sprites& _sprites, const StyleParam& _style, const ViewportParam& _viewport) :
    pageset(&_pageset),
    sprites(&_sprites),
    style(&_style),
    viewport(&_viewport) {}

// engrave the score
void Engraver::engrave(const Score& score, const unsigned int start_page, const unsigned int head_height)
{
    log_debug("engrave (score)");
    EngraverState state(score, start_page, *pageset, *sprites, head_height, parameters, *style, *viewport);
    state.log_set(*this);
    while (state.engrave_next());
}

void Engraver::engrave(const Score& score, const unsigned int start_page, const unsigned int head_height, ReengraveInfo& info)
{
    log_debug("engrave (score with reengrave-info)");
    EngraverState state(score, start_page, *pageset, *sprites, head_height, parameters, *style, *viewport);
    state.set_reengrave_info(info);
    state.log_set(*this);
    while (state.engrave_next());
}

// engrave the document
void Engraver::engrave(const Document& document)
{
    log_debug("engrave (document)");
    pageset->clear();
    pageset->page_layout.set(document.page_layout, *viewport);
    pageset->head_height = viewport->umtopx_v(document.head_height);
    pageset->stem_width = viewport->umtopx_h(document.stem_width);
    
    for (std::list<Document::Score>::const_iterator i = document.scores.begin(); i != document.scores.end(); ++i)
    {
        engrave(i->score, i->start_page, document.head_height);
    };
    engrave_attachables(document);
}

void Engraver::engrave(const Document& document, ReengraveInfo& info)
{
    log_debug("engrave (document with reengrave-info)");
    pageset->clear();
    pageset->page_layout.set(document.page_layout, *viewport);
    pageset->head_height = viewport->umtopx_v(document.head_height);
    pageset->stem_width = viewport->umtopx_h(document.stem_width);
    
    for (std::list<Document::Score>::const_iterator i = document.scores.begin(); i != document.scores.end(); ++i)
    {
        engrave(i->score, i->start_page, document.head_height, info);
    };
    engrave_attachables(document);
}



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

#include "classes.hh"           // [score classes]
#include "engraver_state.hh"    // EngraverState
#include "undefined.hh"         // defines "UNDEFINED" macro, resolving to the largest value "size_t" can contain
                                // this number is interpreted as an undefined value
using namespace ScorePress;

inline static int _round(const double d) {return static_cast<int>(d + 0.5);}

// returns a human readable class name (for debugging purposes)
const std::string ScorePress::classname(Class::classType type)
{
    switch (type)
    {
    case Class::VISIBLEOBJECT: return "VisibleObject";
    case Class::STAFFOBJECT:   return "StaffObject";
    case Class::VOICEOBJECT:   return "VoiceObject";
    case Class::NEWLINE:       return "Newline";
    case Class::PAGEBREAK:     return "Pagebreak";
    case Class::MUSICOBJECT:   return "MusicObject";
    case Class::CLEF:          return "Clef";
    case Class::KEY:           return "Key";
    case Class::TIMESIG:       return "TimeSig";
    case Class::CUSTOMTIMESIG: return "CustomTimeSig";
    case Class::BARLINE:       return "Barline";
    case Class::NOTEOBJECT:    return "NoteObject";
    case Class::CHORD:         return "Chord";
    case Class::REST:          return "Rest";
    
    case Class::ACCIDENTAL:    return "Accidental";
    case Class::ARTICULATION:  return "Articulation";
    case Class::HEAD:          return "Head";
    case Class::TIEDHEAD:      return "TiedHead";
    
    case Class::VOICE:         return "Voice";
    case Class::MAINVOICE:     return "MainVoice";
    case Class::SUBVOICE:      return "SubVoice";
    
    case Class::MOVABLE:       return "Movable";
    case Class::TEXTAREA:      return "TextArea";
    case Class::ANNOTATION:    return "Annotation";
    case Class::PLUGININFO:    return "PluginInfo";
    case Class::SYMBOL:        return "Symbol";
    case Class::CUSTOMSYMBOL:  return "CustomSymbol";
    case Class::DURABLE:       return "Durable";
    case Class::SLUR:          return "Slur";
    case Class::HAIRPIN:       return "Hairpin";
    
    default:                   return "Unknown";
    };
}

// RGBA-color structure (equality operator)
bool ScorePress::operator == (const Color& c1, const Color& c2)
{
    return (c1.r == c2.r) && (c1.g == c2.g) && (c1.b == c2.b) && (c1.a == c2.a);
}

// check, if the score-object contains a given point
bool ScoreDimension::contains(const Position<>& pos)
{
    return (   pos.x >= position.x && pos.y >= position.y
            && pos.x < position.x + static_cast<int>(width)
            && pos.y < position.y + static_cast<int>(height));
}

const char* const Barline::singlebar = "\x01";
const char* const Barline::doublebar = "\x01\x01\x01";
const char* const Barline::endbar    = "\x01\x01\x02";

// tone modification for each accidental-type
const int Accidental::note_modifier[9] = {-2, -1, -1, 0, 0, 0, 1, 1, 2};

//
// let v / (2 ^ VALUE_BASE) be the value of the note-object:
//
//                d                                                          d
//      e        2  - 1     enum       e    e-d   d        enum        e    2  - 1     enum
// v = 2  ( 1 + -------- ) ------- = (2  + 2    (2  - 1)) ------- = ( 2  + -------- ) -------
//                  d       denom                          denom              d-e      denom
//                 2                                                         2
//
// in case that e >= d:
//    it is 2^e a power of two and 2^e > 2^(e-d)*(2^d-1) so that we can use the | operator instead of +
//
// in case that e < d:
//    we split the result into a mixed fraction: 2^e+(2^d-1)/(2^(d-e))
//
const value_t NoteObject::value() const
{
    return
    (irr_denom == 0) ?
    (
        (val.dots <= val.exp) ?   // 2 ^ exp + (2 ^ dots - 1) * (2 ^ (exp - dots))
            value_t(base() | (((1u << val.dots) - 1u) << (val.exp - val.dots))) :
            value_t(base(), (1u << val.dots) - 1u, 1u << (val.dots - val.exp))
                                  // 2 ^ exp + (2 ^ dots - 1) / (2 ^ (dots - exp))
    ) : (
        (value_t((irr_enum == 0 ? irr_denom - 1 : irr_enum), irr_denom)) *
        ((val.dots <= val.exp) ?  // 2 ^ exp + (2 ^ dots - 1) * (2 ^ (exp - dots))
            value_t(base() | (((1u << val.dots) - 1u) << (val.exp - val.dots))) :
            value_t(base(), (1u << val.dots) - 1u, 1u << (val.dots - val.exp))
        )                         // 2 ^ exp + (2 ^ dots - 1) / (2 ^ (dots - exp))
    );
}

// create note object from length (return error factor)
value_t NoteObject::set_value(value_t v)
{
    v = v.abs();
    
    unsigned long vi = v.i();
    unsigned char nexp = 0x00;
    unsigned char ndots = 0x00;
    unsigned long nenum = 0x00;
    unsigned long ndenom = 0x00;
    
    // calculate biggest possible exponent
    while ((1u << nexp) <= vi && nexp <= 0x0F) ++nexp;
    if (nexp) v /= (1u << --nexp);
    
    // if there may be any dots, calculate biggest number of dots
    if (2 * v.e() <= static_cast<long>(v.d()) || v.d() % 2 == 0)
    {
            while (value_t((1l << (ndots + 1)) - 1l, 1l << ndots) <= v && ndots <= 0x08) ++ndots;
            if (ndots) --ndots;
            v /= value_t((1l << (ndots + 1)) - 1l, 1l << ndots);
    };
    
    // normalize irregular tuplet
    while (v.e() > static_cast<long>(v.d())) {++nexp; v /= 2;};
    nenum = v.e();
    ndenom = v.d();
    while (nenum >= 256 || ndenom >= 256) {nenum /= 2; ndenom /= 2;};
    
    // set values
    val.exp = nexp & 0x0F;
    val.dots = ndots & 0x07;
    irr_enum = nenum & 0xFF;
    irr_denom = ndenom & 0xFF;
    
    // return error (due to tuplet accuracy limit; as factor)
    v -= value_t(nenum, ndenom);
    return v.abs();
}

void NoteObject::add_subvoice(RefPtr<SubVoice>& newvoice)
{
    if (!subvoice) subvoice = newvoice;
    else
    {
        RefPtr<SubVoice>* sub = &subvoice;
        while (!!*sub)
        {
            VoiceObjectList::iterator i = (*sub)->notes.begin();
            while (i != (*sub)->notes.end() && !(*i)->is(NOTEOBJECT)) ++i;
            if (i == (*sub)->notes.end()) return;
            sub = &static_cast<NoteObject&>(**i).subvoice;
        };
        *sub = newvoice;
    };
}

// engraving method (Clef)
void Clef::engrave(EngraverState& engraver) const
{
    // get data
          Plate::pNote&  pnote       = engraver.get_target();
    const Sprites&       sprites     = engraver.get_sprites();
    const mpx_t&         head_height = engraver.get_head_height();
    
    // modify context
    engraver.get_target_line().staffctx[&engraver.get_staff()].modify(*this);
    
    // set sprite
    pnote.sprite = Pick::sprite_id(sprites, this);
    
    // get sprite and position-offset
    const SpriteInfo& spriteinfo = sprites[pnote.sprite];   // get sprite-info
    const double scale = head_height /                      // calculate scale factor
                         (sprites.head_height(pnote.sprite) * 1000.0);
    
    Position<mpx_t> anchor;                         // anchor position
    anchor.x = _round(spriteinfo.get_real("anchor.x") * scale * this->appearance.scale);
    anchor.y = _round(spriteinfo.get_real("anchor.y") * scale * this->appearance.scale);
    
    // apply sprite-offset (x-offset applied by "Engraver::apply_offsets()")
    pnote.absolutePos.front() -= anchor;
    pnote.absolutePos.front().y += (head_height * this->line) / 2;
    
    // calculate graphical boundaries
    pnote.gphBox.pos    = pnote.absolutePos.front();
    pnote.gphBox.width  = _round(spriteinfo.width  * scale * this->appearance.scale);
    pnote.gphBox.height = _round(spriteinfo.height * scale * this->appearance.scale);
    
    // break ties
    engraver.break_ties();
}

// engraving method (Key)
void Key::engrave(EngraverState& engraver) const
{
    // get data
          Plate::pNote&  pnote       = engraver.get_target();
    const Staff&         staff       = engraver.get_staff();
    const Sprites&       sprites     = engraver.get_sprites();
    const mpx_t&         head_height = engraver.get_head_height();
    
    // modify context
    StaffContext& staffctx = engraver.get_target_line().staffctx[&staff];
    staffctx.modify(*this);
    
    // set sprite
    pnote.sprite = Pick::sprite_id(sprites, this);
    
    // get sprite and position-offset
    const SpriteInfo& spriteinfo = sprites[pnote.sprite];   // get sprite-info
    const double scale = head_height /                      // calculate scale factor
                         (sprites.head_height(pnote.sprite) * 1000.0);
    
    const mpx_t sprite_width = _round(spriteinfo.width * scale * this->appearance.scale);
    const mpx_t sprite_height = _round(spriteinfo.height * scale * this->appearance.scale);
    
    Position<mpx_t> anchor;                                         // anchor position
    anchor.x = _round(spriteinfo.get_real("anchor.x") * scale * this->appearance.scale);
    anchor.y = _round(spriteinfo.get_real("anchor.y") * scale * this->appearance.scale);
    
    // initialize graphical boundaries
    pnote.gphBox.pos = pnote.absolutePos.back();
    pnote.gphBox.width = 1000;
    pnote.gphBox.height = 1000;
    
    // calculate key accidental positions
    Position<mpx_t> kpos;
    for (char i = 0; i < this->number; ++i)
    {
        // context.key_offset returns correct values, because the context was already modified by the key
        kpos.x = (i * Pick::width(sprites, this, head_height)) / this->number;
        kpos.y = engraver.get_staffctx().key_offset(this->type, i, head_height);
        kpos -= anchor;
        pnote.absolutePos.push_back(pnote.absolutePos.front() + kpos);
        
        // calculate graphical boundaries
        pnote.gphBox.extend(Plate::pGraphical::Box(pnote.absolutePos.back(), sprite_width, sprite_height));
    };
    
    // break ties
    engraver.break_ties();
}

// engraving method (Time Signature)
void TimeSig::engrave(EngraverState& engraver) const {engrave(engraver, 0);}
void TimeSig::engrave(EngraverState& engraver, size_t setid) const
{
    // get data
          Plate::pNote&  pnote       = engraver.get_target();
          Plate::pLine&  pline       = engraver.get_target_line();
    const Sprites&       sprites     = engraver.get_sprites();
    const mpx_t&         head_height = engraver.get_head_height();
    
    // apply time signature to the whole score
    const value_t& time = engraver.get_time();
    for (std::list<Plate::pVoice>::iterator i = pline.voices.begin(); i != pline.voices.end(); ++i)
    {
        i->context.modify(*this, time);
    };
    
    // set graphical boundary width to 0 (indicating, that it has not been set yet)
    pnote.gphBox.width = 0;
    
    // set sprite
    pnote.sprite = SpriteId(setid, sprites[setid].undefined_symbol);
    
    // save number- and denominator-widths
    const mpx_t width_number = Pick::width(sprites[setid], this->number, head_height);
    const mpx_t width_beat   = Pick::width(sprites[setid], this->beat, head_height);
    const mpx_t time_height  = _round((engraver.get_staff().line_count - 1) * head_height / 2.0);
    
    // start engraving
    unsigned int n;                     // number buffer
    Position<mpx_t> npos;               // position buffer
    Position<mpx_t> anchor;             // anchor position
    size_t sprite;                      // sprite buffer
    const double scale = head_height /  // scale factor
                         (sprites.head_height(pnote.sprite) * 1000.0);
    
    // engrave number
    n = this->number;                   // copy the number to be drawn
    npos = pnote.absolutePos.front();   // copy the base position
    npos.x += width_number;             // add complete width
    if (width_number < width_beat)      // if this is wider than the beat
        npos.x += (width_beat - width_number) / 2;  // center this number (horizontally)
    
    do  // engrave each digit
    {
        // calculate the sprite-id
        sprite = (sprites[setid].digits_time[n % 10] == UNDEFINED) ?
                    sprites[setid].undefined_symbol :
                    sprites[setid].digits_time[n % 10];
        
        // get "anchor" attribute
        anchor.x = _round(sprites[setid][sprite].get_real("anchor.x") * scale * this->appearance.scale);
        anchor.y = _round(sprites[setid][sprite].get_real("anchor.y") * scale * this->appearance.scale);
        
        // detract sprite-width from horizontal position
        npos.x -= (sprites[setid][sprite].width * head_height) / sprites[setid].head_height;
        pnote.absolutePos.push_back(npos);  // append position
        pnote.absolutePos.back() -= anchor; // apply sprite-offset
        
        // center (vertically)
        pnote.absolutePos.back().y +=
            (time_height > static_cast<mpx_t>((sprites[setid][sprite].height * head_height) / sprites[setid].head_height)) ?
                (time_height - (sprites[setid][sprite].height * head_height) / sprites[setid].head_height) / 2 :
                 time_height - (sprites[setid][sprite].height * head_height) / sprites[setid].head_height;
        
        // consider space between digits
        npos.x -= (sprites[setid].timesig_digit_space * head_height) / sprites[setid].head_height;
        
        // calculate graphical boundaries
        if (pnote.gphBox.width == 0)    // if the box was not set yet
        {                               // set to dimensions of this sprite
            pnote.gphBox.pos = pnote.absolutePos.back();
            pnote.gphBox.width = _round(sprites[setid][sprite].width * scale * this->appearance.scale);
            pnote.gphBox.height = _round(sprites[setid][sprite].height * scale * this->appearance.scale);
        }
        else                            // otherwise
        {                               // extend the graphical boundary box
            pnote.gphBox.extend(Plate::pGraphical::Box(
                pnote.absolutePos.back(),
                _round(sprites[setid][sprite].width * scale * this->appearance.scale),
                _round(sprites[setid][sprite].height * scale * this->appearance.scale)));
        };
        
        n /= 10;        // cut off last digit
    } while(n != 0);    // repeat until the whole number is drawn
    
    // engrave beat
    n = this->beat;                     // copy the number to be drawn
    npos = pnote.absolutePos.front();   // copy the base position
    npos.x += width_beat;               // add complete width
    if (width_beat < width_number)      // if this is wider than the beat
        npos.x += (width_number - width_beat) / 2;  // center this number (horizontally)
    npos.y += time_height;              // move to the lower half of the staff
    
    do  // engrave each digit
    {
        // calculate the sprite-id
        sprite = (sprites[setid].digits_time[n % 10] == UNDEFINED) ?
                    sprites[setid].undefined_symbol :
                    sprites[setid].digits_time[n % 10];
        
        // get "anchor" attribute
        anchor.x = _round(sprites[setid][sprite].get_real("anchor.x") * scale * this->appearance.scale);
        anchor.y = _round(sprites[setid][sprite].get_real("anchor.y") * scale * this->appearance.scale);
        
        // detract sprite-width from horizontal position
        npos.x -= (sprites[setid][sprite].width * head_height) / sprites[setid].head_height;
        pnote.absolutePos.push_back(npos);  // append position
        pnote.absolutePos.back() -= anchor; // apply sprite-offset
        
        // center (vertically)
        if (time_height > static_cast<mpx_t>((sprites[setid][sprite].height * head_height) / sprites[setid].head_height))
            pnote.absolutePos.back().y += (time_height - (sprites[setid][sprite].height * head_height) / sprites[setid].head_height) / 2;
        
        // calculate graphical boundaries
        pnote.gphBox.extend(Plate::pGraphical::Box(
            pnote.absolutePos.back(),
            _round(sprites[setid][sprite].width * scale * this->appearance.scale),
            _round(sprites[setid][sprite].height * scale * this->appearance.scale)));
        
        // consider space between digits
        npos.x -= (sprites[setid].timesig_digit_space * head_height) / sprites[setid].head_height;
        
        n /= 10;        // cut off last digit
    } while(n != 0);    // repeat until the whole number is drawn
    
    // break ties
    engraver.break_ties();
}

// engraving method (Custom Time Signature)
void CustomTimeSig::engrave(EngraverState& engraver) const
{
    // if the sprite-id is complete
    if (this->sprite.ready())
    {
        // get data
              Plate::pNote&  pnote       = engraver.get_target();
        const Sprites&       sprites     = engraver.get_sprites();
        const mpx_t&         head_height = engraver.get_head_height();
        
        // set sprite
        pnote.sprite = this->sprite;
        
        // get sprite and position-offset
        const SpriteInfo& spriteinfo = sprites[pnote.sprite]; // get sprite-info
        const double scale = head_height /                    // calculate scale factor
                             (sprites.head_height(pnote.sprite) * 1000.0);
        
        Position<mpx_t> anchor;                                        // anchor position
        anchor.x = _round(spriteinfo.get_real("anchor.x") * scale * this->appearance.scale);
        anchor.y = _round(spriteinfo.get_real("anchor.y") * scale * this->appearance.scale);
        
        // apply sprite-offset (x-offset applied by "apply_offsets()")
        pnote.absolutePos.front() -= anchor;
        pnote.absolutePos.front().y += _round(  (engraver.get_staff().line_count - 1) * head_height
                                              - spriteinfo.height * scale * this->appearance.scale   ) / 2;
        
        // calculate graphical boundaries
        pnote.gphBox.pos = pnote.absolutePos.front();
        pnote.gphBox.width = _round(spriteinfo.width * scale * this->appearance.scale);
        pnote.gphBox.height = _round(spriteinfo.height * scale * this->appearance.scale);
        
        // break ties
        engraver.break_ties();
    }
    
    // if only the set-id is given
    else if (this->sprite.setid != UNDEFINED)
    {
        TimeSig::engrave(engraver, this->sprite.setid);  // use it for the normal time-sig
        engraver.break_ties();                           // break ties
    }
    
    // if the sprite is not ready, handle object like a normal time-signature
    else TimeSig::engrave(engraver);
}

// engraving method (Barline)
void Barline::engrave(EngraverState& engraver) const
{
    // get data
          Plate::pNote&  pnote       = engraver.get_target();
          Plate::pLine&  pline       = engraver.get_target_line();
    const Sprites&       sprites     = engraver.get_sprites();
    const mpx_t&         head_height = engraver.get_head_height();
    const EngraverParam& param       = engraver.get_parameters();
    const ViewportParam& viewport    = engraver.get_viewport();
    
    // set default sprite-set
    pnote.sprite.setid = 0;
    
    // add additional distance
    engraver.add_distance_after(viewport.umtopx_h(param.barline_distance), engraver.get_time());
    pnote.absolutePos.front().x += viewport.umtopx_h(param.barline_distance);
    mpx_t posx = pnote.absolutePos.front().x;
    
    // forget memorized accidentals
    for (Plate::pLine::StaffContextMap::iterator ctx = pline.staffctx.begin(); ctx != pline.staffctx.end(); ++ctx)
        ctx->second.reset_acc();
    
    // engrave barline
    std::list<Staff>::const_iterator i   = engraver.get_score().staves.begin();
    std::list<Staff>::const_iterator end = engraver.get_score().staves.end();
    std::list<Plate::pVoice>::const_iterator pvoice = pline.get_voice(*i);
    pnote.absolutePos.front().y = pvoice->basePos.y;
    while (i != end)    // iterate the staffs
    {
        if (pline.voices.end() != pline.get_voice(*i))  // if voice exists,...
            pvoice = pline.get_voice(*i);               // get on-plate voice
        
        if (!i->long_barlines)    // if the barline is not a long one
        {
            pnote.absolutePos.push_back(    // add end-point at the staff's bottom
                Position<mpx_t>(
                    posx,
                    pvoice->basePos.y +
                        viewport.umtopx_v((static_cast<int>(i->line_count) - 1) * i->head_height)
                )
            );
            
            // append next start point (if necessary)
            if (++i != end)
                pnote.absolutePos.push_back(Position<mpx_t>(posx, pvoice->basePos.y));
        }
        else if (++i == end)    // if the barline is a long one, but we have no further staffs
        {
            --i;                // get the last staff
            pnote.absolutePos.push_back(    // and add end-point at the staff's bottom
                Position<mpx_t>(
                    posx,
                    pvoice->basePos.y +
                        viewport.umtopx_v((static_cast<int>(i->line_count) - 1) * i->head_height)
                )
            );
        };
    };
    
    // calculate graphical boundaries
    pnote.gphBox.pos = pnote.absolutePos.front();
    pnote.gphBox.width = Pick::width(sprites, this, head_height) * viewport.umtopx_h(engraver.get_style().bar_thickness);
    pnote.gphBox.extend(pnote.absolutePos.back());
    
    // warn about illegal barline style
    if (this->style.size() % 2 != 1)
        engraver.log_warn("Barline has illegal style. (class: Barline)");
}

// engraving method (Newline)
void Newline::engrave(EngraverState& engraver) const
{
    Plate::pNote& pnote = engraver.get_target();
    pnote.gphBox.pos = pnote.absolutePos.front();
    pnote.gphBox.width = 1000;
    pnote.gphBox.height = engraver.get_head_height() * (engraver.get_staff().line_count - 1);
}

// add the given head to the target "pNote" instance (and return true, if a cluster ocurred; for stem correction)
static bool engrave_head(Head&          head,           // head to be engraved
                     Plate::pNote&      target,         // target pNote instance
                     Position<mpx_t>    headPos,        // position of the head
               const Plate::pVoice&     voice,          // voice (for accidental offset)
               const StaffContext&      staffctx,       // staff-context for accidental calculation
               const StaffContext&      gph_context,    // staff-context for note-offset
               const mpx_t              sprite_width,   // width of the head's sprite
               const mpx_t              head_height,    // the staff's head-height
               const double             chord_scale,    // the scale of the chord (and such for the head, too)
                     bool               no_accidental,  // omit accidental?
               const Sprites&           sprites,        // sprite set
                     EngraverState&     engraver)       // the engraver instance
{
    // return value
    bool got_cluster = false;    // will be set to true, if the head has to be moved due to close neighbors
    
    // add notehead-offset
    headPos.y += gph_context.note_offset(head, head_height);
    
    // add horizontal offset for close heads in the chord
    Position<mpx_t> unscaledPos(headPos);            // save unscaled pos for accidental
    for (std::list< Position<mpx_t> >::iterator j = ++target.absolutePos.begin(); j != target.absolutePos.end(); ++j)
    {
        // if the head is close to ours, and is not drawn on the other side of the stem
        if (((j->y - headPos.y > 0 && j->y - headPos.y < head_height)  ||
             (headPos.y - j->y > 0 && headPos.y - j->y < head_height)) &&
            headPos.x == j->x)
        {
            headPos.x += sprite_width;  // move this head to the other side
            got_cluster = true;         // change return value
            break;                      // further heads will not collide
        };
    };
    
    // apply scaling offset (vertical centerizing)
    headPos.y += _round((((1000.0 - (head.appearance.scale * chord_scale)) / 1000.0) * head_height) / 2.0);
    
    // push position for head ("sprite_width < 0" implies downward stem, that means heads are inserted backwards)
    if (sprite_width >= 0) target.absolutePos.push_back(headPos);
    else                   target.absolutePos.insert(++target.absolutePos.begin(), headPos);
    
    // move cluster to the right
    if (got_cluster && sprite_width < 0 && engraver.has_cluster_space())
    {
        int offset = _round((-sprite_width * head.appearance.scale) / 1000.0);
        target.add_offset(offset);
        target.absolutePos.front().x -= offset;
        unscaledPos += offset;
    };
    
    // add accidental
    if (head.accidental.force || (!no_accidental && staffctx.acc_visible(head)))    // check for visibility
    {
        // create attachable
        target.attachables.push_back(Plate::pNote::AttachablePtr(new Plate::pAttachable(head.accidental, unscaledPos)));
        
        // get sprite-id
        target.attachables.back()->sprite = Pick::sprite_id(sprites, head.accidental);
        
        // get sprite and calculate scale factor
        const SpriteInfo& sprite = sprites[target.attachables.back()->sprite];
        double scale  = head_height /
                        (sprites.head_height(target.attachables.back()->sprite) * 1000.0);
        double head_width = 1000.0 * scale * sprites.head_width(target.sprite);
               scale *= head.accidental.appearance.scale / 1000.0;
        
        // calculate sprite position offset
        Position<mpx_t> anchor;                                 // anchor position instance
        mpx_t offset;                                           // default accidental offset (in pohw)
        anchor.x = _round(sprite.get_real("anchor.x") * 1000 * scale);
        anchor.y = _round(sprite.get_real("anchor.y") * 1000 * scale);
        offset   = _round(sprite.get_real("offset") * 1000 * scale);
        
        // apply position offset
        target.attachables.back()->absolutePos.x -= _round((head.accidental.offset_x * head_width) / 1000.0);
        target.attachables.back()->absolutePos.x -= _round(sprite.width * 1000 * scale + anchor.x + offset);
        target.attachables.back()->absolutePos.y -= anchor.y;
        
        // apply offsets due to sprite scaling
        target.attachables.back()->absolutePos.y += _round(((1.0 - head.accidental.appearance.scale / 1000.0) * head_height) / 2.0);
        
        // add accidental space
        if (engraver.has_accidental_space())
            engraver.add_offset(_round(sprite.width * scale * engraver.get_parameters().accidental_space));
        
        // adjust distances (for accidentals, which are too close to the previous object)
        if (voice.context.has_buffer())    // if there is a previous object
        {
            // check type of this object
            if (voice.context.get_buffer()->is(Class::CLEF) ||        // clef
                voice.context.get_buffer()->is(Class::KEY)  ||        // key signature and
                voice.context.get_buffer()->is(Class::TIMESIG))       // time signature
            {
                // calculate minimal position regarding the previous object
                const mpx_t minpos = voice.context.get_buffer_xpos() + engraver.min_distance();
                
                // if it's too close to the current object...
                if (minpos > target.attachables.back()->absolutePos.x)
                {
                    // ...add neccessary distance
                    engraver.add_offset(minpos - target.attachables.back()->absolutePos.x);
                };
            }
            else if (voice.context.get_buffer()->is(Class::BARLINE))    // barline
            {
                // calculate minimal position regarding the previous object
                const mpx_t minpos = voice.context.get_buffer_xpos() + engraver.barline_distance();
                
                // if it's too close to the current object...
                if (minpos > target.attachables.back()->absolutePos.x)
                {
                    // ...add neccessary distance
                    engraver.add_offset(minpos - target.attachables.back()->absolutePos.x);
                };
            }
            else if (voice.context.get_buffer()->is(Class::NOTEOBJECT))    // regular note-object
            {
                if (voice.notes.end() != ++voice.notes.begin())    // if there is a previous note in this line
                {
                    // calculate minimal position regarding the previous object
                    const mpx_t minpos = voice.context.get_buffer_xpos() + engraver.min_distance();
                    
                    // if it's too close to the current object...
                    if (minpos > target.attachables.back()->absolutePos.x)
                    {
                        // ...add neccessary distance
                        engraver.add_offset(minpos - target.attachables.back()->absolutePos.x);
                    };
                };
            };
        }
        else if (target.attachables.back()->absolutePos.x < engraver.min_distance())
        {
            // add neccessary distance
            engraver.add_offset(engraver.min_distance() - target.attachables.back()->absolutePos.x);
        };
        
        // calculate the accidental's graphical boundaries
        target.attachables.back()->gphBox.pos = target.attachables.back()->absolutePos;
        target.attachables.back()->gphBox.width = _round(sprite.width * 1000 * scale);
        target.attachables.back()->gphBox.height = _round(sprite.height * 1000 * scale);
    };
    
    // return cluster-indicator
    return got_cluster;
}

// engraving method (Chord)
void Chord::engrave(EngraverState& engraver) const
{
    // get data
          Plate::pNote&  pnote       = engraver.get_target();
          Plate::pVoice& pvoice      = engraver.get_target_voice();
    const Staff&         staff       = engraver.get_staff();
          StaffContext&  src_context = engraver.get_target_line().staffctx[&staff];
          StaffContext&  gph_context = engraver.get_target_line().staffctx[&engraver.get_visual_staff()];
    const Sprites&       sprites     = engraver.get_sprites();
    const ViewportParam& viewport    = engraver.get_viewport();
    const mpx_t&         head_height = engraver.get_head_height();
    
    // get sprite
    pnote.sprite = Pick::sprite_id(sprites, this);
    const SpriteInfo& headsprite = sprites[pnote.sprite];    // get sprite-info
    
    // calculate scale factors
    const double scale = head_height / (sprites.head_height(pnote.sprite) * 1000.0);
    const double chord_scale = this->appearance.scale / 1000.0;
    
    // get sprite attributes
    Position<mpx_t> anchor;                  // anchor position
    Position<mpx_t> stem;                    // stem position
    anchor.x = _round(headsprite.get_real("anchor.x") * scale * this->appearance.scale);
    anchor.y = _round(headsprite.get_real("anchor.y") * scale * this->appearance.scale);
    stem.x   = _round(headsprite.get_real("stem.x") * scale * this->appearance.scale);
    stem.y   = _round(headsprite.get_real("stem.y") * scale * this->appearance.scale);
    
    // get dot sprite and distance
    const SpriteInfo& dotsprite =
        (sprites[pnote.sprite.setid].dot != UNDEFINED) ?
            sprites[pnote.sprite.setid][sprites[pnote.sprite.setid].dot] :
            sprites[pnote.sprite.setid][sprites[pnote.sprite.setid].undefined_symbol];
    
    Position<mpx_t> dotanchor;  // anchor position
    mpx_t dotdistance = 0;      // dot distance
    if (this->val.dots)         // only get dot-attributes, if there are dots to be engraved
    {
        dotanchor.x = _round(dotsprite.get_real("anchor.x") * scale * this->appearance.scale);
        dotanchor.y = _round(dotsprite.get_real("anchor.y") * scale * this->appearance.scale);
        dotdistance = _round(dotsprite.get_real("distance") * scale * this->appearance.scale);
    };
    
    // engrave
    StemInfo stem_info;             // stem position information
    int max_ledger = 0;             // highest ledger count (ledger count above)
    int min_ledger = 0;             // lowest ledger count (ledger count below)
    int max_cluster_ledger = 0;     // highest ledger count of clustered note (ledger count above)
    int min_cluster_ledger = 0;     // lowest ledger count of clustered note (ledger count below)
    
    const mpx_t stem_width = viewport.umtopx_h(engraver.get_style().stem_width);    // get stem width
    const mpx_t stem_len = (this->stem_length * head_height) / 1000;                // calculate stem length
    const mpx_t sprite_width = _round(headsprite.width * scale * this->appearance.scale);
    const double head_width = sprites.head_width(pnote.sprite) * scale * 1000.0;
    
    // engrave heads
    if (stem_len >= 0)    // upward stem
    {
        // engrave heads
        bool _cluster = false;
        for (HeadList::const_iterator i = this->heads.begin(); i != this->heads.end(); ++i)
        {
            // engrave head
            _cluster = engrave_head(**i,                       // note head
                                    pnote,                     // note on the plate
                                    pnote.absolutePos.front()  // base note position
                                       - Position<mpx_t>(_round((anchor.x * (*i)->appearance.scale) / 1000.0),
                                                         _round((anchor.y * (*i)->appearance.scale) / 1000.0)),
                                    pvoice,                    // current voice
                                    src_context,               // current staff-context
                                    gph_context,               // current staff-context (visual)
                                    sprite_width,              // width of the notehead
                                    head_height,               // head height for the current staff
                                    chord_scale,               // scale of the chord
                                    engraver.has_tie(**i),     // no accidental for tied heads
                                    sprites,                   // sprite set
                                    engraver);                 // engraver instance
            
            stem_info.cluster |= _cluster;  // memorize cluster occurance for stem and ledger engraver
            
            // calculate graphical boundaries
            if (i == this->heads.begin())
                pnote.gphBox = Plate::pGraphical::Box(
                    pnote.absolutePos.back(),
                    _round(headsprite.width * scale * chord_scale * (*i)->appearance.scale),
                    _round((head_height * chord_scale * (*i)->appearance.scale) / 1000.0));
            else
                pnote.gphBox.extend(Plate::pGraphical::Box(
                    pnote.absolutePos.back(),
                    _round(headsprite.width * scale * chord_scale * (*i)->appearance.scale),
                    _round((head_height * chord_scale * (*i)->appearance.scale) / 1000.0)));
            
            // check for edge-heads
            if (pnote.absolutePos.back().y > stem_info.base_pos || i == this->heads.begin())
            {
                // we got a head lower than the current base (or just save the first occurring)
                stem_info.base_pos = pnote.absolutePos.back().y;    // memorize vertical base pos
                stem_info.base_scale = (*i)->appearance.scale;      // memorize scale
                stem_info.base_side = _cluster;                     // memorize side of the stem
            };
            if (pnote.absolutePos.back().y < stem_info.top_pos || i == this->heads.begin())
            {
                // we got a head higher than the current top (or just save the first occurring)
                stem_info.top_pos = pnote.absolutePos.back().y;     // memorize vertical base pos
                stem_info.top_scale = (*i)->appearance.scale;       // memorize scale
                stem_info.top_side = _cluster;                      // memorize side of the stem
            };
            
            // check for ledger counts
            const int ledgercnt = gph_context.ledger_count(**i);
            if (max_ledger < ledgercnt) max_ledger = ledgercnt;
            if (min_ledger > ledgercnt) min_ledger = ledgercnt;
            if (_cluster && max_cluster_ledger < ledgercnt) max_cluster_ledger = ledgercnt;
            if (_cluster && min_cluster_ledger > ledgercnt) min_cluster_ledger = ledgercnt;
            
            // remember accidental
            if (engraver.get_parameters().remember_accidentals && !engraver.has_tie(**i))
                gph_context.remember_acc(**i);
        };
        
        // correct accidental position and incorporate the accidental's graphical boundaries into the chord's
        Plate::pNote::AttachableList::iterator j = pnote.attachables.begin();
        for (Plate::pNote::AttachableList::iterator i = pnote.attachables.begin(); i != pnote.attachables.end(); ++i)
        {
            if (i != j && (*i)->gphBox.overlaps((*j)->gphBox))
            {
                (*i)->absolutePos.x -= (*j)->gphBox.width + 1000;
                (*i)->gphBox.pos.x  -= (*j)->gphBox.width + 1000;
            };
            
            pnote.gphBox.extend((*i)->gphBox);
            j = i;
        };
    }
    else                  // downward stem
    {
        // engrave heads
        bool _cluster = false;
        for (HeadList::const_reverse_iterator i = this->heads.rbegin(); i != this->heads.rend(); ++i)
        {
            // engrave head
            _cluster = engrave_head(**i,                       // note head
                                    pnote,                     // note on the plate
                                    pnote.absolutePos.front()  // base note position
                                       - Position<mpx_t>(_round((anchor.x * (*i)->appearance.scale) / 1000.0),
                                                         _round((anchor.y * (*i)->appearance.scale) / 1000.0)),
                                    pvoice,                    // current voice
                                    src_context,               // current staff-context
                                    gph_context,               // current staff-context (visual)
                                    -sprite_width,             // width of the notehead
                                    head_height,               // head height for the current staff
                                    chord_scale,               // scale of the chord
                                    engraver.has_tie(**i),     // no accidental for tied heads
                                    sprites,                   // sprite set
                                    engraver);                 // engraver instance
            
            stem_info.cluster |= _cluster;      // memorize cluster occurance for stem engraver
            
            // calculate graphical boundaries
            const Position<mpx_t>& newpos = *++pnote.absolutePos.begin(); // position of new head
            if (i == this->heads.rbegin())
                pnote.gphBox = Plate::pGraphical::Box(
                    newpos,
                    _round(headsprite.width * scale * chord_scale * (*i)->appearance.scale),
                    _round((head_height  * chord_scale * (*i)->appearance.scale) / 1000.0));
            else
                pnote.gphBox.extend(Plate::pGraphical::Box(
                    newpos,
                    _round(headsprite.width * scale * chord_scale * (*i)->appearance.scale),
                    _round((head_height  * chord_scale * (*i)->appearance.scale) / 1000.0)));
            
            // check for edge-heads
            if (newpos.y > stem_info.base_pos || i == this->heads.rbegin())
            {
                // we got a head lower than the current base (or just save the first occurring)
                stem_info.base_pos = newpos.y;                  // memorize vertical base pos
                stem_info.base_scale = (*i)->appearance.scale;  // memorize scale
                stem_info.base_side = _cluster;                 // memorize side of the stem
            };
            if (pnote.absolutePos.back().y < stem_info.top_pos || i == this->heads.rbegin())
            {
                // we got a head higher than the current top (or just save the first occurring)
                stem_info.top_pos = newpos.y;                   // memorize vertical base pos
                stem_info.top_scale = (*i)->appearance.scale;   // memorize scale
                stem_info.top_side = _cluster;                  // memorize side of the stem
            };
            
            // check for ledger counts
            const int ledgercnt = gph_context.ledger_count(**i);
            if (max_ledger < ledgercnt) max_ledger = ledgercnt;
            if (min_ledger > ledgercnt) min_ledger = ledgercnt;
            if (_cluster && max_cluster_ledger < ledgercnt) max_cluster_ledger = ledgercnt;
            if (_cluster && min_cluster_ledger > ledgercnt) min_cluster_ledger = ledgercnt;
            
            // remember accidental
            if (engraver.get_parameters().remember_accidentals && !engraver.has_tie(**i))
                gph_context.remember_acc(**i);
        };
        
        // correct accidental position and incorporate the accidental's graphical boundaries into the chord's
        Plate::pNote::AttachableList::reverse_iterator j = pnote.attachables.rbegin();
        for (Plate::pNote::AttachableList::reverse_iterator i = pnote.attachables.rbegin(); i != pnote.attachables.rend(); ++i)
        {
            if (stem_info.cluster)
            {
                (*i)->absolutePos.x -= sprite_width;
                (*i)->gphBox.pos.x  -= sprite_width;
            };
            
            if (i != j && (*i)->gphBox.overlaps((*j)->gphBox))
            {
                (*i)->absolutePos.x -= (*j)->gphBox.width + 1000;
                (*i)->gphBox.pos.x  -= (*j)->gphBox.width + 1000;
            };
            
            pnote.gphBox.extend((*i)->gphBox);
            j = i;
        };
    };
    
    // engrave stem
    if (stem_len >= 0)  // an upward stem is right of the chord
    {
        pnote.stem.x = pnote.absolutePos.front().x                      // base position
            + stem.x                                                    // offset for the sprite
            - _round(stem_info.cluster ? 0 : stem_width * scale / 2);   // offset for the stem's width
        
        pnote.stem.top = stem_info.top_pos + head_height / 2 - stem_len;
        pnote.stem.base = stem_info.base_pos + (stem_info.base_side ?
            _round(((headsprite.height * scale * this->appearance.scale - stem.y) * stem_info.base_scale) / 1000.0) :
            stem.y);
    }
    else                // downward stems are placed left of the chord
    {
        pnote.stem.x = pnote.absolutePos.front().x             // base position
         + _round(stem_info.cluster ? stem.x : sprite_width - stem.x + stem_width * scale / 2);
        
        pnote.stem.top = stem_info.base_pos + head_height / 2 - stem_len;
        pnote.stem.base = stem_info.top_pos + (stem_info.top_side ?
            stem.y :
            _round(((headsprite.height * scale * this->appearance.scale - stem.y) * stem_info.top_scale) / 1000.0));
    };
    pnote.gphBox.extend(Position<mpx_t>(pnote.stem.x, pnote.stem.top));
    
    // engrave dots
    {
    std::list< Position<mpx_t> >::iterator p;
    for (unsigned char j = 0; j < this->val.dots; ++j)
    {
        p = ++pnote.absolutePos.begin();
        for (HeadList::const_iterator i = this->heads.begin(); i != this->heads.end(); ++i, ++p)
        {
            pnote.dotPos.push_back(
                Position<mpx_t>(
                  _round(pnote.gphBox.right()
                          + dotdistance + ((*i)->dot_offset.x * head_width) / 1000.0   // offset
                          + j * (dotdistance + dotsprite.width * scale * this->appearance.scale)
                          - dotanchor.x),               // anchor | [above]: dot distance
                  _round(p->y
                          + ((*i)->dot_offset.y * head_height) / 1000.0 // offset
                          + (gph_context.on_line(**i) ? // line collision
                                ((stem_len > 0) ? head_height : 0) :
                                head_height / 2.0)
                          - dotanchor.y)                // anchor
                )
            );
        };
    };
    };
    
    // engrave ledger lines
    {
    const mpx_t ledger_length = engraver.get_style().ledger_length;
    if (min_ledger < 0)         // if there are ledger lines above the staff
    {
        if (min_cluster_ledger < 0)           // engrave wide ledger lines
        {
            pnote.ledgers.push_back(Plate::pNote::LedgerLines());
            pnote.ledgers.back().count = -min_cluster_ledger;
            pnote.ledgers.back().length = _round(((ledger_length + 1000) * head_width) / 1000.0);
            pnote.ledgers.back().basepos = pnote.absolutePos.front();
            pnote.ledgers.back().basepos.x -= _round((head_width * (ledger_length - 1000)) / 2000.0);
            pnote.ledgers.back().basepos.y -= head_height;
            pnote.ledgers.back().below = false;
        } else min_cluster_ledger = 0;
        if (min_cluster_ledger > min_ledger)  // engrave normal ledger lines
        {
            pnote.ledgers.push_back(Plate::pNote::LedgerLines());
            pnote.ledgers.back().count = min_cluster_ledger - min_ledger;
            pnote.ledgers.back().length = _round((ledger_length * head_width) / 1000.0);
            pnote.ledgers.back().basepos = pnote.absolutePos.front();
            pnote.ledgers.back().basepos.x -= _round((head_width * (ledger_length + (stem_info.cluster && stem_len < 0 ? -3000 : -1000))) / 2000.0);
            pnote.ledgers.back().basepos.y -= head_height;
            pnote.ledgers.back().below = false;
        };
    };
    max_ledger -= staff.line_count - 1;       // regard existing stafflines
    max_cluster_ledger -= staff.line_count - 1;
    if (max_ledger > 0)    // if there are ledger lines below the staff
    {
        if (max_cluster_ledger > 0)          // engrave wide ledger lines
        {
            pnote.ledgers.push_back(Plate::pNote::LedgerLines());
            pnote.ledgers.back().count = max_cluster_ledger;
            pnote.ledgers.back().length = _round(((ledger_length + 1000) * head_width) / 1000.0);
            pnote.ledgers.back().basepos = pnote.absolutePos.front();
            pnote.ledgers.back().basepos.x -= _round((head_width * (ledger_length - 1000)) / 2000.0);
            pnote.ledgers.back().basepos.y += staff.line_count * head_height;
            pnote.ledgers.back().below = true;
        } else max_cluster_ledger = 0;
        if (max_cluster_ledger < max_ledger) // engrave normal ledger lines
        {
            pnote.ledgers.push_back(Plate::pNote::LedgerLines());
            pnote.ledgers.back().count = max_ledger - max_cluster_ledger;
            pnote.ledgers.back().length = _round((ledger_length * head_width) / 1000.0);
            pnote.ledgers.back().basepos = pnote.absolutePos.front();
            pnote.ledgers.back().basepos.x -= _round((head_width * (ledger_length + (stem_info.base_side ? -3000 : -1000))) / 2000.0);
            pnote.ledgers.back().basepos.y += staff.line_count * head_height;
            pnote.ledgers.back().below = true;
        };
    };
    };
    
    // engrave ties (from previous note)
    {
    std::list< Position<mpx_t> >::iterator p = ++pnote.absolutePos.begin();
    for (HeadList::const_iterator i = this->heads.begin(); i != this->heads.end(); ++i, ++p)
    {
        if (!engraver.has_tie(**i)) continue;
        TieInfo& tieinfo = engraver.get_tieinfo(**i);
        if (tieinfo.target != NULL)    // if the tie is not broken
        {
            // calculate normal tie position
            tieinfo.target->pos2 = *p + (tieinfo.source->offset2 * head_height) / 1000;
            tieinfo.target->pos2.y += head_height;
            tieinfo.target->control2 = tieinfo.target->pos2 + (tieinfo.source->control2 * head_height) / 1000;
        }
        else    // if the tie is broken
        {
            // create new tie on this note (using TieInfo::refPos)
            // (swap refpoint positions to indicate as second half to justification algorithm)
            if (p->x - tieinfo.refPos < engraver.min_distance())
                engraver.add_offset(engraver.min_distance() + tieinfo.refPos - p->x);
            pnote.ties.push_back(Plate::pNote::Tie());
            pnote.ties.back().pos1 = *p + (tieinfo.source->offset2 * head_height) / 1000;
            pnote.ties.back().pos1.y += head_height;
            pnote.ties.back().control1 = pnote.ties.back().pos1 + (tieinfo.source->control2 * head_height) / 1000;
            pnote.ties.back().pos2.x = tieinfo.refPos - (tieinfo.source->offset2.x * head_height) / 1000;
            pnote.ties.back().pos2.y = p->y + head_height + (tieinfo.source->offset2.y * head_height) / 1000;
            pnote.ties.back().control2.x = pnote.ties.back().pos2.x - (tieinfo.source->control2.x * head_height) / 1000;
            pnote.ties.back().control2.y = pnote.ties.back().pos2.y + (tieinfo.source->control2.y * head_height) / 1000;
        };
    };
    };
    
    // engrave ties (on this note)
    {
    engraver.erase_tieinfo();                // erase tie-information (for the voice)
    HeadList::const_iterator head = this->heads.begin();                       // setup head iterator
    std::list< Position<mpx_t> >::iterator hpos = ++pnote.absolutePos.begin(); // setup head-position iterator
    const TiedHead* thead;
    while (head != this->heads.end() && hpos != pnote.absolutePos.end())       // for each head
    {
        if ((*head)->is(Class::TIEDHEAD))    // check if it has got a tie
        {
            thead = &static_cast<TiedHead&>(**head);
            
            // engrave tie
            pnote.ties.push_back(Plate::pNote::Tie());    // add tie to on-plate object
            
            // base position
            pnote.ties.back().pos1.x = _round(hpos->x
                                               + headsprite.width * scale * this->appearance.scale
                                               + (thead->offset1.x * head_height) / 1000.0);
            pnote.ties.back().pos1.y = _round(hpos->y
                                               + (thead->offset1.y * head_height) / 1000.0)
                                               + head_height;
            
            // first control point
            pnote.ties.back().control1.x = pnote.ties.back().pos1.x
                                         + (thead->control1.x * head_height) / 1000;
            pnote.ties.back().control1.y = pnote.ties.back().pos1.y
                                         + (thead->control1.y * head_height) / 1000;
            
            // set tie-information
            engraver.add_tieinfo(*thead);
        };
        
        ++head;    // goto the next head
        ++hpos;
    };
    };
    
    // prepare beam engraving
    engraver.engrave_beam(*this, stem_info);
    
    // engrave articulation symbols
    {
    Position<mpx_t> artanchor;      // articulation symbol anchor position
    mpx_t artoffset;                // articulation symbol y-offset
    for (ArticulationList::const_iterator i = this->articulation.begin(); i != this->articulation.end(); ++i)
    {
        // get articulation symbol sprite
        const SpriteInfo& artsprite = sprites[i->sprite.ready() ?
                                                    i->sprite :
                                                    SpriteId(0, sprites[0].undefined_symbol)];
        
        // get sprite anchor
        artanchor.x = _round(artsprite.get_real("anchor.x") * scale * this->appearance.scale);
        artanchor.y = _round(artsprite.get_real("anchor.y") * scale * this->appearance.scale);
        artoffset   = _round(artsprite.get_real("offset") * scale * this->appearance.scale);
        
        // create attachable
        pnote.attachables.push_back(
            Plate::pNote::AttachablePtr(
                new Plate::pAttachable(*i,  // articulation object
                    Position<mpx_t>(        // calculate position
                        _round(   pnote.absolutePos.begin()->x
                                + headsprite.width * scale * chord_scale * 500
                                - (artanchor.x * i->appearance.scale) / 1000.0),
                        (stem_len >= 0) ?
                            // upward stem and far symbol (i.e. on top of the stem)
                            (i->far ? _round(pnote.stem.top
                                           - artsprite.height * scale * chord_scale * i->appearance.scale
                                           - (i->offset_y * head_height) / 1000.0
                                           - artoffset
                                           - (artanchor.y * i->appearance.scale) / 1000.0)
                            // upward stem and normal symbol
                                    : _round(stem_info.base_pos
                                           + head_height
                                           + (i->offset_y * head_height) / 1000.0
                                           + artoffset
                                           - (artanchor.y * i->appearance.scale) / 1000.0)
                            )
                                        :
                            // downward stem and far symbol (i.e. on top of the stem)
                            (i->far ? _round(pnote.stem.top
                                           + (i->offset_y * head_height) / 1000.0
                                           + artoffset
                                           - (artanchor.y * i->appearance.scale) / 1000.0)
                            // downward stem and normal symbol
                                    : _round(stem_info.base_pos
                                           - artsprite.height * scale * chord_scale * i->appearance.scale
                                           - (i->offset_y * head_height) / 1000.0
                                           - artoffset
                                           - (artanchor.y * i->appearance.scale) / 1000.0)
                            )
                    )
                )
            )
        );
        
        // save sprite to plate
        pnote.attachables.back()->sprite = i->sprite.ready() ?
                                               i->sprite :
                                               SpriteId(0, sprites[0].undefined_symbol);
        
        // calculate graphical boundaries
        pnote.attachables.back()->gphBox.pos = pnote.attachables.back()->absolutePos;
        pnote.attachables.back()->gphBox.width = _round(artsprite.width * scale * chord_scale * i->appearance.scale);
        pnote.attachables.back()->gphBox.height = _round(artsprite.height * scale * chord_scale * i->appearance.scale);
    };
    };
    
    // check for flags (for additional distance)
    if (this->val.exp < VALUE_BASE - 2 &&        // if this note can have a flag
        !this->beam &&                           // and this note does not have a beam
          (!pvoice.context.has_buffer() ||       // and the previous note did not have a beam either
           !pvoice.context.get_buffer()->is(Class::CHORD) ||
           !static_cast<const Chord*>(pvoice.context.get_buffer())->beam)
        )                                        // then this note has a flag
    {
        // get flag sprite information
        const SpriteInfo& flagsprite =
            (sprites[pnote.sprite.setid].flags_note != UNDEFINED) ?
                sprites[pnote.sprite.setid][sprites[pnote.sprite.setid].flags_note] :
                sprites[pnote.sprite.setid][sprites[pnote.sprite.setid].undefined_symbol];
        
        mpx_t flag_width = _round(    (flagsprite.width - flagsprite.get_real("anchor.x"))
                                    * scale * this->appearance.scale);
        
        // extend graphical boundary box
        pnote.gphBox.extend(Position<mpx_t>(pnote.stem.x + flag_width, pnote.stem.top));
    };
}

// engraving method (Rest)
void Rest::engrave(EngraverState& engraver) const
{
    // get data
          Plate::pNote&  pnote       = engraver.get_target();
    const Sprites&       sprites     = engraver.get_sprites();
    const mpx_t&         head_height = engraver.get_head_height();
    
    // get sprite and position-offset
    pnote.sprite = Pick::sprite_id(sprites, this);
    const SpriteInfo& restsprite = sprites[pnote.sprite];  // get sprite-info
    const double scale = head_height /                     // calculate scale factor
                         (sprites.head_height(pnote.sprite) * 1000.0);
    
    const mpx_t sprite_width = _round(restsprite.width * scale * this->appearance.scale);
    const mpx_t sprite_height = _round(restsprite.height * scale * this->appearance.scale);
    const double head_width = sprites.head_width(pnote.sprite) * scale * 1000.0;
    
    Position<mpx_t> anchor;        // anchor position
    int baseline = 0;              // base line
    mpx_t slope = 0;               // stem slope
    mpx_t minlen = head_height;    // stem length
    mpx_t stembottomx1 = 0;        // stem bottom position (left horizontal)
    
    anchor.x = _round(restsprite.get_real("anchor.x") * scale * this->appearance.scale);
    anchor.y = _round(restsprite.get_real("anchor.y") * scale * this->appearance.scale);
    baseline = restsprite.get_integer("line");
    slope    = _round(restsprite.get_real("slope") * scale * this->appearance.scale);
    minlen   = _round(restsprite.get_real("stem.minlen") * scale * this->appearance.scale);
    stembottomx1 = _round(restsprite.get_real("stem.bottom.x1") * scale * this->appearance.scale);
    
    // get dot sprite and distance
    const SpriteInfo& dotsprite =
        (sprites[pnote.sprite.setid].dot != UNDEFINED) ?
            sprites[pnote.sprite.setid][sprites[pnote.sprite.setid].dot] :
            sprites[pnote.sprite.setid][sprites[pnote.sprite.setid].undefined_symbol];
    
    Position<mpx_t> dotanchor;  // anchor position
    mpx_t dotdistance = 0;      // dot distance
    mpx_t dotoffset = 0;        // dot offset (in promille of head-width)
    if (this->val.dots > 0)     // only get dot-attributes, if there are dots to be engraved
    {
        dotanchor.x = _round(dotsprite.get_real("anchor.x") * scale * this->appearance.scale);
        dotanchor.y = _round(dotsprite.get_real("anchor.y") * scale * this->appearance.scale);
        dotdistance = _round(dotsprite.get_real("distance") * scale * this->appearance.scale);
        dotoffset   = _round(dotsprite.get_real("offset") * scale * this->appearance.scale);
    };
    
    // apply sprite-offset (x-offset applied by "apply_offsets()")
    pnote.absolutePos.front() -= anchor;
    pnote.absolutePos.front().y += (head_height * this->offset_y) / 1000;
    pnote.absolutePos.front().y += (head_height * baseline) / 2;
    
    // calculate graphical boundaries
    pnote.gphBox.pos = pnote.absolutePos.front();
    pnote.gphBox.width = sprite_width;
    pnote.gphBox.height = sprite_height;
    
    // calculate position of rest-flags
    if (this->val.exp < VALUE_BASE - 2)    // if the rest is flagged
    {
        // correct position of topmost rest-flag
        pnote.absolutePos.front().y -= head_height * ((VALUE_BASE - 3 - this->val.exp) / 2);
        
        // add further flags
        for (size_t i = this->val.exp; i < VALUE_BASE - 3; ++i)
        {
            pnote.absolutePos.push_back(pnote.absolutePos.front());    // get position
            pnote.absolutePos.back().y += head_height * (i - this->val.exp + 1);
            
            // Annotation: boundary box update during vertical adjustment (see below)
        };
        
        // add position of bottom piece
        pnote.absolutePos.push_back(pnote.absolutePos.back());
        pnote.absolutePos.back().y += minlen;
        pnote.absolutePos.back().x += sprite_width - slope;
        pnote.absolutePos.back().x += stembottomx1;
        
        // calculate graphical boundaries
        pnote.gphBox.pos = pnote.absolutePos.back();
        pnote.gphBox.width = 0;
        pnote.gphBox.height = 0;
        
        // adjust vertical flag-positions
        for (std::list< Position<mpx_t> >::iterator i = pnote.absolutePos.begin(); i != --pnote.absolutePos.end(); ++i)
        {
            i->x += (slope * (pnote.absolutePos.back().y - i->y)) / (pnote.absolutePos.back().y - pnote.absolutePos.front().y) - 1;
            
            // update boundary box
            pnote.gphBox.extend(Plate::pGraphical::Box(*i, sprite_width, sprite_height));
        };
    };
    
    // erase tie information
    engraver.erase_tieinfo();
    
    // engrave dots
    for (unsigned char j = 0; j < this->val.dots; ++j)
    {
        pnote.dotPos.push_back(
            Position<mpx_t>(
                _round(
                   pnote.gphBox.right()
                 + dotoffset + (this->dot_offset.x * head_width) / 1000.0   // offset
                 + j * (dotdistance + dotsprite.width * scale    // dot distance
                                      * this->appearance.scale)
                 - dotanchor.x),                   // anchor
                _round(
                   pnote.absolutePos.begin()->y
                 + anchor.y                        // base line
                 - ((this->val.exp < VALUE_BASE - 2) ? 0 : head_height / 2.0)
                 + (this->dot_offset.y * head_height) / 1000.0    // offset
                 - dotanchor.y)                    // anchor
            )
        );
    };
}

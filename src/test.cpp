
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

#include "test.hh"

#define JUSTIFY       false
#define FORCE_JUSTIFY false

using namespace ScorePress;

static void add(Staff& voice, unsigned char exp, int tone, int stem_length, Chord::BeamType beam = Chord::NO_BEAM);
static void add(SubVoice& voice, unsigned char exp, int tone, int stem_length, Chord::BeamType beam = Chord::NO_BEAM);
static void add(Staff& voice, unsigned char exp, int tone, int stem_length,
                int x1, int y1, int cx1, int cy1, int cx2, int cy2, int x2, int y2, Chord::BeamType beam = Chord::NO_BEAM);
//static void add(SubVoice& voice, unsigned char exp, int tone, int stem_length,
//                int x1, int y1, int cx1, int cy1, int cx2, int cy2, int x2, int y2, Chord::BeamType beam = Chord::NO_BEAM);
//static void addr(Staff& voice, unsigned char exp, int offset_y = 0);
static void addr(SubVoice& voice, unsigned char exp, int offset_y = 0);
static void add_head(Staff& voice, int tone);
static void add_head(SubVoice& voice, int tone);
static void add_accidental(Staff& voice, Accidental::Type type, int offset_x);
static void add_accidental(SubVoice& voice, Accidental::Type type, int offset_x);
static void add_articulation(Staff& voice, const SpriteId& sprite, int offset_y, bool far = false);
static void add_articulation(SubVoice& voice, const SpriteId& sprite, int offset_y, bool far = false);
static void add_newline(Staff& voice, unsigned int distance, int indent = 0, int right_margin = 0, bool justify = false, bool force = false);
static void add_newline(SubVoice& voice, unsigned int distance, int indent = 0, int right_margin = 0, bool justify = false, bool force = false);
//static void add_pagebreak(Staff& voice, unsigned int distance, int indent = 0, int right_margin = 0, bool justify = false, bool force = false);
//static void add_pagebreak(SubVoice& voice, unsigned int distance, int indent = 0, int right_margin = 0, bool justify = false, bool force = false);
static void add1(Staff& staff, const Sprites& sprites, int toneoffset = 0);
static void add2(Staff& staff, const Sprites& sprites, int toneoffset = 0, int staffdist = 0);

static void add(Staff& voice, unsigned char exp, int tone, int stem_length, Chord::BeamType beam)
{
    voice.notes.push_back(StaffObjectPtr(new Chord()));
    static_cast<Chord&>(*voice.notes.back()).val.exp = exp & 0x0F;
    static_cast<Chord&>(*voice.notes.back()).heads.push_back(HeadPtr(new Head()));
    static_cast<Chord&>(*voice.notes.back()).heads.back()->tone = static_cast<tone_t>(tone);
    static_cast<Chord&>(*voice.notes.back()).stem_length = stem_length * 500;
    static_cast<Chord&>(*voice.notes.back()).beam = beam;
}

static void add(SubVoice& voice, unsigned char exp, int tone, int stem_length, Chord::BeamType beam)
{
    voice.notes.push_back(VoiceObjectPtr(new Chord()));
    static_cast<Chord&>(*voice.notes.back()).val.exp = exp & 0x0F;
    static_cast<Chord&>(*voice.notes.back()).heads.push_back(HeadPtr(new Head()));
    static_cast<Chord&>(*voice.notes.back()).heads.back()->tone = static_cast<tone_t>(tone);
    static_cast<Chord&>(*voice.notes.back()).stem_length = stem_length * 500;
    static_cast<Chord&>(*voice.notes.back()).beam = beam;
}

static void add(Staff& voice, unsigned char exp, int tone, int stem_length,
                int x1, int y1, int cx1, int cy1, int cx2, int cy2, int x2, int y2, Chord::BeamType beam)
{
    voice.notes.push_back(StaffObjectPtr(new Chord()));
    static_cast<Chord&>(*voice.notes.back()).val.exp = exp & 0x0F;
    static_cast<Chord&>(*voice.notes.back()).heads.push_back(HeadPtr(new TiedHead()));
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).tone = static_cast<tone_t>(tone);
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).offset1.x = x1;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).offset1.y = y1;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).control1.x = cx1;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).control1.y = cy1;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).control2.x = cx2;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).control2.y = cy2;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).offset2.x = x2;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).offset2.y = y2;
    static_cast<Chord&>(*voice.notes.back()).stem_length = stem_length * 500;
    static_cast<Chord&>(*voice.notes.back()).beam = beam;
}
/*
static void add(SubVoice& voice, unsigned char exp, int tone, int stem_length,
                int x1, int y1, int cx1, int cy1, int cx2, int cy2, int x2, int y2, Chord::BeamType beam)
{
    voice.notes.push_back(VoiceObjectPtr(new Chord()));
    static_cast<Chord&>(*voice.notes.back()).val.exp = exp & 0x0F;
    static_cast<Chord&>(*voice.notes.back()).heads.push_back(HeadPtr(new TiedHead()));
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).tone = static_cast<tone_t>(tone);
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).offset1.x = x1;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).offset1.y = y1;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).control1.x = cx1;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).control1.y = cy1;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).control2.x = cx2;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).control2.y = cy2;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).offset2.x = x2;
    static_cast<TiedHead&>(*static_cast<Chord&>(*voice.notes.back()).heads.back()).offset2.y = y2;
    static_cast<Chord&>(*voice.notes.back()).stem_length = stem_length * 500;
    static_cast<Chord&>(*voice.notes.back()).beam = beam;
}

static void addr(Staff& voice, unsigned char exp, int offset_y)
{
    voice.notes.push_back(StaffObjectPtr(new Rest()));
    static_cast<Rest&>(*voice.notes.back()).val.exp = exp & 0x0F;
    static_cast<Rest&>(*voice.notes.back()).offset_y = offset_y;
}
*/
static void addr(SubVoice& voice, unsigned char exp, int offset_y)
{
    voice.notes.push_back(VoiceObjectPtr(new Rest()));
    static_cast<Rest&>(*voice.notes.back()).val.exp = exp & 0x0F;
    static_cast<Rest&>(*voice.notes.back()).offset_y = offset_y;
}

static void add_head(Staff& voice, int tone)
{
    static_cast<Chord&>(*voice.notes.back()).heads.push_back(HeadPtr(new Head()));
    static_cast<Chord&>(*voice.notes.back()).heads.back()->tone = static_cast<tone_t>(tone);
}

static void add_head(SubVoice& voice, int tone)
{
    static_cast<Chord&>(*voice.notes.back()).heads.push_back(HeadPtr(new Head()));
    static_cast<Chord&>(*voice.notes.back()).heads.back()->tone = static_cast<tone_t>(tone);
}

static void add_accidental(Staff& voice, Accidental::Type type, int offset_x)
{
    static_cast<Chord&>(*voice.notes.back()).heads.back()->accidental.type = type;
    static_cast<Chord&>(*voice.notes.back()).heads.back()->accidental.offset_x = offset_x;
}

static void add_accidental(SubVoice& voice, Accidental::Type type, int offset_x)
{
    static_cast<Chord&>(*voice.notes.back()).heads.back()->accidental.type = type;
    static_cast<Chord&>(*voice.notes.back()).heads.back()->accidental.offset_x = offset_x;
}

static void add_articulation(Staff& voice, const SpriteId& sprite, int offset_y, bool far)
{
    static_cast<Chord&>(*voice.notes.back()).articulation.push_back(Articulation());
    static_cast<Chord&>(*voice.notes.back()).articulation.back().sprite = sprite;
    static_cast<Chord&>(*voice.notes.back()).articulation.back().offset_y = offset_y;
    static_cast<Chord&>(*voice.notes.back()).articulation.back().far = far;
}

static void add_articulation(SubVoice& voice, const SpriteId& sprite, int offset_y, bool far)
{
    static_cast<Chord&>(*voice.notes.back()).articulation.push_back(Articulation());
    static_cast<Chord&>(*voice.notes.back()).articulation.back().sprite = sprite;
    static_cast<Chord&>(*voice.notes.back()).articulation.back().offset_y = offset_y;
    static_cast<Chord&>(*voice.notes.back()).articulation.back().far = far;
}

static void add_newline(Staff& voice, unsigned int distance, int indent, int right_margin, bool justify, bool force)
{
    voice.notes.push_back(StaffObjectPtr(new Newline()));
    static_cast<Newline&>(*voice.notes.back()).distance = distance;
    static_cast<Newline&>(*voice.notes.back()).indent = indent;
    static_cast<Newline&>(*voice.notes.back()).right_margin = right_margin;
    static_cast<Newline&>(*voice.notes.back()).justify = justify;
    static_cast<Newline&>(*voice.notes.back()).forced_justification = force;
}

static void add_newline(SubVoice& voice, unsigned int distance, int indent, int right_margin, bool justify, bool force)
{
    voice.notes.push_back(VoiceObjectPtr(new Newline()));
    static_cast<Newline&>(*voice.notes.back()).distance = distance;
    static_cast<Newline&>(*voice.notes.back()).indent = indent;
    static_cast<Newline&>(*voice.notes.back()).right_margin = right_margin;
    static_cast<Newline&>(*voice.notes.back()).justify = justify;
    static_cast<Newline&>(*voice.notes.back()).forced_justification = force;
}
/*
static void add_pagebreak(Staff& voice, unsigned int distance, int indent, int right_margin, bool justify, bool force)
{
    voice.notes.push_back(StaffObjectPtr(new Pagebreak()));
    static_cast<Pagebreak&>(*voice.notes.back()).distance = distance;
    static_cast<Pagebreak&>(*voice.notes.back()).indent = indent;
    static_cast<Pagebreak&>(*voice.notes.back()).right_margin = right_margin;
    static_cast<Pagebreak&>(*voice.notes.back()).justify = justify;
    static_cast<Pagebreak&>(*voice.notes.back()).forced_justification = force;
}

static void add_pagebreak(SubVoice& voice, unsigned int distance, int indent, int right_margin, bool justify, bool force)
{
    voice.notes.push_back(VoiceObjectPtr(new Pagebreak()));
    static_cast<Pagebreak&>(*voice.notes.back()).distance = distance;
    static_cast<Pagebreak&>(*voice.notes.back()).indent = indent;
    static_cast<Pagebreak&>(*voice.notes.back()).right_margin = right_margin;
    static_cast<Pagebreak&>(*voice.notes.back()).justify = justify;
    static_cast<Pagebreak&>(*voice.notes.back()).forced_justification = force;
}
*/
static void add1(Staff& staff, const Sprites& sprites, int toneoffset)
{
    // main-voice notes
    add(staff, VALUE_BASE-2, 69 + toneoffset, 6);
    static_cast<Chord&>(*staff.notes.back()).subvoice = RefPtr<SubVoice>(new SubVoice(staff));
    SubVoice& subvoice = *static_cast<Chord&>(*staff.notes.back()).subvoice;
    subvoice.stem_direction = Voice::STEM_DOWNWARDS;
    static_cast<Chord&>(*staff.notes.back()).attached.push_back(MovablePtr(new Slur()));
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).duration = 1u << VALUE_BASE;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position.orig.x = UnitPosition<>::NOTE;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position.orig.y = UnitPosition<>::STAFF;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position.unit.x = UnitPosition<>::HEAD;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position.unit.y = UnitPosition<>::HEAD;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position.co.x = 0;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position.co.y = 0;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).control1
         = static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).control1.co.x = 2000;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).control1.co.y = -3000;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).control2
         = static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).control2.co.x = -2000;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).control2.co.y = -3000;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).end =
           static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).end.co.x = 0;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).end.co.y = 0;
    add(staff, VALUE_BASE-2, 70 + toneoffset, 6, 100, -700, 1500, -600, -1500, -600, -100, -700);
    add_accidental(staff, Accidental::flat, 0);
    add_articulation(staff, SpriteId(0, sprites.front().index("articulation.tenuto")), 0, true);
    add(staff, VALUE_BASE-3, 70 + toneoffset, 6, Chord::NO_BEAM);
    add_accidental(staff, Accidental::flat, 0);
    /**/add(staff, VALUE_BASE-3, 69 + toneoffset, 6);
    add(staff, VALUE_BASE-2, 72 + toneoffset, 6);
    add_articulation(staff, SpriteId(0, sprites.front().index("articulation.marcato")), 0, true);
    
    // sub-voice notes
    add(subvoice, VALUE_BASE-2, 65 + toneoffset, -6);
    add(subvoice, VALUE_BASE-2, 63 + toneoffset, -6);
    add_accidental(subvoice, Accidental::flat, 500);
    add(subvoice, VALUE_BASE-2, 67 + toneoffset, -6);
    add(subvoice, VALUE_BASE-2, 67 + toneoffset, -6, Chord::NO_BEAM);
    add_articulation(subvoice, SpriteId(0, sprites.front().index("articulation.marcato")), 0, true);
}

static void add2(Staff& staff, const Sprites& sprites, int toneoffset, int staffdist)
{
    // main-voice notes
    add(staff, VALUE_BASE-2, 69 + toneoffset, 6);
    static_cast<Chord&>(*staff.notes.back()).subvoice = RefPtr<SubVoice>(new SubVoice(staff));
    SubVoice& subvoice = *static_cast<Chord&>(*staff.notes.back()).subvoice;
    subvoice.stem_direction = Voice::STEM_DOWNWARDS;
    add_head(staff, 71 + toneoffset);
    add_articulation(staff, SpriteId(0, sprites.front().index("articulation.staccato")), 0, false);
    add(staff, VALUE_BASE-1, 71 + toneoffset, 6);
    /*
    add(staff, 5, 72 + toneoffset, 4);
    add_accidental(staff, Accidental::natural, 1200);
    add_head(staff, 74 + toneoffset);
    add_accidental(staff, Accidental::natural, 200);
    add_head(staff, 78 + toneoffset);
    add_accidental(staff, Accidental::sharp, 200);
    //*/
    add(staff, VALUE_BASE-2, 69 + toneoffset, 6, 0, -1100, 1500, -600, -800, -650, 0, -1100);
    add_newline(staff, staffdist, 0, 0, JUSTIFY, FORCE_JUSTIFY);
    //add_pagebreak(staff, staffdist, 0, 0, JUSTIFY, FORCE_JUSTIFY);
    
    // sub-voice notes
    addr(subvoice, VALUE_BASE-2, 3000);
    add(subvoice, VALUE_BASE-3, 65 + toneoffset, -6, Chord::FORCE_BEAM);
    /**/static_cast<Chord&>(*subvoice.notes.back()).val.dots = 1;
    static_cast<Chord&>(*subvoice.notes.back()).attached.push_back(MovablePtr(new Hairpin()));
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).duration = 3u << (VALUE_BASE-2);
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).position.orig.x = UnitPosition<>::NOTE;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).position.orig.y = UnitPosition<>::STAFF;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).position.unit.x = UnitPosition<>::HEAD;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).position.unit.y = UnitPosition<>::HEAD;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).position.co.x = 0;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).position.co.y = 8500;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).end
        = static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).position;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).end.co.x = 0;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).end.co.y = 8500;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).thickness = 1000;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).height = 800;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).crescendo = true;
    
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).appearance.scale = 1300;
    
    add_accidental(subvoice, Accidental::sharp, 0);
    add_head(subvoice, 66 + toneoffset);
    add_accidental(subvoice, Accidental::sharp, 0);
    //addr(subvoice, VALUE_BASE-4, 2000);
    add(subvoice, VALUE_BASE-4, 69 + toneoffset, -8);
    add(subvoice, VALUE_BASE-2, 67 + toneoffset, -6);
    add(subvoice, VALUE_BASE-2, 65 + toneoffset, -6);
    add_accidental(subvoice, Accidental::natural, 0);
    add_head(subvoice, 67 + toneoffset);
    add_accidental(subvoice, Accidental::natural, 250);
    add_newline(subvoice, staffdist, 0, 0, JUSTIFY, FORCE_JUSTIFY);
    //add_pagebreak(subvoice, staffdist, 0, 0, JUSTIFY, FORCE_JUSTIFY);
}

static void set_test(Document& document, const Sprites& sprites)
{
    // setup document
    document.head_height = 2000;       // µm
    document.stem_width = 250;         // µm
    document.add_attached(new TextArea(), 0);
    static_cast<TextArea*>(&*document.attached[0].back())->position.co.x = 55000;
    static_cast<TextArea*>(&*document.attached[0].back())->position.co.y = 0;
    static_cast<TextArea*>(&*document.attached[0].back())->width = 80000;
    static_cast<TextArea*>(&*document.attached[0].back())->height = 16000;
    static_cast<TextArea*>(&*document.attached[0].back())->text.push_back(Paragraph());
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().align = Paragraph::CENTER;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().justify = false;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.push_back(PlainText());
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().text = "   ScorePress   ";
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.family = "Liberation Serif";
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.size = 24;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.bold = true;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.italic = false;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.underline = true;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.color.r = 0;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.color.g = 0;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.color.b = 0;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.color.a = 255;
    static_cast<TextArea*>(&*document.attached[0].back())->text.push_back(Paragraph());
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().align = Paragraph::CENTER;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().justify = false;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.push_back(PlainText());
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().text = "Music Engraving Software";
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.family = "Liberation Serif";
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.size = 12;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.bold = false;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.italic = true;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.underline = false;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.color.r = 0;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.color.g = 0;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.color.b = 0;
    static_cast<TextArea*>(&*document.attached[0].back())->text.back().text.back().font.color.a = 255;
    
    // setup score
    document.scores.push_back(Document::Score(0));
    Score& score = document.scores.back().score;
    score.layout.dimension.width = 190000;      // µm
    score.layout.dimension.height = 297000;     // µm
    
    // setup staff
    score.staves.push_back(Staff());
    score.staves.back().offset_y = 5000;        // pohh
    score.staves.back().line_count = 5;
    score.staves.back().long_barlines = true;
    score.staves.back().curlybrace = true;
    score.staves.back().layout.indent = 10000;      // µm
    score.staves.back().layout.justify = JUSTIFY;
    score.staves.back().layout.distance = 10000;    // pohh
    //score.staves.back().layout.auto_clef = false;
    //score.staves.back().layout.auto_key = false;
    //score.staves.back().layout.auto_timesig = false;
    
    score.staves.back().notes.push_back(StaffObjectPtr(new Clef()));
    static_cast<Clef&>(*score.staves.back().notes.back()).sprite = SpriteId(0, sprites.front().index("clef.treble"));
    static_cast<Clef&>(*score.staves.back().notes.back()).base_note = static_cast<tone_t>(sprites.front().get("clef.treble").get_integer("basenote"));
    static_cast<Clef&>(*score.staves.back().notes.back()).line = static_cast<unsigned char>(sprites.front().get("clef.treble").get_integer("line"));
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_sharp = static_cast<tone_t>(sprites.front().get("clef.treble").get_integer("keybound.sharp"));
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_flat = static_cast<tone_t>(sprites.front().get("clef.treble").get_integer("keybound.flat"));
    //*
    score.staves.back().notes.push_back(StaffObjectPtr(new Key()));
    static_cast<Key&>(*score.staves.back().notes.back()).type = Key::SHARP;
    static_cast<Key&>(*score.staves.back().notes.back()).number = 4;
    //*/
    score.staves.back().notes.push_back(StaffObjectPtr(new CustomTimeSig()));
    static_cast<CustomTimeSig&>(*score.staves.back().notes.back()).number = 4;
    static_cast<CustomTimeSig&>(*score.staves.back().notes.back()).beat = 4;
    static_cast<CustomTimeSig&>(*score.staves.back().notes.back()).sprite = SpriteId(0, sprites.front().index("timesig.symbol_4_4_timesigC"));
    
    //*
    add1(score.staves.back(), sprites);
    add2(score.staves.back(), sprites, 0, 3000);
    add1(score.staves.back(), sprites);
    add2(score.staves.back(), sprites, 0, 3000);
    //*/
    score.staves.push_back(Staff());
    score.staves.back().offset_y = 6000;        // pohh
    score.staves.back().line_count = 5;
    score.staves.back().long_barlines = false;
    score.staves.back().curlybrace = false;
    score.staves.back().layout.indent = 10000;  // µm
    score.staves.back().layout.justify = JUSTIFY;
    score.staves.back().layout.distance = 0;    // pohh
    //score.staves.back().layout.auto_clef = false;
    //score.staves.back().layout.auto_key = false;
    //score.staves.back().layout.auto_timesig = false;
    
    score.staves.back().notes.push_back(StaffObjectPtr(new Clef()));
    static_cast<Clef&>(*score.staves.back().notes.back()).sprite = SpriteId(0, sprites.front().index("clef.bass"));
    static_cast<Clef&>(*score.staves.back().notes.back()).base_note = static_cast<tone_t>(sprites.front().get("clef.bass").get_integer("basenote"));
    static_cast<Clef&>(*score.staves.back().notes.back()).line = static_cast<unsigned char>(sprites.front().get("clef.bass").get_integer("line"));
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_sharp = static_cast<tone_t>(sprites.front().get("clef.bass").get_integer("keybound.sharp"));
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_flat = static_cast<tone_t>(sprites.front().get("clef.bass").get_integer("keybound.flat"));
    //*
    score.staves.back().notes.push_back(StaffObjectPtr(new Key()));
    static_cast<Key&>(*score.staves.back().notes.back()).type = Key::SHARP;
    static_cast<Key&>(*score.staves.back().notes.back()).number = 4;
    //*/
    score.staves.back().notes.push_back(StaffObjectPtr(new CustomTimeSig()));
    static_cast<CustomTimeSig&>(*score.staves.back().notes.back()).number = 4;
    static_cast<CustomTimeSig&>(*score.staves.back().notes.back()).beat = 4;
    static_cast<CustomTimeSig&>(*score.staves.back().notes.back()).sprite = SpriteId(0, sprites.front().index("timesig.symbol_4_4_timesigC"));
    //*
    add1(score.staves.back(), sprites, -24);
    add2(score.staves.back(), sprites, -24);
    add1(score.staves.back(), sprites, -24);
    add2(score.staves.back(), sprites, -24);
    //*/
}

const Document& Test::get_document(const Sprites& sprites)
{
    static Document document;
    if (document.scores.empty())
        set_test(document, sprites);
    return document;
}

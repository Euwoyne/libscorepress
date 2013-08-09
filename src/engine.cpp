
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

#include "engine.hh"
#include "log.hh"

#define JUSTIFY false

using namespace ScorePress;

void add(MainVoice& voice, unsigned char exp, int tone, int stem_length, Chord::BeamType beam = Chord::NO_BEAM);
void add(SubVoice& voice, unsigned char exp, int tone, int stem_length, Chord::BeamType beam = Chord::NO_BEAM);
void add(MainVoice& voice, unsigned char exp, int tone, int stem_length,
         int x1, int y1, int cx1, int cy1, int cx2, int cy2, int x2, int y2, Chord::BeamType beam = Chord::NO_BEAM);
void add(SubVoice& voice, unsigned char exp, int tone, int stem_length,
         int x1, int y1, int cx1, int cy1, int cx2, int cy2, int x2, int y2, Chord::BeamType beam = Chord::NO_BEAM);
void addr(MainVoice& voice, unsigned char exp, int offset_y = 0);
void addr(SubVoice& voice, unsigned char exp, int offset_y = 0);
void add_head(MainVoice& voice, int tone);
void add_head(SubVoice& voice, int tone);
void add_accidental(MainVoice& voice, Accidental::Type type, int offset_x);
void add_accidental(SubVoice& voice, Accidental::Type type, int offset_x);
void add_articulation(MainVoice& voice, const SpriteId& sprite, int offset_y, bool far = false);
void add_articulation(SubVoice& voice, const SpriteId& sprite, int offset_y, bool far = false);
void add_newline(MainVoice& voice, unsigned int distance, int indent = 0, int right_margin = 0, bool justify = false);
void add_newline(SubVoice& voice, unsigned int distance, int indent = 0, int right_margin = 0, bool justify = false);
void add1(Staff& staff, Sprites& sprites, int toneoffset = 0);
void add2(Staff& staff, Sprites& sprites, int toneoffset = 0, int staffdist = 0);

void add(MainVoice& voice, unsigned char exp, int tone, int stem_length, Chord::BeamType beam)
{
    voice.notes.push_back(StaffObjectPtr(new Chord()));
    static_cast<Chord&>(*voice.notes.back()).val.exp = exp & 0x0F;
    static_cast<Chord&>(*voice.notes.back()).heads.push_back(HeadPtr(new Head()));
    static_cast<Chord&>(*voice.notes.back()).heads.back()->tone = static_cast<tone_t>(tone);
    static_cast<Chord&>(*voice.notes.back()).stem_length = stem_length * 500;
    static_cast<Chord&>(*voice.notes.back()).beam = beam;
}

void add(SubVoice& voice, unsigned char exp, int tone, int stem_length, Chord::BeamType beam)
{
    voice.notes.push_back(VoiceObjectPtr(new Chord()));
    static_cast<Chord&>(*voice.notes.back()).val.exp = exp & 0x0F;
    static_cast<Chord&>(*voice.notes.back()).heads.push_back(HeadPtr(new Head()));
    static_cast<Chord&>(*voice.notes.back()).heads.back()->tone = static_cast<tone_t>(tone);
    static_cast<Chord&>(*voice.notes.back()).stem_length = stem_length * 500;
    static_cast<Chord&>(*voice.notes.back()).beam = beam;
}

void add(MainVoice& voice, unsigned char exp, int tone, int stem_length,
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

void add(SubVoice& voice, unsigned char exp, int tone, int stem_length,
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

void addr(MainVoice& voice, unsigned char exp, int offset_y)
{
    voice.notes.push_back(StaffObjectPtr(new Rest()));
    static_cast<Rest&>(*voice.notes.back()).val.exp = exp & 0x0F;
    static_cast<Rest&>(*voice.notes.back()).offset_y = offset_y;
}

void addr(SubVoice& voice, unsigned char exp, int offset_y)
{
    voice.notes.push_back(VoiceObjectPtr(new Rest()));
    static_cast<Rest&>(*voice.notes.back()).val.exp = exp & 0x0F;
    static_cast<Rest&>(*voice.notes.back()).offset_y = offset_y;
}

void add_head(MainVoice& voice, int tone)
{
    static_cast<Chord&>(*voice.notes.back()).heads.push_back(HeadPtr(new Head()));
    static_cast<Chord&>(*voice.notes.back()).heads.back()->tone = static_cast<tone_t>(tone);
}

void add_head(SubVoice& voice, int tone)
{
    static_cast<Chord&>(*voice.notes.back()).heads.push_back(HeadPtr(new Head()));
    static_cast<Chord&>(*voice.notes.back()).heads.back()->tone = static_cast<tone_t>(tone);
}

void add_accidental(MainVoice& voice, Accidental::Type type, int offset_x)
{
    static_cast<Chord&>(*voice.notes.back()).heads.back()->accidental.type = type;
    static_cast<Chord&>(*voice.notes.back()).heads.back()->accidental.offset_x = offset_x;
}

void add_accidental(SubVoice& voice, Accidental::Type type, int offset_x)
{
    static_cast<Chord&>(*voice.notes.back()).heads.back()->accidental.type = type;
    static_cast<Chord&>(*voice.notes.back()).heads.back()->accidental.offset_x = offset_x;
}

void add_articulation(MainVoice& voice, const SpriteId& sprite, int offset_y, bool far)
{
    static_cast<Chord&>(*voice.notes.back()).articulation.push_back(Articulation());
    static_cast<Chord&>(*voice.notes.back()).articulation.back().sprite = sprite;
    static_cast<Chord&>(*voice.notes.back()).articulation.back().offset_y = offset_y;
    static_cast<Chord&>(*voice.notes.back()).articulation.back().far = far;
}

void add_articulation(SubVoice& voice, const SpriteId& sprite, int offset_y, bool far)
{
    static_cast<Chord&>(*voice.notes.back()).articulation.push_back(Articulation());
    static_cast<Chord&>(*voice.notes.back()).articulation.back().sprite = sprite;
    static_cast<Chord&>(*voice.notes.back()).articulation.back().offset_y = offset_y;
    static_cast<Chord&>(*voice.notes.back()).articulation.back().far = far;
}

void add_newline(MainVoice& voice, unsigned int distance, int indent, int right_margin, bool justify)
{
    voice.notes.push_back(StaffObjectPtr(new Newline()));
    static_cast<Newline&>(*voice.notes.back()).distance = distance;
    static_cast<Newline&>(*voice.notes.back()).indent = indent;
    static_cast<Newline&>(*voice.notes.back()).right_margin = right_margin;
    static_cast<Newline&>(*voice.notes.back()).justify = justify;
}

void add_newline(SubVoice& voice, unsigned int distance, int indent, int right_margin, bool justify)
{
    voice.notes.push_back(VoiceObjectPtr(new Newline()));
    static_cast<Newline&>(*voice.notes.back()).distance = distance;
    static_cast<Newline&>(*voice.notes.back()).indent = indent;
    static_cast<Newline&>(*voice.notes.back()).right_margin = right_margin;
    static_cast<Newline&>(*voice.notes.back()).justify = justify;
}

void add1(Staff& staff, Sprites& sprites, int toneoffset)
{
    // main-voice notes
    add(staff, 5, toneoffset + 69, 6);
    static_cast<Chord&>(*staff.notes.back()).subvoice = RefPtr<SubVoice>(new SubVoice(&staff));
    SubVoice& subvoice = *static_cast<Chord&>(*staff.notes.back()).subvoice;
    subvoice.stem_direction = Voice::STEM_DOWNWARDS;
    static_cast<Chord&>(*staff.notes.back()).attached.push_back(MovablePtr(new Slur()));
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).duration = 5;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).typeX = Movable::PARENT;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).typeY = Movable::STAFF;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position.x = 0;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).position.y = 0;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).control1.x = 2000;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).control1.y = -3000;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).control2.x = -2000;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).control2.y = -3000;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).end_position.x = 0;
    static_cast<Slur&>(*static_cast<Chord&>(*staff.notes.back()).attached.front()).end_position.y = 0;
    add(staff, 5, 70 + toneoffset, 6, 100, -700, 1500, -600, -1500, -600, -100, -700);
    add_accidental(staff, Accidental::flat, 0);
    add_articulation(staff, SpriteId(0, sprites[0].ids["articulation.tenuto"]), 0, true);
    add(staff, 4, 70 + toneoffset, 6, Chord::NO_BEAM);
    add_accidental(staff, Accidental::flat, 0);
    /**/add(staff, 4, 69 + toneoffset, 6);
    add(staff, 5, 72 + toneoffset, 6);
    add_articulation(staff, SpriteId(0, sprites[0].ids["articulation.marcato"]), 0, true);
    
    // sub-voice notes
    add(subvoice, 5, 65 + toneoffset, -6);
    add(subvoice, 5, 63 + toneoffset, -6);
    add_accidental(subvoice, Accidental::flat, 500);
    add(subvoice, 5, 67 + toneoffset, -6);
    add(subvoice, 5, 67 + toneoffset, -6, Chord::NO_BEAM);
    add_articulation(subvoice, SpriteId(0, sprites[0].ids["articulation.marcato"]), 0, true);
}

void add2(Staff& staff, Sprites& sprites, int toneoffset, int staffdist)
{
    // main-voice notes
    add(staff, 5, 69 + toneoffset, 6);
    static_cast<Chord&>(*staff.notes.back()).subvoice = RefPtr<SubVoice>(new SubVoice(&staff));
    SubVoice& subvoice = *static_cast<Chord&>(*staff.notes.back()).subvoice;
    subvoice.stem_direction = Voice::STEM_DOWNWARDS;
    add_head(staff, 71 + toneoffset);
    add_articulation(staff, SpriteId(0, sprites[0].ids["articulation.staccato"]), 0, false);
    add(staff, 6, 71 + toneoffset, 6);
    /*
    add(staff, 5, 72 + toneoffset, 4);
    add_accidental(staff, Accidental::natural, 1200);
    add_head(staff, 74 + toneoffset);
    add_accidental(staff, Accidental::natural, 200);
    add_head(staff, 78 + toneoffset);
    add_accidental(staff, Accidental::sharp, 200);
    //*/
    add(staff, 5, 69 + toneoffset, 6, 0, -1100, 1500, -600, -800, -650, 0, -1100);
    add_newline(staff, staffdist, 0, 0, JUSTIFY);
    
    // sub-voice notes
    addr(subvoice, 5, 3000);
    add(subvoice, 4, 65 + toneoffset, -6, Chord::FORCE_BEAM);
    /**/static_cast<Chord&>(*subvoice.notes.back()).val.dots = 1;
    static_cast<Chord&>(*subvoice.notes.back()).attached.push_back(MovablePtr(new Hairpin()));
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).duration = 4;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).typeX = Movable::PARENT;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).typeY = Movable::STAFF;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).position.x = 0;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).position.y = 8000;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).end_position.x = 0;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).end_position.y = 8000;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).thickness = 1000;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).height = 800;
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).crescendo = true;
    
    static_cast<Hairpin&>(*static_cast<Chord&>(*subvoice.notes.back()).attached.back()).appearance.scale = 1300;
    
    add_accidental(subvoice, Accidental::sharp, 0);
    add_head(subvoice, 66 + toneoffset);
    add_accidental(subvoice, Accidental::sharp, 0);
    //addr(subvoice, 3, 2000);
    add(subvoice, 3, 69 + toneoffset, -8);
    add(subvoice, 5, 67 + toneoffset, -6);
    add(subvoice, 5, 65 + toneoffset, -6);
    add_accidental(subvoice, Accidental::natural, 0);
    add_head(subvoice, 67 + toneoffset);
    add_accidental(subvoice, Accidental::natural, 250);
    add_newline(subvoice, staffdist, 0, 0, JUSTIFY);
}

void Engine::set_test(Sprites& sprites)
{
    // setup parameters
    engraver.parameters.force_justification = true;
    //engraver.parameters.auto_barlines = true;
    press.parameters.scale = 1200;
    //press.parameters.draw_linebounds = true;
    //press.parameters.draw_notebounds = true;
    //press.parameters.draw_attachbounds = true;
    interface.relative_accidentals = false;
    
    // setup document
    document.head_height = 2000;       // µm
    document.stem_width = 250;         // µm
    document.add_attached(new TextArea(), 0);
    static_cast<TextArea*>(document.get_attached().back().object)->position.x = 55000;
    static_cast<TextArea*>(document.get_attached().back().object)->position.y = 0;
    static_cast<TextArea*>(document.get_attached().back().object)->width = 80000;
    static_cast<TextArea*>(document.get_attached().back().object)->height = 16000;
    static_cast<TextArea*>(document.get_attached().back().object)->text.push_back(Paragraph());
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().align = Paragraph::CENTER;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().justify = false;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.push_back(PlainText());
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().text = "   ScorePress   ";
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.family = "Liberation Serif";
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.size = 24;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.bold = true;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.italic = false;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.underline = true;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.color.r = 0;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.color.g = 0;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.color.b = 0;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.color.a = 255;
    static_cast<TextArea*>(document.get_attached().back().object)->text.push_back(Paragraph());
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().align = Paragraph::CENTER;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().justify = false;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.push_back(PlainText());
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().text = "Music Engraving Software";
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.family = "Liberation Serif";
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.size = 12;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.bold = false;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.italic = true;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.underline = false;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.color.r = 0;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.color.g = 0;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.color.b = 0;
    static_cast<TextArea*>(document.get_attached().back().object)->text.back().text.back().font.color.a = 255;
    
    // setup score
    document.scores.push_back(Document::Score(0));
    Score& score = document.scores.back().score;
    score.layout.dimension.width = 190000;      // µm
    score.layout.dimension.height = 297000;     // µm
    
    // setup staff
    score.staves.push_back(Staff());
    score.staves.back().head_height = 1760;     // µm
    score.staves.back().offset_y = 5000;        // pohh
    score.staves.back().line_count = 5;
    score.staves.back().long_barlines = true;
    score.staves.back().curlybrace = true;
    score.staves.back().layout.indent = 10000;      // µm
    score.staves.back().layout.justify = JUSTIFY;
    score.staves.back().layout.distance = 10000;    // pohh
    score.staves.back().layout.auto_clef = false;
    score.staves.back().layout.auto_key = false;
    score.staves.back().layout.auto_timesig = false;
    
    score.staves.back().notes.push_back(StaffObjectPtr(new Clef()));
    static_cast<Clef&>(*score.staves.back().notes.back()).sprite = SpriteId(0, sprites[0].ids["clef.treble"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).base_note = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.treble"]].integer["basenote"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).line = static_cast<unsigned char>(sprites[0][sprites[0].ids["clef.treble"]].integer["line"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_sharp = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.treble"]].integer["keybound.sharp"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_flat = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.treble"]].integer["keybound.flat"]);
    
    score.staves.back().notes.push_back(StaffObjectPtr(new Key()));
    static_cast<Key&>(*score.staves.back().notes.back()).type = Key::SHARP;
    static_cast<Key&>(*score.staves.back().notes.back()).number = 4;
    
    score.staves.back().notes.push_back(StaffObjectPtr(new TimeSig()));
    static_cast<TimeSig&>(*score.staves.back().notes.back()).number = 2;
    static_cast<TimeSig&>(*score.staves.back().notes.back()).beat = 4;
    //static_cast<CustomTimeSig&>(*score.staves.back().notes.back()).sprite = SpriteId(0, sprites[0].ids["timesig.symbol4_4_timesigC"]);
    
    add1(score.staves.back(), sprites);
    add2(score.staves.back(), sprites, 0, 3000);
    /*
    score.staves.back().notes.push_back(StaffObjectPtr(new Clef()));
    static_cast<Clef&>(*score.staves.back().notes.back()).sprite = SpriteId(0, sprites[0].ids["clef.treble"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).base_note = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.treble"]].integer["basenote"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).line = static_cast<unsigned char>(sprites[0][sprites[0].ids["clef.treble"]].integer["line"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_sharp = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.treble"]].integer["keybound.sharp"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_flat = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.treble"]].integer["keybound.flat"]);
    
    score.staves.back().notes.push_back(StaffObjectPtr(new Key()));
    static_cast<Key&>(*score.staves.back().notes.back()).type = Key::SHARP;
    static_cast<Key&>(*score.staves.back().notes.back()).number = 4;
    //*/
    add1(score.staves.back(), sprites);
    add2(score.staves.back(), sprites, 0, 3000);
    
    score.staves.push_back(Staff());
    score.staves.back().head_height = 1760;     // µm
    score.staves.back().offset_y = 6000;        // pohh
    score.staves.back().line_count = 5;
    score.staves.back().long_barlines = false;
    score.staves.back().curlybrace = false;
    score.staves.back().layout.indent = 10000;  // µm
    score.staves.back().layout.justify = JUSTIFY;
    score.staves.back().layout.distance = 0;    // pohh
    score.staves.back().layout.auto_clef = false;
    score.staves.back().layout.auto_key = false;
    score.staves.back().layout.auto_timesig = false;
    
    score.staves.back().notes.push_back(StaffObjectPtr(new Clef()));
    static_cast<Clef&>(*score.staves.back().notes.back()).sprite = SpriteId(0, sprites[0].ids["clef.bass"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).base_note = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.bass"]].integer["basenote"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).line = static_cast<unsigned char>(sprites[0][sprites[0].ids["clef.bass"]].integer["line"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_sharp = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.bass"]].integer["keybound.sharp"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_flat = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.bass"]].integer["keybound.flat"]);
    
    score.staves.back().notes.push_back(StaffObjectPtr(new Key()));
    static_cast<Key&>(*score.staves.back().notes.back()).type = Key::SHARP;
    static_cast<Key&>(*score.staves.back().notes.back()).number = 2;
    
    score.staves.back().notes.push_back(StaffObjectPtr(new TimeSig()));
    static_cast<TimeSig&>(*score.staves.back().notes.back()).number = 2;
    static_cast<TimeSig&>(*score.staves.back().notes.back()).beat = 4;
    //static_cast<CustomTimeSig&>(*score.staves.back().notes.back()).sprite = SpriteId(0, sprites[0].ids["timesig.symbol4_4_timesigC"]);
    
    add1(score.staves.back(), sprites, -24);
    add2(score.staves.back(), sprites, -24);
    /*
    score.staves.back().notes.push_back(StaffObjectPtr(new Clef()));
    static_cast<Clef&>(*score.staves.back().notes.back()).sprite = SpriteId(0, sprites[0].ids["clef.bass"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).base_note = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.bass"]].integer["basenote"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).line = static_cast<unsigned char>(sprites[0][sprites[0].ids["clef.bass"]].integer["line"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_sharp = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.bass"]].integer["keybound.sharp"]);
    static_cast<Clef&>(*score.staves.back().notes.back()).keybnd_flat = static_cast<tone_t>(sprites[0][sprites[0].ids["clef.bass"]].integer["keybound.flat"]);
    
    score.staves.back().notes.push_back(StaffObjectPtr(new Key()));
    static_cast<Key&>(*score.staves.back().notes.back()).type = Key::SHARP;
    static_cast<Key&>(*score.staves.back().notes.back()).number = 4;
    //*/
    add1(score.staves.back(), sprites, -24);
    add2(score.staves.back(), sprites, -24);
}

Engine::Engine(Sprites& sprites) : engraver(pageset, sprites, style, viewport),
                                   press(style, viewport),
                                   cursor(document, pageset, interface) {}

void Engine::set_resolution(unsigned int hppm, unsigned int vppm)
{
    viewport.hppm = hppm;
    viewport.vppm = vppm;
}

void Engine::engrave()
{
    engraver.engrave(document);
    if (!cursor.ready())
    {
        cursor.set_score(document.scores.front());
        cursor.set_engraver(engraver);
    };
}

void Engine::render(Renderer& renderer, const Position<mpx_t>& offset)
{
    if (pageset.pages.empty()) engrave();
    Position<mpx_t> margin_offset((press.parameters.scale * pageset.page_layout.margin.left) / 1000,
                                  (press.parameters.scale * pageset.page_layout.margin.top) / 1000);
    
    Position<mpx_t> off(offset);
    for (std::list<PageSet::pPage>::const_iterator i = pageset.pages.begin(); i != pageset.pages.end(); ++i)
    {
        press.render_decor(renderer, pageset, off);
        press.render(renderer, *i, pageset, off + margin_offset);
        off.y += pageset.page_layout.height;
    };
}

void Engine::render_cursor(Renderer& renderer, const Position<mpx_t>& offset)
{
    Position<mpx_t> margin_offset((press.parameters.scale * pageset.page_layout.margin.left) / 1000,
                                  (press.parameters.scale * pageset.page_layout.margin.top) / 1000);
    try
    {
        renderer.set_color(0, 0, 0, 255);
        press.render(renderer, cursor, offset + margin_offset);
    }
    catch (UserCursor::NotValidException& e)
    {
        Log::error("Invalid UserCursor (class: Engine)");
        cursor.set_score(document.scores.front());
    };
}

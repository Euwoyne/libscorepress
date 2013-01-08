
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2012 Dominik Lehmann
  
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

#ifndef SCOREPRESS_CLASSES_HH
#define SCOREPRESS_CLASSES_HH

#include <string>          // std::string
#include <list>            // std::list
#include "smartptr.hh"     // SmartPtr
#include "refptr.hh"       // RefPtr
#include "fraction.hh"     // Fraction
#include "sprite_id.hh"    // SpriteId

namespace ScorePress
{
//  CLASSES
// ---------

// PROTOTYPES
class EngraverState;   // engraver state class prototype (see "engraver_state.hh")

// BASE CLASSES
template <typename T>
class Position;        // graphical position (2-dimensional vector)
struct Color;          // RGBA-color structure
struct Font;           // font structure
class Appearance;      // graphical appearance properties (visibility, color, scale)
class Class;           // abstract base class for all classes

// MAIN MUSIC OBJECT CLASSES
class VisibleObject;   // base class for visible objects
class StaffObject;     // class for objects to reside within a staff (clefs, keys, time-signatures, notes and newlines)
class MusicObject;     // visible staff-objects
class Clef;            // a clef (contains sprite and note-positioning information)
class Key;             // class representing a key signature
class TimeSig;         // a time signature
class CustomTimeSig;   // a time signature with a custom sprite
class Barline;         // a barline
class VoiceObject;     // class for objects to reside within a voice (notes and newlines)
class Newline;         // newline indicator
class ScoreDimension;  // dimension of the on-page score-object
class Pagebreak;       // page-break indicator (with next page's layout information)
class NoteObject;      // class for played objects (visible voice-objects, which have a duration, as chords and rests)
class Chord;           // a chord (note-object consisting of several heads)
class Rest;            // a rest (inherits note-object interface)

// MISCELLANEOUS CLASSES (AS PARTS OF MAIN MUSIC CLASSES)
class Accidental;      // accidental abstraction (type, offset)
class Articulation;    // articulation symbol (temporarily context changing)
class Head;            // note-head class (with tone, accidental, etc.)
class TiedHead;        // note-head with tie-position information

// VOICE STRUCTURE CLASSES
class Voice;           // voice base type
class MainVoice;       // main-voice; wrapper for a list of staff-objects
class SubVoice;        // sub-voice; wrapper for a list of note-objects

// MOVABLE AND ATTACHABLE OBJECTS
class Movable;         // base class for movable, attachable objects (position data)
class PlainText;       // plain-text object (text with formatting information)
class Paragraph;       // paragraph object (list of consequent plain-text objects with alignment information)
class TextArea;        // text-area object (movable list of consequent plain-text objects)
class ContextChanging; // context-changing object (see "context.hh")
class Symbol;          // base class for interpretable symbols (has context-changing information)
class PluginInfo;      // a movable object carrying arbitrary information for a plugin
class Annotation;      // a text-area with context-changing information
class CustomSymbol;    // a custom symbol is a symbol with custom graphical representation (sprite-id)
class Durable;         // base class for symbols with two anchor points
class Slur;            // legato slur (a symbol, rendered as a cubic bezier curve)
class Hairpin;         // crescendo and diminuendo "hairpin" symbols

// TYPE DEFINTIONS
typedef int           mpx_t;       // milli-pixel (graphical positioning)
typedef unsigned char tone_t;      // note-number (as defined for the MIDI standard: a' = 69)
typedef Fraction      value_t;     // note-values will be represented by exact fractions ("double" is too imprecise)

typedef SmartPtr<Movable,     CloneTrait> MovablePtr;       // smart pointer to movable object
typedef SmartPtr<Head,        CloneTrait> HeadPtr;          // smart pointer to head
typedef SmartPtr<StaffObject, CloneTrait> StaffObjectPtr;   // smart pointer to staff-object
typedef SmartPtr<VoiceObject, CloneTrait> VoiceObjectPtr;   // smart pointer to note-object

typedef std::list<MovablePtr>             MovableList;      // list of smart pointers to movable objects
typedef std::list<HeadPtr>                HeadList;         // list of smart pointers to heads
typedef std::list<Articulation>           ArticulationList; // list of articulation symbols
typedef std::list<StaffObjectPtr>         StaffObjectList;  // list of smart pointers to staff-objects
typedef std::list<VoiceObjectPtr>         VoiceObjectList;  // list of smart pointers to note-objects

// note value base exponent (i.e. the exponent of a whole note)
static const int VALUE_BASE = 7;


//
//     BASE CLASSES
//    ==============
//

// graphical position (2-dimensional vector)
template <typename T = int> class Position
{
 public:
    T x; T y;
    Position(const T _x = 0, const T _y = 0) : x(_x), y(_y) {};
    Position& operator += (const Position& p) {x += p.x; y += p.y; return *this;};
    Position& operator -= (const Position& p) {x -= p.x; y -= p.y; return *this;};
    Position& operator *= (const T& p) {x *= p; y *= p; return *this;};
    Position& operator /= (const T& p) {x /= p; y /= p; return *this;};
};

template <typename T> Position<T> operator + (const Position<T>& p, const Position<T>& q) {return Position<T>(p.x + q.x, p.y + q.y);}
template <typename T> Position<T> operator - (const Position<T>& p, const Position<T>& q) {return Position<T>(p.x - q.x, p.y - q.y);}
template <typename T> Position<T> operator * (const Position<T>& p, const T& q) {return Position<T>(p.x * q, p.y * q);}
template <typename T> Position<T> operator * (const T& p, const Position<T>& q) {return Position<T>(p * q.x, p * q.y);}
template <typename T> Position<T> operator / (const Position<T>& p, const T& q) {return Position<T>(p.x / q, p.y / q);}

// RGBA-color structure
struct Color{unsigned char r; unsigned char g; unsigned char b; unsigned char a;};
bool operator == (const Color& c1, const Color& c2);

// font structure
struct Font{std::string family; double size; bool bold; bool italic; bool underline; Color color;};

// graphical appearance properties (visibility, color, scale)
class Appearance
{
 public:
    bool visible;       // visibility
    Color color;        // color
    unsigned int scale; // in promille
    
 public:
    Appearance() : visible(true), scale(1000) {color.r = color.g = color.b = 0; color.a = 255;};
};

// abstract base class for all classes
class Class
{
 public:
    // type enumeration
    enum classType {VISIBLEOBJECT,
                    STAFFOBJECT, MUSICOBJECT, CLEF,    // MAIN MUSIC OBJECT CLASSES
                                              KEY,
                                              TIMESIG, CUSTOMTIMESIG,
                                              BARLINE,
                    VOICEOBJECT, NEWLINE, PAGEBREAK,
                                 NOTEOBJECT, CHORD,
                                             REST,
                    
                    ACCIDENTAL, ARTICULATION,          // SPECIAL ATTACHED OBJECTS
                    HEAD, TIEDHEAD,                    // SPECIAL HEADS
                    
                    VOICE, MAINVOICE,                  // VOICE STRUCTURE CLASSES
                           SUBVOICE,
                    
                    MOVABLE, TEXTAREA, ANNOTATION,     // MOVABLE AND ATTACHABLE OBJECTS
                             PLUGININFO,
                             SYMBOL, CUSTOMSYMBOL,
                                     DURABLE, SLUR,
                                              HAIRPIN};
    
 public:
    virtual bool is(classType type) const = 0;
    virtual classType classtype() const = 0;
    virtual Class* clone() const = 0;
    virtual ~Class() {};
};

// returns a human readable class name (for debugging purposes)
const std::string classname(Class::classType type);


//
//     MAIN MUSIC OBJECT CLASSES
//    ===========================
//

// base class for visible objects
class VisibleObject : virtual public Class
{
 public:
    MovableList attached;      // attached objects
    int offset_x;              // horizontal offset (in promille of head-width)
    Appearance appearance;     // graphical appearance properties
    
 protected: VisibleObject() : offset_x(0) {};
 public:
    virtual bool is(classType type) const {return (type == VISIBLEOBJECT);};
    virtual classType classtype() const {return VISIBLEOBJECT;};
    virtual VisibleObject* clone() const = 0;
};

// class for objects to reside within a staff (clefs, keys, time-signatures, notes and newlines)
class StaffObject : virtual public Class
{
 public:
    int acc_offset;             // accumulative horizontal offset (in promille of head-width) 
    
 protected: StaffObject() : acc_offset(0) {};
 public:
    virtual VisibleObject& get_visible() = 0;
    virtual const VisibleObject& get_visible() const = 0;
    
    virtual void engrave(EngraverState& engraver) const = 0;
    virtual bool is(classType type) const {return (type == STAFFOBJECT);};
    virtual StaffObject* clone() const = 0;
    virtual classType classtype() const {return STAFFOBJECT;};
};

// visible staff-objects
class MusicObject : public StaffObject, public VisibleObject
{
 protected: MusicObject() {};
 public:
    virtual VisibleObject& get_visible() {return *this;};
    virtual const VisibleObject& get_visible() const {return *this;};
    virtual void engrave(EngraverState& engraver) const = 0;
    virtual bool is(classType type) const {return ((type == MUSICOBJECT) || StaffObject::is(type) || VisibleObject::is(type));};
    virtual MusicObject* clone() const = 0;
    virtual classType classtype() const {return MUSICOBJECT;};
};

// a clef (contains sprite and note-positioning information)
class Clef : public MusicObject
{
 public:
    SpriteId sprite;       // sprite
    tone_t base_note;      // tone residing on the specified line
    unsigned char line;    // 0 - first line, 1 - first space, 2 - second line, 3 - second space, 4 - third line, etc.
    tone_t keybnd_sharp;   // lowest tone for sharp-key display (to specify area of key signature)
    tone_t keybnd_flat;    // lowest tone for flat-key display (to specify area of key signature)
    
 public:
    Clef() : base_note(67), line(5), keybnd_sharp(69), keybnd_flat(65) {};
    virtual void engrave(EngraverState& engraver) const;
    virtual bool is(classType type) const {return ((type == CLEF) || MusicObject::is(type));};
    virtual classType classtype() const {return CLEF;};
    virtual Clef* clone() const {return new Clef(*this);};
};

// class representing a key signature
class Key : public MusicObject
{
 public:
    enum Type {SHARP, FLAT} type;  // key type
    char number;                   // number of "accidentals" (e.g. FLAT * 4 is As-Major or f-minor)
    SpriteId sprite;               // accidental sprite id
    
 public:
    Key() : type(SHARP), number(0) {};
    virtual void engrave(EngraverState& engraver) const;
    virtual bool is(classType _type) const {return ((_type == KEY) || MusicObject::is(_type));};
    virtual classType classtype() const {return KEY;};
    virtual Key* clone() const {return new Key(*this);};
};

// a time signature
class TimeSig : public MusicObject
{
 public:
    unsigned char number;  // number of beats per measure (enumerator)
    unsigned char beat;    // length of a beat in measure (denominator)
    
 public:
    TimeSig() : number(4), beat(4) {};
    void engrave(EngraverState& engraver, size_t setid) const;
    virtual void engrave(EngraverState& engraver) const;
    virtual bool is(classType type) const {return ((type == TIMESIG) || MusicObject::is(type));};
    virtual classType classtype() const {return TIMESIG;};
    virtual TimeSig* clone() const {return new TimeSig(*this);};
    
    inline operator value_t() const {return value_t(static_cast<unsigned long>(number) << VALUE_BASE,
                                                    static_cast<long>(beat));}
};

// a time signature with a custom sprite
class CustomTimeSig : public TimeSig
{
 public:
    SpriteId sprite;   // custom sprite for the time signature
    
 public:
    virtual void engrave(EngraverState& engraver) const;
    virtual bool is(classType type) const {return ((type == CUSTOMTIMESIG) || TimeSig::is(type));};
    virtual classType classtype() const {return CUSTOMTIMESIG;};
    virtual CustomTimeSig* clone() const {return new CustomTimeSig(*this);};
};

// a barline
class Barline : public MusicObject
{
 public:
    typedef std::string Style;
    
    static const char* const singlebar;
    static const char* const doublebar;
    static const char* const endbar;
    Style style;
    
 public:
    Barline() : style(singlebar) {};
    Barline(const Style& s) : style(s) {};
    virtual void engrave(EngraverState& engraver) const;
    virtual bool is(classType type) const {return ((type == BARLINE) || MusicObject::is(type));};
    virtual classType classtype() const {return BARLINE;};
    virtual Barline* clone() const {return new Barline(*this);};
};

// class for objects to reside within a voice (notes and newlines)
class VoiceObject : public StaffObject
{
 protected: VoiceObject() {};
 public:
    virtual VisibleObject& get_visible() = 0;
    virtual const VisibleObject& get_visible() const = 0;
    virtual void engrave(EngraverState& engraver) const = 0;
    virtual bool is(classType type) const {return ((type == VOICEOBJECT) || StaffObject::is(type));};
    virtual classType classtype() const {return VOICEOBJECT;};
    virtual VoiceObject* clone() const = 0;
};

// newline indicator
class Newline : public VoiceObject
{
 public:
    int  indent;                // LINE:  in micrometer
    bool justify;               // LINE:  width justification for this line?
    int  right_margin;          // LINE:  in micrometer (only for justified lines)
    int  distance;              // STAFF: in promille of head-height
    bool auto_clef;             // STAFF: insert clef at the line front
    bool auto_key;              // STAFF: insert key at the line front
    bool auto_timesig;          // STAFF: insert time-signature at the line front
    bool visible;               // VOICE: voice visible in this line?
    
 public:
    Newline() : indent(0), justify(false), right_margin(0), distance(0), auto_clef(true), auto_key(true), auto_timesig(false), visible(true) {};
    virtual VisibleObject& get_visible() {return *static_cast<VisibleObject*>(NULL);};
    virtual const VisibleObject& get_visible() const {return *static_cast<VisibleObject*>(NULL);};
    virtual void engrave(EngraverState&) const {};
    virtual bool is(classType type) const {return ((type == NEWLINE) || VoiceObject::is(type));};
    virtual classType classtype() const {return NEWLINE;};
    virtual Newline* clone() const {return new Newline(*this);};
};

// dimension of the on-page score-object
class ScoreDimension
{
 public:
    Position<> position;        // position of the score-object (in micrometer)
    unsigned int width;         // in micrometer
    unsigned int height;        // in micrometer
    
 public:
    ScoreDimension() : width(0), height(0) {};
    bool contains(const Position<>& pos);   // check, if the score-object contains a given point
};

// page-break indicator (with next page's layout information)
class Pagebreak : public Newline
{
 public:
    MovableList attached;       // objects attached to the page
    ScoreDimension dimension;   // layout information
    
 public:
    virtual bool is(classType type) const {return ((type == PAGEBREAK) || Newline::is(type));};
    virtual classType classtype() const {return PAGEBREAK;};
    virtual Pagebreak* clone() const {return new Pagebreak(*this);};
};

// class for played objects (music-objects, which have a duration, as chords and rests)
class NoteObject : public VoiceObject, public VisibleObject
{
 public:
    RefPtr<SubVoice> subvoice;  // sub voice attached to this note
    int staff_shift;            // note in different staff (if neq 0)
    
    struct
    {
        unsigned exp  : 4;
        unsigned dots : 3;
        unsigned mute : 1;
    } val;
    
    unsigned char irr_enum;     // tuplet enumerator
    unsigned char irr_denom;    // tuplet denominator
    
 protected: NoteObject() : subvoice(NULL), staff_shift(0), irr_enum(0), irr_denom(0) {val.exp = 5; val.dots = 0; val.mute = 0;};
 public:
    virtual VisibleObject& get_visible() {return *this;};
    virtual const VisibleObject& get_visible() const {return *this;};
    virtual void engrave(EngraverState& engraver) const = 0;
    virtual bool is(classType type) const {return ((type == NOTEOBJECT) || VoiceObject::is(type) || VisibleObject::is(type));};
    virtual classType classtype() const {return NOTEOBJECT;};
    virtual NoteObject* clone() const = 0;
     
    inline unsigned int base() const      {return 1u << val.exp;};  // 2^exp
    inline void set_exp(unsigned char e)  {val.exp = e & 0x0F;};
    inline void set_dots(unsigned char d) {val.dots = d & 0x07;};
    inline void set_mute(bool m)          {val.mute = m;};
    
    value_t set_value(value_t v);   // create note object from length (return error factor)
    const value_t value() const;    // calculate the note-object's value
    
    void add_subvoice(RefPtr<SubVoice>& newvoice);
};

// a chord (note-object consisting of several heads)
class Chord : public NoteObject
{
 public:
    enum BeamType {NO_BEAM, AUTO_BEAM, FORCE_BEAM, CUT_BEAM};
    
 public:
    // TODO: tremolo implementation
    HeadList heads;                 // heads of the chord
    ArticulationList articulation;  // articulation symbols
    SpriteId sprite;                // head sprite id
    int stem_length;                // stem length (in promille of head_height)
    Color stem_color;               // stem color
    bool invisible_flag;            // should the flag be rendered (ignored on notes with stem)
    Color flag_color;               // flag color
    BeamType beam;                  // type of the beam to the next note
    
 public:
    Chord() : stem_length(3500), invisible_flag(false), beam(AUTO_BEAM) {stem_color.r = stem_color.g = stem_color.b = 0; stem_color.a = 255;};
    virtual void engrave(EngraverState& engraver) const;
    virtual bool is(classType type) const {return ((type == CHORD) || NoteObject::is(type));};
    virtual classType classtype() const {return CHORD;};
    virtual Chord* clone() const {return new Chord(*this);};
};

// a rest (inherits note-object interface)
class Rest : public NoteObject
{
 public:
    int offset_y;          // vertical offset (in promille of head-height)
                           // (horizontal offset inherited from "NoteObject")
    Position<> dot_offset; // offset for the dots (in promille of head-height)
    
    SpriteId sprite;       // rest sprite id
    
 public:
    Rest() : offset_y(0) {};
    virtual void engrave(EngraverState& engraver) const;
    virtual bool is(classType type) const {return ((type == REST) || NoteObject::is(type));};
    virtual classType classtype() const {return REST;};
    virtual Rest* clone() const {return new Rest(*this);};
};


//
//     MISCELLANEOUS CLASSES (AS PARTS OF MAIN MUSIC CLASSES)
//    ========================================================
//

// accidental abstraction (type, offset)
class Accidental : public Class
{
 public:
    static const int  note_modifier[9]; // tone modification for each accidental-type
                                        // (quarter tones are rounded down)
    enum Type   // accidental type enumerator
    {
        double_flat    = 0,
        flat_andahalf  = 1,
        flat           = 2,
        half_flat      = 3,
        natural        = 4,
        half_sharp     = 5,
        sharp          = 6,
        sharp_andahalf = 7,
        double_sharp   = 8
    };
    
 public:
    Type type;              // accidental type
    int offset_x;           // horizontal offset (in promille of head-width)
    Appearance appearance;  // graphical appearance properties
    bool force;             // force rendering (do not check key signature)
    SpriteId sprite;        // accidental sprite id
    
 public:
    Accidental() : type(natural), offset_x(0), force(false) {};
    virtual bool is(classType _type) const {return (_type == ACCIDENTAL);};
    virtual classType classtype() const {return ACCIDENTAL;};
    virtual Accidental* clone() const {return new Accidental(*this);};
};

// articulation symbol (temporarily context changing)
class Articulation : public Class
{
 public:
    SpriteId sprite;               // sprite id
    Appearance appearance;         // graphical appearance properties
    int offset_y;                  // vertical offset (in promille of head-height)
    bool far;                      // symbol placed far from the heads (i.e. on top of the stem)
    unsigned int value_modifier;   // (in promille)
    unsigned int volume_modifier;  // (in promille)
    
 public:
    Articulation() : offset_y(0), far(false), value_modifier(0), volume_modifier(0) {};
    virtual bool is(classType type) const {return (type == ARTICULATION);};
    virtual classType classtype() const {return ARTICULATION;};
    virtual Articulation* clone() const {return new Articulation(*this);};
};

// note-head class (with tone, accidental, etc.)
class Head : public Class
{
 public:
    tone_t tone;               // as defined for MIDI: a' = 69
    Accidental accidental;     // associated accidental
    Appearance appearance;     // graphical appearance properties
    Position<> dot_offset;     // offset for the dots (in promille of head-size)
    
 public:
    Head() : tone(69) {};
    virtual bool is(classType type) const {return (type == HEAD);};
    virtual classType classtype() const {return HEAD;};
    virtual Head* clone() const {return new Head(*this);};
};

// note-head with tie-position information
class TiedHead : public Head
{
 public:
    Position<> offset1;    // offset of the first anchor (in promille of head-height)
    Position<> offset2;    // offset of the second anchor (in promille of head-height)
    Position<> control1;   // offset of the first control point (in promille of head-height)
    Position<> control2;   // offset of the second control point (in promille of head-height)
    
 public:
    TiedHead() : Head() {};
    TiedHead(const Head& head) : Head(head) {};
    virtual bool is(classType type) const {return ((type == TIEDHEAD) || Head::is(type));};
    virtual classType classtype() const {return TIEDHEAD;};
    virtual TiedHead* clone() const {return new TiedHead(*this);};
};


//
//     VOICE STRUCTURE CLASSES
//    =========================
//

// voice base type
class Voice : public Class
{
 public:
    enum StemDirection {STEM_AUTOMATIC, STEM_UPWARDS, STEM_DOWNWARDS};
    StemDirection stem_direction;
    
 public:
    Voice() : stem_direction(STEM_AUTOMATIC) {};
    virtual bool is(classType type) const {return (type == VOICE);};
    virtual classType classtype() const {return VOICE;};
    virtual Voice* clone() const = 0;
};

// main-voice; wrapper for a list of staff-objects
class MainVoice : public Voice
{
 public:
    StaffObjectList notes;     // content of the voice
    
 public:
    virtual bool is(classType type) const {return ((type == MAINVOICE) || Voice::is(type));};
    virtual classType classtype() const {return MAINVOICE;};
    virtual MainVoice* clone() const {return new MainVoice(*this);};
};

// sub-voice; wrapper for a list of note-objects
class SubVoice : public Voice
{
 public:
    Voice* parent;         // parent voice (either a main-voice or another sub-voice)
    bool on_top;           // voice insertion direction (for cursor movement control)
    VoiceObjectList notes; // content of the voice (no staff objects; i.e. clefs and key/time signatures)
    
 public:
    SubVoice(Voice* _parent) : parent(_parent), on_top(false) {};
    virtual bool is(classType type) const {return ((type == SUBVOICE) || Voice::is(type));};
    virtual classType classtype() const {return SUBVOICE;};
    virtual SubVoice* clone() const {return new SubVoice(*this);};
};


//
//     MOVABLE AND ATTACHABLE OBJECTS
//    ================================
//

// base class for movable, attachable objects (position data)
class Movable : public Class
{
 public:
    Position<> position;                   // position data (in micrometer or promille of head-height)
    enum Type {PAGE, LINE, STAFF, PARENT}; // position type (origin position; ignored for on-page objects)
    Type typeX, typeY;                     // position type for X- and Y-coordinate
    Appearance appearance;                 // graphical appearance properties
    
 public:
    Movable() : typeX(PAGE), typeY(PAGE) {};
    virtual bool is(classType type) const {return (type == MOVABLE);};
    virtual classType classtype() const {return MOVABLE;};
    virtual Movable* clone() const = 0;
    virtual ContextChanging* ctxchange() {return NULL;};
    virtual const ContextChanging* ctxchange() const {return NULL;};
};

// plain-text object (text with formatting information)
class PlainText
{
 public:
    std::string text;  // text to be shown (encoded in UTF-8)
    Font font;         // font of the text
};

// paragraph object (list of consequent plain-text objects with alignment information)
class Paragraph
{
 public:
    std::list<PlainText> text;                 // plain-text parts
    enum Align {LEFT, CENTER, RIGHT} align;    // align of the text
    bool justify;                              // justification flag
    
 public:
    Paragraph() : align(LEFT), justify(false) {};
};

// text-area object (movable list of consequent paragraphs)
class TextArea : public Movable
{
 public:
    unsigned int width;            // width of the text-area (in micrometer)
    unsigned int height;           // height of the text-area (in micrometer)
    std::list<Paragraph> text;     // paragraphs
    
 public:
    virtual bool is(classType type) const {return ((type == TEXTAREA) || (Movable::is(type)));};
    virtual classType classtype() const {return TEXTAREA;};
    virtual TextArea* clone() const {return new TextArea(*this);};
};

// context-changing object (see "context.hh")
class ContextChanging
{
 public:
    enum Type  // modifier type
    {
        NONE,      // do nothing
        ABSOLUTE,  // set the value to the modifier
        RELATIVE,  // add the modifier to the old value
        PROMILLE   // multiply the value with the modifier (in promille)
    };
    
    enum Scope {VOICE, STAFF, INSTRUMENT, GROUP, SCORE};   // scope of the change
    
    int tempo;                     // new tempo value/modifier
    Type tempo_type;               // tempo modifier type
    int volume;                    // new volume value/modifier
    Type volume_type;              // volume modifier type
    Scope volume_scope;            // volume scope
    unsigned int value_modifier;   // in promille
    Scope value_scope;             // value scope
    bool permanent;                // is this permanent, or for just this note?
    
 public:
    ContextChanging() : tempo(120), tempo_type(NONE),
                        volume(127), volume_type(NONE), volume_scope(VOICE),
                        value_modifier(0), value_scope(VOICE),
                        permanent(true) {};
};

// base class for interpretable symbols (has context-changing information)
class Symbol : public Movable
{
 private:
    ContextChanging ctxchanger;    // context-changing information
    
 public:
    virtual bool is(classType type) const {return ((type == SYMBOL) || (Movable::is(type)));};
    virtual classType classtype() const {return SYMBOL;};
    virtual Symbol* clone() const {return new Symbol(*this);};
    virtual ContextChanging* ctxchange() {return &ctxchanger;};
    virtual const ContextChanging* ctxchange() const {return &ctxchanger;};
};

// a movable object carrying arbitrary information for a plugin
class PluginInfo : public Movable
{
 private:
    char* data;             // data delivered to the plugin (will NOT be deleted with object!)
        
 public:
    std::string caption;    // caption, shown to the user
    std::string plugin;     // plugin id
        
 public:
    PluginInfo() : data(NULL) {};
    void reserve(const size_t size) {delete[] data; data = new char[size];};
    void free()                     {delete[] data; data = NULL;};
    operator const char* () const {return data;};
    operator       char* ()       {return data;};
    virtual bool is(classType type) const {return ((type == PLUGININFO) || (Movable::is(type)));};
    virtual classType classtype() const {return PLUGININFO;};
    virtual PluginInfo* clone() const {return new PluginInfo(*this);};
};

// a text-area with context-changing information
class Annotation : public TextArea
{
 private:
    ContextChanging ctxchanger;    // context-changing information
    
 public:
    virtual bool is(classType type) const {return ((type == ANNOTATION) || (TextArea::is(type)));};
    virtual classType classtype() const {return ANNOTATION;};
    virtual Annotation* clone() const {return new Annotation(*this);};
    virtual ContextChanging* ctxchange() {return &ctxchanger;};
    virtual const ContextChanging* ctxchange() const {return &ctxchanger;};
};

// a custom symbol is a symbol with custom graphical representation (sprite-id)
class CustomSymbol : public Symbol
{
 public:
    SpriteId sprite;   // sprite of the symbol
    
 public:
    virtual bool is(classType type) const {return ((type == CUSTOMSYMBOL) || (Symbol::is(type)));};
    virtual classType classtype() const {return CUSTOMSYMBOL;};
    virtual CustomSymbol* clone() const {return new CustomSymbol(*this);};
};

// base class for symbols with two anchor points
class Durable : public Symbol
{
 public:
    size_t duration;           // number of staff-objects within the scope of the symbol
    Position<> end_position;   // position of the end-node (same unit as position; see "Movable")
    
 public:
    Durable() : duration(1) {};
    virtual bool is(classType type) const {return ((type == DURABLE) || (Symbol::is(type)));};
    virtual classType classtype() const {return DURABLE;};
    virtual Durable* clone() const {return new Durable(*this);};
};

// legato slur (a symbol, rendered as a cubic bezier curve)
class Slur : public Durable
{
 public:
    Position<> control1;       // first control point        (same unit as position; see "Movable")
    Position<> control2;       // second control point       (same unit as position; see "Movable")
    unsigned int thickness1;   // line-width at the ends     (in promille of stem-width)
    unsigned int thickness2;   // line-width at the center   (in promille of stem-width)
    
 public:
    Slur() : thickness1(500), thickness2(2000) {};
    virtual bool is(classType type) const {return ((type == SLUR) || (Durable::is(type)));};
    virtual classType classtype() const {return SLUR;};
    virtual Slur* clone() const {return new Slur(*this);};
};

// crescendo and diminuendo "hairpin" symbols
class Hairpin : public Durable
{
 public:
    unsigned int thickness;    // line-width (in promille of stem-width)
    unsigned int height;       // height at the open end of the "hairpin" (in promille of head-height)
    bool crescendo;            // crescendo or decrescendo symbol?
    
 public:
    Hairpin() : thickness(1000), height(1000), crescendo(true) {};
    virtual bool is(classType type) const {return ((type == HAIRPIN) || (Durable::is(type)));};
    virtual classType classtype() const {return HAIRPIN;};
    virtual Hairpin* clone() const {return new Hairpin(*this);};
};

} // end namespace

#endif


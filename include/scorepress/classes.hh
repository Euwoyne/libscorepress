
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2016 Dominik Lehmann
  
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

#ifndef SCOREPRESS_CLASSES_HH
#define SCOREPRESS_CLASSES_HH

#include <string>          // std::string
#include <list>            // std::list

#include "basetypes.hh"    // mpx_t, tone_t, value_t, Position, Color, Font
#include "smartptr.hh"     // SmartPtr
#include "refptr.hh"       // RefPtr
#include "fraction.hh"     // Fraction
#include "sprite_id.hh"    // SpriteId
#include "parameters.hh"   // LayoutParam, StyleParam, ViewportParam
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------

// PROTOTYPES
class  EngraverState;       // engraver state class prototype (see "engraver_state.hh")
class  StaffContext;        // staff-context class prototype (see "context.hh")
class  PressState;          // press state class prototype (see "press.hh")
class  Renderer;            // renderer class prototype (see "renderer.hh")
class  ObjectCursor;        // object cursor prototype (see "object_cursor.hh")
class  Plate_pNote;         // pNote prototype (see "plate.hh")
class  Plate_pAttachable;   // pAttachable prototype (see "plate.hh")
class  Sprites;             // sprites prototype (see "sprites.hh")
struct DurableInfo;         // durable-information prototype (see "engrave_info.hh")

// BASE CLASSES
class SCOREPRESS_API Appearance;        // graphical appearance properties (visibility, color, scale)
class SCOREPRESS_API Class;             // abstract base class for all classes

// MAIN MUSIC OBJECT CLASSES
class SCOREPRESS_API VisibleObject;     // base class for visible objects
class SCOREPRESS_API SpriteObject;      // base class for objects, which appearance is defined by a sprite
class SCOREPRESS_API StaffObject;       // class for objects to reside within a staff (clefs, keys, time-signatures, notes and newlines)
class SCOREPRESS_API MusicObject;       // visible staff-objects
class SCOREPRESS_API Clef;              // a clef (contains sprite and note-positioning information)
class SCOREPRESS_API Key;               // class representing a key signature
class SCOREPRESS_API TimeSig;           // a time signature
class SCOREPRESS_API CustomTimeSig;     // a time signature with a custom sprite
class SCOREPRESS_API Barline;           // a barline
class SCOREPRESS_API VoiceObject;       // class for objects to reside within a voice (notes and newlines)
class SCOREPRESS_API Newline;           // newline indicator
class SCOREPRESS_API ScoreDimension;    // dimension of the on-page score-object
class SCOREPRESS_API Pagebreak;         // page-break indicator (with next page's layout information)
class SCOREPRESS_API NoteObject;        // class for played objects (visible voice-objects, which have a duration, as chords and rests)
class SCOREPRESS_API Chord;             // a chord (note-object consisting of several heads)
class SCOREPRESS_API Rest;              // a rest (inherits note-object interface)

// MISCELLANEOUS CLASSES (AS PARTS OF MAIN MUSIC CLASSES)
class SCOREPRESS_API AttachedObject;    // base class for visible objects, attached to notes
class SCOREPRESS_API Accidental;        // accidental abstraction (type, offset)
class SCOREPRESS_API Articulation;      // articulation symbol (temporarily context changing)
class SCOREPRESS_API Head;              // note-head class (with tone, accidental, etc.)
class SCOREPRESS_API TiedHead;          // note-head with tie-position information

// VOICE STRUCTURE CLASSES
class SCOREPRESS_API Voice;             // voice base type
class SCOREPRESS_API Staff;             // staff; wrapper for a list of staff-objects
class SCOREPRESS_API SubVoice;          // sub-voice; wrapper for a list of note-objects
class SCOREPRESS_API NamedVoice;        // named sub-voice (to be referenced by VoiceRef)

// MOVABLE AND ATTACHABLE OBJECTS
class SCOREPRESS_API Movable;           // base class for movable, attachable objects (position data)
class SCOREPRESS_API PlainText;         // plain-text object (text with formatting information)
class SCOREPRESS_API Paragraph;         // paragraph object (list of consequent plain-text objects with alignment information)
class SCOREPRESS_API TextArea;          // text-area object (movable list of consequent plain-text objects)
class SCOREPRESS_API ContextChanging;   // context-changing object (see "context.hh")
class SCOREPRESS_API Symbol;            // base class for interpretable symbols (has context-changing information)
class SCOREPRESS_API PluginInfo;        // a movable object carrying arbitrary information for a plugin
class SCOREPRESS_API Annotation;        // a text-area with context-changing information
class SCOREPRESS_API CustomSymbol;      // a custom symbol is a symbol with custom graphical representation (sprite-id)
class SCOREPRESS_API Durable;           // base class for symbols with two anchor points
class SCOREPRESS_API Slur;              // legato slur (a symbol, rendered as a cubic bezier curve)
class SCOREPRESS_API Hairpin;           // crescendo and diminuendo "hairpin" symbols

// TYPE DEFINTIONS
typedef SmartPtr<Movable,     CloneTrait> MovablePtr;       // smart pointer to movable object
typedef SmartPtr<Head,        CloneTrait> HeadPtr;          // smart pointer to head
typedef SmartPtr<StaffObject, CloneTrait> StaffObjectPtr;   // smart pointer to staff-object
typedef SmartPtr<VoiceObject, CloneTrait> VoiceObjectPtr;   // smart pointer to note-object
typedef SmartPtr<SubVoice,    CloneTrait> SubVoicePtr;      // smart pointer to sub-voice

typedef std::list<MovablePtr>             MovableList;      // list of smart pointers to movable objects
typedef std::list<HeadPtr>                HeadList;         // list of smart pointers to heads
typedef std::list<Articulation>           ArticulationList; // list of articulation symbols
typedef std::list<StaffObjectPtr>         StaffObjectList;  // list of smart pointers to staff-objects
typedef std::list<VoiceObjectPtr>         VoiceObjectList;  // list of smart pointers to note-objects
typedef std::list<SubVoicePtr>            SubVoiceList;     // list of smart pointers to sub-voices


//
//     BASE CLASSES
//    ==============
//

// graphical appearance properties (visibility, color, scale)
class SCOREPRESS_API Appearance
{
 public:
    bool       visible; // visibility
    Color      color;   // color
    promille_t scale;   // scaling
    
 public:
    Appearance() : visible(true), scale(1000) {color.r = color.g = color.b = 0; color.a = 255;}
};

// abstract base class for all classes
class SCOREPRESS_API Class
{
 public:
    // type enumeration
    enum classType {VISIBLEOBJECT,                     // ABSTRACT BASE CLASSES
                    STAFFOBJECT, MUSICOBJECT, CLEF,    // MAIN MUSIC OBJECT CLASSES
                                              KEY,
                                              TIMESIG, CUSTOMTIMESIG,
                                              BARLINE,
                    VOICEOBJECT, NEWLINE, PAGEBREAK,
                                 NOTEOBJECT, CHORD,
                                             REST,
                    
                    ATTACHEDOBJECT,                    // ATTACHED OBJECT BASE CLASS
                    ACCIDENTAL, ARTICULATION,          // SPECIAL ATTACHED OBJECTS
                    HEAD, TIEDHEAD,                    // SPECIAL HEADS
                    
                    VOICE, STAFF,                      // VOICE STRUCTURE CLASSES
                           SUBVOICE, NAMEDVOICE,
                    
                    MOVABLE, SCALABLE,                 // MOVABLE AND ATTACHABLE OBJECTS
                             TEXTAREA, ANNOTATION,
                             PLUGININFO,
                             SYMBOL, CUSTOMSYMBOL,
                                     DURABLE, SLUR,
                                              HAIRPIN,
                    
                    EXTERNAL};                         // EXTERNALLY DEFINED CLASS
    
 public:
    virtual bool is(classType type) const = 0;
    virtual classType classtype() const = 0;
    virtual Class* clone() const = 0;
    virtual ~Class();
};

// returns a human readable class name (for debugging purposes)
extern SCOREPRESS_API const std::string classname(Class::classType type);


//
//     MAIN MUSIC OBJECT CLASSES
//    ===========================
//

// base class for visible objects
class SCOREPRESS_API VisibleObject : virtual public Class
{
 public:
    MovableList attached;       // attached objects
    spohw_t     offset_x;       // horizontal offset
    Appearance  appearance;     // graphical appearance properties
    
 protected:
    VisibleObject() : offset_x(0) {}
    VisibleObject(const VisibleObject& vo, bool no_attached) :
        offset_x  (vo.offset_x),
        appearance(vo.appearance) {
        if (!no_attached)
            attached = vo.attached;
    }
    
 public:
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual VisibleObject* clone() const = 0;
};

// class for objects to reside within a staff (clefs, keys, time-signatures, notes and newlines)
class SCOREPRESS_API StaffObject : virtual public Class
{
 public:
    pohw_t acc_offset;          // accumulative horizontal offset
    
 protected: StaffObject() : acc_offset(0) {}
 public:
    virtual SpriteId get_sprite(const Sprites&) const {return SpriteId();}
    virtual VisibleObject& get_visible() = 0;
    virtual const VisibleObject& get_visible() const = 0;
    
    virtual void engrave(EngraverState& engraver) const = 0;
    virtual void render(Renderer& renderer, const Plate_pNote&, const PressState&) const = 0;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual StaffObject* clone() const = 0;
};

// visible staff-objects
class SCOREPRESS_API MusicObject : public StaffObject, public VisibleObject
{
 protected: MusicObject() {}
 public:
    virtual       VisibleObject& get_visible()       {return *this;}
    virtual const VisibleObject& get_visible() const {return *this;}
    
    virtual void engrave(EngraverState& engraver) const = 0;
    virtual void render(Renderer& renderer, const Plate_pNote&, const PressState&) const = 0;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual MusicObject* clone() const = 0;
};

// a clef (contains sprite and note-positioning information)
class SCOREPRESS_API Clef : public MusicObject
{
 public:
    SpriteId      sprite;       // sprite
    tone_t        base_note;    // tone residing on the specified line
    unsigned char line;         // 0 - first line, 1 - first space, 2 - second line, 3 - second space, 4 - third line, etc.
    tone_t        keybnd_sharp; // lowest tone for sharp-key display (to specify area of key signature)
    tone_t        keybnd_flat;  // lowest tone for flat-key display (to specify area of key signature)
    
 public:
    Clef() : base_note(67), line(5), keybnd_sharp(69), keybnd_flat(65) {}
    virtual SpriteId get_sprite(const Sprites&) const;
    virtual void engrave(EngraverState& engraver) const;
    virtual void render(Renderer& renderer, const Plate_pNote&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Clef* clone() const;
};

// class representing a key signature
class SCOREPRESS_API Key : public MusicObject
{
 public:
    enum Type {SHARP, FLAT};
    Type     type;              // key type
    char     number;            // number of "accidentals" (e.g. FLAT * 4 is As-Major or f-minor)
    SpriteId sprite;            // accidental sprite id
    
 public:
    Key() : type(SHARP), number(0) {}
    virtual SpriteId get_sprite(const Sprites&) const;
    virtual void engrave(EngraverState& engraver) const;
    virtual void render(Renderer& renderer, const Plate_pNote&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Key* clone() const;
};

// a time signature
class SCOREPRESS_API TimeSig : public MusicObject
{
 public:
    unsigned char number;  // number of beats per measure (enumerator)
    unsigned char beat;    // length of a beat in measure (denominator)
    
 public:
    TimeSig() : number(4), beat(4) {}
    void engrave(EngraverState& engraver, size_t setid) const;
    virtual void engrave(EngraverState& engraver) const;
    virtual void render(Renderer& renderer, const Plate_pNote&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual TimeSig* clone() const;
    
    inline const value_t beat_length() const {return value_t(static_cast<long>(number) << VALUE_BASE,
                                                             static_cast<long>(beat));}
};

// a time signature with a custom sprite
class SCOREPRESS_API CustomTimeSig : public TimeSig
{
 public:
    SpriteId sprite;   // custom sprite for the time signature
    
 public:
    virtual SpriteId get_sprite(const Sprites&) const;
    virtual void engrave(EngraverState& engraver) const;
    virtual void render(Renderer& renderer, const Plate_pNote&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual CustomTimeSig* clone() const;
};

// a barline
class SCOREPRESS_API Barline : public MusicObject
{
 public:
    typedef std::string Style;
    
    static const char* const singlebar;
    static const char* const doublebar;
    static const char* const endbar;
    Style style;
    
 public:
    Barline() : style(singlebar) {}
    Barline(const Style& s) : style(s) {}
    virtual void engrave(EngraverState& engraver) const;
    virtual void render(Renderer& renderer, const Plate_pNote&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Barline* clone() const;
};

// class for objects to reside within a voice (notes and newlines)
class SCOREPRESS_API VoiceObject : public StaffObject
{
 protected: VoiceObject() {}
 public:
    virtual VisibleObject& get_visible() = 0;
    virtual const VisibleObject& get_visible() const = 0;
    virtual void engrave(EngraverState& engraver) const = 0;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual VoiceObject* clone() const = 0;
};

// newline indicator
class SCOREPRESS_API Newline : public VoiceObject
{
 public:
    LayoutParam layout; // staff layout
    
 public:
    Newline() {}
    Newline(const LayoutParam& _layout) : layout(_layout) {}
    virtual VisibleObject& get_visible() {return *static_cast<VisibleObject*>(NULL);}
    virtual const VisibleObject& get_visible() const {return *static_cast<VisibleObject*>(NULL);}
    virtual void engrave(EngraverState&) const;
    virtual void render(Renderer&, const Plate_pNote&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Newline* clone() const;
};

// dimension of the on-page score-object
class SCOREPRESS_API ScoreDimension
{
 public:
    Position<um_t> position;    // position of the score-object (in micrometer)
    um_t           width;       // in micrometer
    um_t           height;      // in micrometer
    
 public:
    ScoreDimension() : width(0), height(0) {}
    bool contains(const Position<um_t>& pos);   // check, if the score-object contains a given point
};

// page-break indicator (with next page's layout information)
class SCOREPRESS_API Pagebreak : public Newline
{
 public:
    MovableList    attached;    // objects attached to the page
    ScoreDimension dimension;   // layout information
    
 public:
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Pagebreak* clone() const;
};

// class for played objects (music-objects, which have a duration, as chords and rests)
class SCOREPRESS_API NoteObject : public VoiceObject, public VisibleObject
{
 public:
    // value structure
    struct Value
    {
        unsigned exp  : 4;  // in [0, VALUE_BASE+2]
        unsigned dots : 4;  // in [0, exp]
    };
    
    // list of sub-voices attached to this note
    class SubVoices : public SubVoiceList
    {
     private:
        SubVoiceList::iterator below;   // first sub voice below this voice (others on top)
        
     public:
        SubVoices();                    // default constructor
        SubVoices(const SubVoices&);    // copy constructor (iterator initializing)
        
        SubVoices& operator = (const SubVoices&);
        
        SubVoicePtr& add_top();         // create new sub-voice atop all sub-voices
        SubVoicePtr& add_above();       // create new sub-voice above this voice
        SubVoicePtr& add_below();       // create new sub-voice below this voice
        SubVoicePtr& add_bottom();      // create new sub-voice below all sub-voices
        
        void remove(const Voice&);      // remove sub-voice
        
        // iterator checking
        inline bool is_first_below(const SubVoiceList::const_iterator it) const {return it == below;};
        inline bool is_first_below(const SubVoiceList::iterator       it)       {return it == below;};
    };
    
    // note object data
    Value         val;          // value of this note
    unsigned char irr_enum;     // tuplet enumerator
    unsigned char irr_denom;    // tuplet denominator
    int           staff_shift;  // note in different staff (if neq 0)
    SubVoices     subvoices;    // sub-voices attached to this note
    
 protected: NoteObject() : irr_enum(0), irr_denom(0), staff_shift(0) {val.exp = 5; val.dots = 0;}
 public:
    NoteObject(const NoteObject&, bool no_sub);
    
    virtual VisibleObject& get_visible() {return *this;}
    virtual const VisibleObject& get_visible() const {return *this;}
    virtual void engrave(EngraverState& engraver) const = 0;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual NoteObject* clone() const = 0;
     
    inline unsigned int base() const      {return 1u << val.exp;}   // 2^exp
    inline void set_exp(unsigned char e)  {val.exp  = e & 0x0F;}    // in [0, VALUE_BASE+2]
    inline void set_dots(unsigned char d) {val.dots = d & 0x0F;}    // in [0, exp]
    
    value_t set_value(value_t v);   // create note object from length (return error factor)
    const value_t value() const;    // calculate the note-object's value
};

// a chord (note-object consisting of several heads)
class SCOREPRESS_API Chord : public NoteObject
{
 public:
    // type enums
    enum StemType {STEM_CUSTOM,     // given by "stem.length"
                   STEM_UP,         // automatical (force upwards)
                   STEM_DOWN,       // automatical (force downwards)
                   STEM_VOICE,      // automatical (force direction as given by voice)
                   STEM_AUTO};      // automatical direction and length
    
    enum SlopeType {SLOPE_CUSTOM,   // given by "stem.slope"
                    SLOPE_STEM,     // given solely by the stems' lengths
                    SLOPE_BOUNDED,  // bounded by "stem.slope"
                    SLOPE_AUTO};    // bounded by style (see "StyleParam")
    
    enum BeamType {BEAM_NONE,       // no beam
                   BEAM_AUTO,       // beam according to "beam_group" (see "EngraverParam")
                   BEAM_FORCED,     // force beam
                   BEAM_CUT};       // force beam and cut all but the top one
    
    // stem information
    struct Stem
    {
        StemType    type;           // type (direction and length)
        spohh_t     length;         // length (for "STEM_CUSTOM" type)
        SlopeType   slope_type;     // slope calculation method
        spohh_t     slope;          // slope parameter (explicit or bound)
        Color       color;          // stem / beam color
        
        Stem() : type(STEM_VOICE), length(0), slope_type(SLOPE_AUTO), slope(0) {
            color.r = color.g = color.b = 0; color.a = 255;}
    };
    
 public:
    // TODO: sprite per head, not per chord
    HeadList         heads;             // heads of the chord (in ascending order)
    ArticulationList articulation;      // articulation symbols
    SpriteId         sprite;            // head sprite id
    Stem             stem;              // stem information
    BeamType         beam;              // type of the beam to the next note
    unsigned char    tremolo;           // tremolo beam count
    Color            flag_color;        // flag color
    
 private:
    spohh_t calculate_stem_length(const Staff&, const Voice&, const StaffContext&, const StyleParam&) const;
    
 public:
    Chord() : beam(BEAM_AUTO), tremolo(0) {}
    Chord(const Chord& c, bool no_sub);
    
    virtual SpriteId get_sprite(const Sprites&) const;
    virtual void engrave(EngraverState& engraver) const;
    virtual void render(Renderer& renderer, const Plate_pNote&, const PressState&) const;
            void render_beam(Renderer& renderer, const Plate_pNote&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Chord* clone() const;
};

// a rest (inherits note-object interface)
class SCOREPRESS_API Rest : public NoteObject
{
 public:
    pohh_t           offset_y;      // vertical offset (in promille of head-height)
                                    // (horizontal offset inherited from "NoteObject")
    Position<pohh_t> dot_offset;    // offset for the dots (in promille of head-height)
    SpriteId         sprite;        // rest sprite id
    
 public:
    Rest() : offset_y(0) {}
    Rest(const Rest&, bool no_sub);
    
    virtual SpriteId get_sprite(const Sprites&) const;
    virtual void engrave(EngraverState& engraver) const;
    virtual void render(Renderer& renderer, const Plate_pNote&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Rest* clone() const;
};


//
//     MISCELLANEOUS CLASSES (AS PARTS OF MAIN MUSIC CLASSES)
//    ========================================================
//

// base class for visible objects, attached to notes
class SCOREPRESS_API AttachedObject : virtual public Class
{
 public:
    Appearance appearance;      // graphical appearance properties
    
 protected: AttachedObject() {}
 public:
    virtual void render_decor(Renderer& renderer, const Plate_pAttachable&, const PressState&) const;
    virtual void render(Renderer& renderer, const Plate_pAttachable&, const PressState&) const = 0;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual AttachedObject* clone() const = 0;
};

// base class for sprites, attached to notes
class SCOREPRESS_API SpriteObject : public AttachedObject
{
 public:
    SpriteId sprite;            // sprite id
    
 protected: SpriteObject() {}
 public:
    virtual SpriteId get_sprite(const Sprites&) const {return sprite;}
    virtual void render(Renderer& renderer, const Plate_pAttachable&, const PressState&) const;
    virtual SpriteObject* clone() const = 0;
};

// accidental abstraction (type, offset)
class SCOREPRESS_API Accidental : public SpriteObject
{
 public:
    SCOREPRESS_LOCAL static const int note_modifier[9]; // tone modification for each accidental-type
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
    SpriteId sprite;    // sprite id
    Type     type;      // accidental type
    pohw_t   offset_x;  // horizontal offset (in promille of head-width)
    bool     force;     // force rendering (do not check key signature)
    
 public:
    Accidental() : type(natural), offset_x(0), force(false) {}
    virtual SpriteId get_sprite(const Sprites&) const;
    virtual bool is(classType _type) const;
    virtual classType classtype() const;
    virtual Accidental* clone() const;
};

// articulation symbol (temporarily context changing)
class SCOREPRESS_API Articulation : public SpriteObject
{
 public:
    SpriteId   sprite;          // sprite id
    pohh_t     offset_y;        // vertical offset (in promille of head-height)
    bool       far;             // symbol placed far from the heads (i.e. on top of the stem)
    promille_t value_modifier;  // value coefficient
    promille_t volume_modifier; // volume coefficient
    
 public:
    Articulation() : offset_y(0), far(false), value_modifier(0), volume_modifier(0) {}
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Articulation* clone() const;
};

// note-head class (with tone, accidental, etc.)
class SCOREPRESS_API Head : public Class
{
 public:
    tone_t            tone;         // as defined for MIDI: a' = 69
    Accidental        accidental;   // associated accidental
    Appearance        appearance;   // graphical appearance properties
    Position<spohh_t> dot_offset;   // offset for the dots
    
 public:
    Head() : tone(69) {}
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Head* clone() const;
};

// note-head with tie-position information
class SCOREPRESS_API TiedHead : public Head
{
 public:
    Position<spohh_t> offset1;      // offset of the first anchor
    Position<spohh_t> offset2;      // offset of the second anchor
    Position<spohh_t> control1;     // offset of the first control point
    Position<spohh_t> control2;     // offset of the second control point
    
 public:
    TiedHead() : Head() {}
    TiedHead(const Head& head) : Head(head) {}
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual TiedHead* clone() const;
};


//
//     VOICE STRUCTURE CLASSES
//    =========================
//

// voice base type
class SCOREPRESS_API Voice : public Class
{
 public:
    enum StemDirection {STEM_AUTO, STEM_UP, STEM_DOWN};
    
    StemDirection stem_direction;   // default stem direction of chords in this voice
    
 public:
    Voice() : stem_direction(STEM_AUTO) {}
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Voice* clone() const = 0;
};

// staff; wrapper for a list of staff-objects
class SCOREPRESS_API Staff : public Voice
{
 public:
    typedef SmartPtr<StyleParam> StyleParamPtr;
    
    // default head-heights (by rastrum number)
    static const uum_t rastrum[9];
    
 public:
    StaffObjectList notes;          // content of the staff
    SubVoiceList    subvoices;      // sub-voices associated with this staff
    
    pohh_t        offset_y;         // basic distance from the staff above
    uum_t         head_height;      // rastral (by head-height)
    unsigned int  line_count;       // number of lines in this staff
    bool          long_barlines;    // draw barlines down to the next staff?
    bool          curlybrace;       // curly brace for connecting staves of one instrument?
    bool          bracket;          // angular bracket for grouping instruments?
    uum_t         brace_pos;        // distance of the brace to the staff 
    uum_t         bracket_pos;      // distance of the bracket to the staff
    StyleParamPtr style;            // optional staff specific style parameters
    LayoutParam   layout;           // initial staff layout
    
    Staff() : offset_y(0), head_height(1875), line_count(5), long_barlines(false), curlybrace(false), bracket(false), brace_pos(500), bracket_pos(1000), style(NULL) {}
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Staff* clone() const;
};

// sub-voice; wrapper for a list of note-objects
class SCOREPRESS_API SubVoice : public Voice
{
 public:
    VoiceObjectList notes;  // content of the voice (no staff objects; i.e. clefs and key/time signatures)
    
 public:
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual SubVoice* clone() const;
};

// named sub-voice (to be referenced by VoiceRef)
class SCOREPRESS_API NamedVoice : public SubVoice
{
 public:
    std::string name;
    
 public:
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual NamedVoice* clone() const;
};


//
//     MOVABLE AND ATTACHABLE OBJECTS
//    ================================
//

// position with unit class
class UnitPosition
{
 public:
    // position coordinates (in micrometer or promille of head-height)
    Position<int> co;
    
    // position unit (micrometer or promille of head-height)
    enum Unit {METRIC, HEAD};
    struct UnitXY
    {
        Unit x;
        Unit y;
        
        UnitXY() : x(METRIC), y(METRIC) {}
    } unit;
    
    // grid origin (ignored for on-page objects)
    enum Origin {PAGE, LINE, STAFF, NOTE};
    struct OriginXY
    {
        Origin x;
        Origin y;
        
        OriginXY() : x(PAGE), y(PAGE) {}
    } orig;
};

// base class for movable, attachable objects (position data)
class SCOREPRESS_API Movable : public AttachedObject
{
 public:
    UnitPosition position;      // position data
    
 public:
    virtual void engrave(EngraverState& engraver) const;
    virtual void render(Renderer& renderer, const Plate_pAttachable&, const PressState&) const = 0;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Movable* clone() const = 0;
    
    static Position<mpx_t> engrave_pos(const UnitPosition& pos, const EngraverState& engraver);
    static Position<mpx_t> engrave_pos(const UnitPosition& pos, const ViewportParam& viewport, umpx_t head_height);
    
    virtual void register_nodes(ObjectCursor&);
    virtual ContextChanging* ctxchange();
    virtual const ContextChanging* ctxchange() const;
};

// base class for scalable, attachable objects (dimension data)
class SCOREPRESS_API Scalable : public Movable
{
 public:
    uum_t width;    // width
    uum_t height;   // height
    
    virtual bool is(classType type) const;
    virtual classType classtype() const;
};

// plain-text object (text with formatting information)
class SCOREPRESS_API PlainText
{
 public:
    std::string text;  // text to be shown (encoded in UTF-8)
    Font font;         // font of the text
};

// paragraph object (list of consequent plain-text objects with alignment information)
class SCOREPRESS_API Paragraph
{
 public:
    std::list<PlainText> text;                 // plain-text parts
    enum Align {LEFT, CENTER, RIGHT} align;    // align of the text
    bool justify;                              // justification flag
    
 public:
    Paragraph() : align(LEFT), justify(false) {}
};

// text-area object (movable list of consequent paragraphs)
class SCOREPRESS_API TextArea : public Scalable
{
 public:
    std::list<Paragraph> text;     // paragraphs
    
 public:
    virtual void engrave(EngraverState& engraver) const;
    virtual void render(Renderer&, const Plate_pAttachable&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual TextArea* clone() const;
};

// context-changing object (see "context.hh")
class SCOREPRESS_API ContextChanging
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
    
    int        tempo;           // new tempo value/modifier
    Type       tempo_type;      // tempo modifier type
    int        volume;          // new volume value/modifier
    Type       volume_type;     // volume modifier type
    Scope      volume_scope;    // volume scope
    promille_t value_modifier;  // value coefficient
    Scope      value_scope;     // value scope
    bool       permanent;       // is this permanent, or for just this note?
    
 public:
    ContextChanging() : tempo(120), tempo_type(NONE),
                        volume(127), volume_type(NONE), volume_scope(VOICE),
                        value_modifier(0), value_scope(VOICE),
                        permanent(true) {}
};

// base class for interpretable symbols (has context-changing information)
class SCOREPRESS_API Symbol : public Movable
{
 private:
    ContextChanging ctxchanger; // context-changing information
    
 public:
    virtual void render(Renderer& renderer, const Plate_pAttachable&, const PressState&) const = 0;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Symbol* clone() const = 0;
    virtual ContextChanging* ctxchange() {return &ctxchanger;}
    virtual const ContextChanging* ctxchange() const {return &ctxchanger;}
};

// a movable object carrying arbitrary information for a plugin
class SCOREPRESS_API PluginInfo : public Movable
{
 private:
    char* data;     // data delivered to the plugin (will NOT be deleted with object!)
        
 public:
    std::string caption;        // caption, shown to the user
    std::string plugin;         // plugin id
        
 public:
    PluginInfo() : data(NULL) {}
    void  reserve(const size_t size) {delete[] data; data = new char[size];}
    void  free()                     {delete[] data; data = NULL;}
    char* get_data()                 {return data;}
    virtual void render(Renderer& renderer, const Plate_pAttachable&, const PressState&) const = 0;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual PluginInfo* clone() const = 0;
};

// a text-area with context-changing information
class SCOREPRESS_API Annotation : public TextArea
{
 private:
    ContextChanging ctxchanger; // context-changing information
    
 public:
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Annotation* clone() const;
    virtual ContextChanging* ctxchange() {return &ctxchanger;}
    virtual const ContextChanging* ctxchange() const {return &ctxchanger;}
};

// a custom symbol is a symbol with custom graphical representation (sprite-id)
class SCOREPRESS_API CustomSymbol : public Symbol
{
 public:
    SpriteId sprite;            // sprite of the symbol
    
 public:
    virtual void engrave(EngraverState& engraver) const;
    virtual void render(Renderer&, const Plate_pAttachable&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual CustomSymbol* clone() const;
};

// base class for symbols with two anchor points
class SCOREPRESS_API Durable : public Symbol
{
 public:
    value_t      duration;      // duration of the symbol
    UnitPosition end;           // position of the end-node
    
 public:
    Durable() : duration(1) {}
    virtual void engrave(EngraverState& engraver) const;
    virtual void engrave(EngraverState& engraver, DurableInfo& info) const = 0;
    virtual void render(Renderer& renderer, const Plate_pAttachable&, const PressState&) const = 0;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Durable* clone() const = 0;
};

// legato slur (a symbol, rendered as a cubic bezier curve)
class SCOREPRESS_API Slur : public Durable
{
 public:
    UnitPosition control1;      // first control point
    UnitPosition control2;      // second control point
    promille_t   thickness1;    // line-width at the ends     (in promille of stem-width)
    promille_t   thickness2;    // line-width at the center   (in promille of stem-width)
    
 public:
    Slur() : thickness1(500), thickness2(2000) {}
    virtual void engrave(EngraverState& engraver, DurableInfo& info) const;
    virtual void render(Renderer&, const Plate_pAttachable&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Slur* clone() const;
};

// crescendo and diminuendo "hairpin" symbols
class SCOREPRESS_API Hairpin : public Durable
{
 public:
    promille_t thickness;       // line-width (in promille of stem-width)
    pohh_t     height;          // height at the open end of the "hairpin" (in promille of head-height)
    bool       crescendo;       // crescendo or decrescendo symbol?
    
 public:
    Hairpin() : thickness(1000), height(1000), crescendo(true) {}
    virtual void engrave(EngraverState& engraver, DurableInfo& info) const;
    virtual void render(Renderer&, const Plate_pAttachable&, const PressState&) const;
    virtual bool is(classType type) const;
    virtual classType classtype() const;
    virtual Hairpin* clone() const;
};

} // end namespace

#endif


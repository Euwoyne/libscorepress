
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

#ifndef SCOREPRESS_PARAMETERS_HH
#define SCOREPRESS_PARAMETERS_HH

#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class  SCOREPRESS_API ViewportParam;   // viewport information for the engraver (see "engraver.hh")
struct SCOREPRESS_API EngraverParam;   // parameter structure for the engraver class (see "engraver.hh")
struct SCOREPRESS_API StyleParam;      // style parameters for the engraving and rendering process
struct SCOREPRESS_API PressParam;      // style parameters for the rendering process (see "press.hh")
struct SCOREPRESS_API InterfaceParam;  // user input interface parameters for the user input interpretation (see "edit_cursor.hh")


//
//     class ViewportParam
//    =====================
//
// This structure contains viewport-specific parameters used during the
// engraving process to adapt to the client hardware.
//
class SCOREPRESS_API ViewportParam
{
 public:
    unsigned int hppm;  // horizontal viewport resolution (in pixels per meter)
    unsigned int vppm;  // vertical viewport resolution   (in pixels per meter)
    
    inline mpx_t umtopx_h(const double um) const {return static_cast<mpx_t>((um * hppm) / 1e3 + .5);};
    inline mpx_t umtopx_v(const double um) const {return static_cast<mpx_t>((um * vppm) / 1e3 + .5);};
    
    ViewportParam() : hppm(3780), vppm(3780) {};    // default to 96dpi
};

//
//     struct EngraverParam
//    ======================
//
// This structure contains the parameters which control the engraving process
// independently from the score object.
//
struct SCOREPRESS_API EngraverParam
{
    unsigned int min_distance;      // minimal graphical distance of note-heads of the same voice (in micrometer)
    unsigned int default_distance;  // graphical distance between non-note objects                (in micrometer)
    unsigned int barline_distance;  // graphical distance preceiding the first chord of a bar     (in micrometer)
    unsigned int accidental_space;  // space between an accidental and the previous note (in promille of head width)
    
    unsigned int exponent;          // note value exponent (in promille)
    unsigned int constant_coeff;    // constant additional distance (in micrometer)
    unsigned int linear_coeff;      // linear coefficient to the note value (in micrometer per whole note)
    
    unsigned int max_justification; // only justify, if the strech-factor is less than this (in promille)
    
    bool newline_time_reset;        // should the time-stamp be reset on newline?
    bool auto_barlines;             // engrave barlines automatically
    bool remember_accidentals;      // memorize accidentals
    
    unsigned char beam_group;       // default beam group
    
    Position<> tieup_offset1;       // default upward tie parameters
    Position<> tieup_offset2;
    Position<> tieup_control1;
    Position<> tieup_control2;
    
    Position<> tiedown_offset1;     // default downward tie parameters
    Position<> tiedown_offset2;
    Position<> tiedown_control1;
    Position<> tiedown_control2;
    
    // default parameters
    EngraverParam() : min_distance(1200),
                      default_distance(1400),
                      barline_distance(3200),
                      accidental_space(500),
                      exponent(1000),
                      constant_coeff(250),
                      linear_coeff(60000),
                      max_justification(2000),
                      newline_time_reset(true),
                      auto_barlines(true),
                      remember_accidentals(true),
                      beam_group(VALUE_BASE - 2),
                      tieup_offset1(100, -700),
                      tieup_offset2(-100, -700),
                      tieup_control1(1500, -600),
                      tieup_control2(-1500, -600),
                      tiedown_offset1(100, -300),
                      tiedown_offset2(-100, -300),
                      tiedown_control1(1500, 600),
                      tiedown_control2(-1500, 600) {};
};

//
//     struct StyleParam
//    ===================
//
// This structure provides parameters to control the engraving process
// dependant on the rendered staff (one instance per staff, and a default one
// within the press instance).
//
struct SCOREPRESS_API StyleParam
{
    unsigned int stem_width;        // in micrometer
    unsigned int ledger_length;     // ledger line length (in promille of head width)
    
    unsigned int flag_distance;     // distance of flags               (in promille of head height)
    unsigned int beam_distance;     // distance of beams               (in promille of head height)
    unsigned int beam_height;       // height of a beam                (in promille of head height)
    unsigned int shortbeam_length;  // length of a short one-note beam (in promille of head width)
    unsigned int shortbeam_short;   // length of a tiny short beam     (in promille of available space)
    
    unsigned int line_thickness;    // line-width for the staff        (in micrometer)
    unsigned int bar_thickness;     // line-width for the bar-lines    (in micrometer)
    unsigned int tie_thickness;     // line-width for the tie          (in micrometer)
    unsigned int ledger_thickness;  // line-width for the ledger-lines (in micrometer)
    
    // default parameters
    StyleParam() : stem_width(250),
                   ledger_length(1400),
                   flag_distance(1000),
                   beam_distance(200),
                   beam_height(600),
                   shortbeam_length(1000),
                   shortbeam_short(500),
                   line_thickness(200),
                   bar_thickness(400),
                   tie_thickness(300),
                   ledger_thickness(300) {};
};

//
//     struct PressParam
//    ===================
//
// This structure provides parameters to control the rendering process
// independently from the plate object.
//
struct SCOREPRESS_API PressParam
{
    unsigned int scale;             // printing scale (in promille)
    
    bool         draw_shadow;       // draw page shadow
    unsigned int shadow_color;      // color of page shadows         (in rgba, little endian)
    unsigned int shadow_offset;     // size of the page shadows      (in mpx)
    bool         draw_margin;       // draw page margin
    unsigned int margin_color;      // color of page margin          (in rgba, little endian)
    unsigned int cursor_width;      // cursor line-width             (in mpx)
    unsigned int cursor_distance;   // cursor distance from the note (in micrometer; should be less than "min-distance")
    
    bool draw_notebounds;           // draw boundary boxes of notes
    bool draw_attachbounds;         // draw boundary boxes of attachable objects
    bool draw_linebounds;           // draw boundary boxes of lines
    bool draw_eov;                  // draw end-of-voice objects
    unsigned int notebounds_color;      // color of boundary boxes       (in rgba, little endian)
    unsigned int virtualbounds_color;   // color of boundary boxes       (in rgba, little endian)
    unsigned int attachbounds_color;    // color of boundary boxes       (in rgba, little endian)
    unsigned int linebounds_color;      // color of boundary boxes       (in rgba, little endian)
    unsigned int eov_color;             // color of end-of-voice objects (in rgba, little endian)
    
    // default parameters
    PressParam() : scale(1000),
                   draw_shadow(true),
                   shadow_color(0x80000000),
                   shadow_offset(5000),
                   draw_margin(true),
                   margin_color(0xFFA0A0A0),
                   cursor_width(2000),
                   cursor_distance(400),
                   draw_notebounds(false),
                   draw_attachbounds(false),
                   draw_linebounds(false),
                   draw_eov(false),
                   notebounds_color(0xFF0000FF),
                   virtualbounds_color(0xFF0000C0),
                   attachbounds_color(0xFFFF0000),
                   linebounds_color(0xFF00FF00),
                   eov_color(0xFF800080) {}
    
    // apply scale
    inline double do_scale(const double coord) const {return (scale * coord) / 1000.0;};
};

//
//     struct InterfaceParam
//    =======================
//
// These parameters control the interpretation of the user input within the
// "UserCursor" instance.
//
//
struct SCOREPRESS_API InterfaceParam
{
    enum InputBase {LOWER_C = 0, LOWER_D, LOWER_E, LOWER_F, LOWER_G, LOWER_A, LOWER_B,
                    UPPER_C = 7, UPPER_D, UPPER_E, UPPER_F, UPPER_G, UPPER_A, UPPER_B,
                    NEAREST = 13};
    
    //enum InputOctave {SUBSUBCONTRA = 0, SUBCONTRA, CONTRA, GREAT, SMALL, LINE1, LINE2, LINE3, LINE4, LINE5, LINE6};
    
    InputBase input_base;           // user note input base
    bool relative_accidentals;      // user accidental input method
    bool prefer_natural;            // prefer natural accidentals to double-sharps/-flats
    
    int          stem_length;       // default stem length       (in promille of head-height)
    unsigned int accidental_offset; // default accidental offset (in promille of head-width)
    Position<>   dot_offset;        // default dot offset        (in promille of head-size)
    unsigned int newline_distance;  // default newline distance
    unsigned int autobeam_slope;    // default slope of automatic beams (in promille of head-height)
    
    // default parameters
    InterfaceParam() : input_base(LOWER_C),
                       relative_accidentals(true),
                       prefer_natural(true),
                       stem_length(3000),
                       accidental_offset(0),
                       dot_offset(),
                       newline_distance(3000),
                       autobeam_slope(500) {};
};

} // end namespace

#endif


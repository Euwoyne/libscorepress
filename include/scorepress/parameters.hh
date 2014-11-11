
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

#ifndef SCOREPRESS_PARAMETERS_HH
#define SCOREPRESS_PARAMETERS_HH

#include "basetypes.hh" // mpx_t, Position<>
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
    
    inline ViewportParam() : hppm(3780), vppm(3780) {}  // default to 96dpi
    
    // convert micrometer to millipixel (and vice versa)
    inline mpx_t  umtopx_h(const double um)  const {return static_cast<mpx_t>((um / 1e3) * hppm + .5);}
    inline mpx_t  umtopx_v(const double um)  const {return static_cast<mpx_t>((um / 1e3) * vppm + .5);}
    
    inline mpx_t  umtopx_h(const um_t   um)  const {return static_cast<mpx_t>((um / 1e3) * hppm + .5);}
    inline mpx_t  umtopx_v(const um_t   um)  const {return static_cast<mpx_t>((um / 1e3) * vppm + .5);}
    inline um_t   pxtoum_h(const mpx_t  mpx) const {return static_cast<um_t>((mpx * 1000.0) / hppm + .5);}
    inline um_t   pxtoum_v(const mpx_t  mpx) const {return static_cast<um_t>((mpx * 1000.0) / hppm + .5);}
    
    inline umpx_t umtopx_h(const uum_t  um)  const {return static_cast<umpx_t>((um / 1e3) * hppm + .5);}
    inline umpx_t umtopx_v(const uum_t  um)  const {return static_cast<umpx_t>((um / 1e3) * vppm + .5);}
    inline uum_t  pxtoum_h(const umpx_t mpx) const {return static_cast<uum_t>((mpx * 1000.0) / hppm + .5);}
    inline uum_t  pxtoum_v(const umpx_t mpx) const {return static_cast<uum_t>((mpx * 1000.0) / hppm + .5);}
    
    // setup viewport in dpi
    inline void set_dpi(double dpi)               {hppm = vppm = static_cast<unsigned int>(dpi / 0.0254 + .5);}
    inline void set_dpi(double hdpi, double vdpi) {hppm = static_cast<unsigned int>(hdpi / 0.0254 + .5);
                                                   vppm = static_cast<unsigned int>(vdpi / 0.0254 + .5);}
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
    // positioning parameters
    um_t         min_distance;      // minimal graphical distance of note-heads of the same voice (in micrometer)
    um_t         default_distance;  // graphical distance between non-note objects                (in micrometer)
    um_t         barline_distance;  // graphical distance preceiding the first chord of a bar     (in micrometer)
    um_t         nonnote_distance;  // graphical distance between non-note objects and notes      (in micrometer)
    pohh_t       accidental_space;  // space between an accidental and the previous note (in promille of head width)
    
    promille_t   exponent;          // note value exponent (in promille)
    um_t         constant_coeff;    // constant additional distance (in micrometer)
    unsigned int linear_coeff;      // linear coefficient to the note value (in micrometer per whole note)
    
    promille_t   max_justification; // only justify, if the strech-factor is less than this (in promille)
    
    // bar calculation parameters
    bool newline_time_reset;        // should the time-stamp be reset on newline?
    bool auto_barlines;             // engrave barlines automatically
    bool remember_accidentals;      // memorize accidentals
    
    // beam parameters
    unsigned char beam_group;       // default beam group
    
    // default tie parameters
    Position<> tieup_offset1;       // default upward tie parameters
    Position<> tieup_offset2;
    Position<> tieup_control1;
    Position<> tieup_control2;
    
    Position<> tiedown_offset1;     // default downward tie parameters
    Position<> tiedown_offset2;
    Position<> tiedown_control1;
    Position<> tiedown_control2;
    
    // default parameters
    EngraverParam();
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
    uum_t       stem_width;         // stem width
    pohw_t      ledger_length;      // ledger line length
    
    pohh_t      flag_distance;      // distance of flags
    pohh_t      beam_distance;      // distance of beams
    pohh_t      beam_height;        // height of a beam
    pohw_t      shortbeam_length;   // length of a short one-note beam
    promille_t  shortbeam_short;    // length of a tiny short beam
    
    uum_t       line_thickness;     // line-width for the staff
    uum_t       bar_thickness;      // line-width for the bar-lines
    uum_t       tie_thickness;      // line-width for the tie
    uum_t       ledger_thickness;   // line-width for the ledger-lines
    
    // default parameters
    StyleParam();
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
    // general render parameters
    promille_t   scale;             // printing scale (in promille)
    
    bool         draw_shadow;       // draw page shadow
    unsigned int shadow_color;      // color of page shadows         (in rgba, little endian)
    mpx_t        shadow_offset;     // size of the page shadows      (in mpx)
    bool         draw_margin;       // draw page margin
    unsigned int margin_color;      // color of page margin          (in rgba, little endian)
    mpx_t        cursor_width;      // cursor line-width             (in mpx)
    uum_t        cursor_distance;   // cursor distance from the note (in micrometer; should be less than "min-distance")
    
    // boundary box parameters
    bool draw_notebounds;           // draw boundary boxes of notes
    bool draw_attachbounds;         // draw boundary boxes of attachable objects
    bool draw_linebounds;           // draw boundary boxes of lines
    bool draw_eov;                  // draw end-of-voice objects
    
    unsigned int notebounds_color;      // color of boundary boxes       (in rgba, little endian)
    unsigned int virtualbounds_color;   // color of boundary boxes       (in rgba, little endian)
    unsigned int attachbounds_color;    // color of boundary boxes       (in rgba, little endian)
    unsigned int linebounds_color;      // color of boundary boxes       (in rgba, little endian)
    unsigned int eov_color;             // color of end-of-voice objects (in rgba, little endian)
    unsigned int decor_color;           // color of selection decoration (in rgba, little endian)
    
    // default parameters
    PressParam();
    
    // apply scale
    inline double do_scale(const double coord) const {return (scale * coord) / 1000.0;}
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
                    NEAREST = 14};
    
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
    InterfaceParam();
};

} // end namespace

#endif


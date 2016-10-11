
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

#include "parameters.hh"
using namespace ScorePress;

// engraver parameters
EngraverParam::EngraverParam() : min_distance(1200),
                                 default_distance(1600),
                                 barline_distance(3400),
                                 nonnote_distance(4200),
                                 accidental_space(500),
                                 exponent(707),
                                 constant_coeff(0),
                                 linear_coeff(600),
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
                                 tiedown_control2(-1500, 600) {}

// style parameters
StyleParam::StyleParam() : stem_length(3000),
                           stem_length_min(3000),
                           stem_width(250),
                           beam_slope_max(1000),
                           ledger_length(1400),
                           flag_distance(1000),
                           beam_distance(200),
                           beam_height(600),
                           shortbeam_length(1000),
                           shortbeam_short(500),
                           line_thickness(200),
                           bar_thickness(400),
                           tie_thickness(300),
                           ledger_thickness(300) {}

// layout parameters
LayoutParam::LayoutParam() : indent(0),
                             justify(false),
                             forced_justification(false),
                             right_margin(0),
                             distance(0),
                             auto_clef(true),
                             auto_key(true),
                             auto_timesig(false),
                             visible(true) {}

// press parameters
PressParam::PressParam() : scale(1000),
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
                           eov_color(0xFF800080),
                           decor_color(0xFF0000FF) {}

// interface parameters
InterfaceParam::InterfaceParam() : input_base(LOWER_C),
                                   relative_accidentals(true),
                                   prefer_natural(true),
                                   stem_length(3000),
                                   accidental_offset(0),
                                   dot_offset(),
                                   newline_distance(3000),
                                   autobeam_slope(500) {}


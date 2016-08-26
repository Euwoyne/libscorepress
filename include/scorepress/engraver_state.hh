
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

#ifndef SCOREPRESS_ENGRAVERSTATE_HH
#define SCOREPRESS_ENGRAVERSTATE_HH

#include "score.hh"          // Score, value_t, std::list
#include "pageset.hh"        // Pageset, Plate, ScoreContext, StaffContext, VoiceContext
#include "pick.hh"           // Pick
#include "sprites.hh"        // Sprites
#include "engrave_info.hh"   // StemInfo, BeamInfo, BeamInfoMap, TieInfo, TieInfoChord, TieInfoMap, SpaceInfo, LineInfo, DurableInfo
#include "reengrave_info.hh" // ReengraveInfo
#include "parameters.hh"     // EngraverParam, StyleParam, ViewportParam
#include "log.hh"            // Logging
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_LOCAL EngraverState;    // engraver, computing a renderable plate from an abstact score-object


//
//     class EngraverState
//    =====================
//
// This class represents the internal state of an "Engraver" instance during
// the engraving process. It contains all the information necessary for the
// engraving of a single object, including the "Pick" instance.
//
class SCOREPRESS_LOCAL EngraverState : public Logging
{
 private:
    // voice map type (maps "Voice" to the corresponding on-plate voice)
    typedef std::map<const Voice*, Plate::VoiceIt> VoiceMap;
    
    // initial parameters
    const Sprites*       sprites;           // pointer to the sprite-library
    const umpx_t         def_head_height;   // default head-height
    const EngraverParam* param;             // current engraver-parameters
    const StyleParam*    style;             // current staff-style
    const StyleParam&    default_style;     // default staff-style
    const ViewportParam* viewport;          // viewport-parameters
          ReengraveInfo* reengrave_info;    // reengrave information
    
    // info structures
    VoiceMap    voiceinfo;          // maps "VoiceCursor" to the corresponding on-plate voice
    BeamInfoMap beaminfo;           // beam-information for each voice
    TieInfoMap  tieinfo;            // tie positioning information
    SpaceInfo   spaceinfo;          // information about accidental- and cluster-spacing
    LineInfo    lineinfo;           // style information for the currently engraved line
    
    // internal pick instance
    Pick pick;
    
    // target instances
    Pageset*         pageset;       // target set of pages
    Pageset::PageIt  page;          // target page
    Pageset::PlateIt plateinfo;     // target plate-info
    RefPtr<Plate>    plate;         // target plate
    Plate::LineIt    pline;         // target on-plate line
    Plate::VoiceIt   pvoice;        // target on-plate voice
    Plate::NoteIt    pnote;         // target on-plate note
    
    // miscellaneous data
    size_t  pagecnt;                // page counter
    size_t  barcnt;                 // bar counter
    value_t start_time;             // line start time
    value_t end_time;               // line end time
    
 private:
    void engrave();             // engrave current note-object
    void create_lineend();      // calculate line-end info/line's rightmost border (see "Plate::pLine::line_end")
    void apply_offsets();       // apply all non-accumulative offsets
    
    void engrave_stems();       // engrave all stems within beams in the current line
    void engrave_attachables(); // engrave all attachables within the current line
    void engrave_braces();      // engrave braces and brackets for the current line
    
    void justify_line();        // justify the given line to fit into the score-area
    
 public:
    Pageset::ScoreDimension dimtopx(const ScoreDimension& dim) const;                     // convert score-dimension from micrometer to millipixel
    Position<mpx_t>         movable_pos(const Movable& obj, const Position<>& pos) const; // calculate on-plate position of movable object
    
 private:
    // break all ties at the specified x-position
    static void break_ties(TieInfoChord& tieinfo, const mpx_t endpos, const mpx_t restartpos, const mpx_t head_height);
    
 public:
    // constructor (will erase "score" from the "pageset" and prepare for engraving)
    EngraverState(const Score&         score,       // score object to be engraved
                  const size_t         start_page,  // page to begin the score on
                        Pageset&       pageset,     // target pageset
                  const Sprites&       sprites,     // sprite set
                  const umpx_t         head_height, // default head-height (may be overridden by score)
                  const EngraverParam& parameters,  // default engraver parameters (may be overridden by score)
                  const StyleParam&    style,       // default staff-style (may be overridden by score)
                  const ViewportParam& viewport);   // viewport parameters
    
    void set_reengrave_info(ReengraveInfo& info);
    
    // state access
    inline const Sprites&            get_sprites()      const {return *sprites;}
    inline const EngraverParam&      get_parameters()   const {return *param;}
    inline const StyleParam&         get_style()        const {return *style;}
    inline const ViewportParam&      get_viewport()     const {return *viewport;}
    inline const Score&              get_score()        const {return pick.get_score();}
    
    inline const StaffObject*        get_note()         const {return &*pick.get_cursor();}
    inline const Staff&              get_staff()        const {return pick.get_cursor().staff();}
    inline const Voice&              get_voice()        const {return pick.get_cursor().voice();}
    inline const value_t&            get_time()         const {return pick.get_cursor().time;}
    inline const value_t&            get_ntime()        const {return pick.get_cursor().ntime;}
    inline const umpx_t&             get_head_height()  const {return pvoice->head_height;}
    
    inline const Pageset::PageIt     get_target_page_it()     {return page;}
    inline const Plate::LineIt&      get_target_line_it()     {return pline;}
    inline const Plate::VoiceIt&     get_target_voice_it()    {return pvoice;}
    inline const Plate::NoteIt&      get_target_it()          {return pnote;}
    
    inline const Pageset&            get_pageset()      const {return *pageset;}
    inline const Pageset::PlateInfo& get_plateinfo()    const {return *plateinfo;}
    inline const Pageset::pPage&     get_target_page()  const {return *page;}
    inline const Plate::pLine&       get_target_line()  const {return *pline;}
    inline const Plate::pVoice&      get_target_voice() const {return *pvoice;}
    inline const Plate::pNote&       get_target()       const {return *pnote;}
    
    inline       Pageset&            get_pageset()            {return *pageset;}
    inline       Pageset::PlateInfo& get_plateinfo()          {return *plateinfo;}
    inline       Plate::pLine&       get_target_line()        {return *pline;}
    inline       Plate::pVoice&      get_target_voice()       {return *pvoice;}
    inline       Plate::pNote&       get_target()             {return *pnote;}
    
    inline       ScoreContext&       get_scorectx()           {return pline->context;}
    inline       StaffContext&       get_staffctx()           {return pline->staffctx[&get_staff()];}
    inline       VoiceContext&       get_voicectx()           {return pvoice->context;}
    
    // miscellaneous state info
    inline unsigned int default_head_height(unsigned int h = 0) const {return (h ? h : def_head_height);}
    inline mpx_t        min_distance()                          const {return viewport->umtopx_h(param->min_distance);}
    inline mpx_t        barline_distance()                      const {return viewport->umtopx_h(param->barline_distance);}
    inline bool         eos()                                   const {return pick.eos();}
    
    const Staff& get_visual_staff() const;   // get the staff, in which the note is drawn (i.e. apply staff-shift)
    
    // engraving methods
    bool engrave_next();                                            // engrave currently referenced object and prepare the next object
    void engrave_beam(const Chord& chord, const StemInfo& info);    // calculate beam end information
    
    // tie control interface
    void add_tieinfo(const TiedHead& thead); // add tie-information for the given tied head
    bool has_tie(const Head& head);          // check, if "head" has got any ties (otherwise "get_tieinfo" fails)
    TieInfo& get_tieinfo(const Head& head);  // get tie-information about the given head
    void erase_tieinfo(const Head& head);    // erase tie-information (for one head)
    void erase_tieinfo();                    // erase tie-information (for the voice)
    void break_ties();                       // break ties at the object (requires correct gphBox)
    
    // offset and space control interface
    void add_offset(const mpx_t offset);              // add the given offset in front of the note to be engraved
    void add_distance_after(mpx_t dst, value_t time); // apply additional distance to notes after a given time
    bool has_cluster_space();                         // check, if this chord has been moved yet, due to clustering
    bool has_accidental_space();                      // check, if this chord has been moved yet, due to clustering
    
    // logging control
    using Logging::log_set;
    void log_set(Log& log);
    void log_unset();
};

// inline method implementations
inline void     EngraverState::set_reengrave_info(ReengraveInfo& info)     {reengrave_info = &info;}
inline void     EngraverState::add_tieinfo(const TiedHead& thead)          {tieinfo[&get_voice()][thead.tone].source = &thead; tieinfo[&get_voice()][thead.tone].target = &pnote->ties.back();}
inline bool     EngraverState::has_tie(const Head& head)                   {return (tieinfo[&get_voice()].find(head.tone) != tieinfo[&get_voice()].end());}
inline TieInfo& EngraverState::get_tieinfo(const Head& head)               {return tieinfo[&get_voice()][head.tone];}
inline void     EngraverState::erase_tieinfo(const Head& head)             {tieinfo[&get_voice()].erase(head.tone);}
inline void     EngraverState::erase_tieinfo()                             {tieinfo[&get_voice()].clear();}
inline void     EngraverState::break_ties()                                {break_ties(tieinfo[&get_voice()], pnote->gphBox.pos.x, pnote->gphBox.right(), pvoice->head_height);}
inline void     EngraverState::add_distance_after(mpx_t dst, value_t time) {pick.add_distance_after(dst, time);}

inline bool EngraverState::has_cluster_space() {
    if (spaceinfo.leftcluster_host == &*pick.get_cursor()) return false;
    spaceinfo.leftcluster_host = &*pick.get_cursor();
    return true;}

inline bool EngraverState::has_accidental_space() {
    if (spaceinfo.accidental_time == pick.get_cursor().time) return false;
    spaceinfo.accidental_time = pick.get_cursor().time;
    return true;}

} // end namespace

#endif


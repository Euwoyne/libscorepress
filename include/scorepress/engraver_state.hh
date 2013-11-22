
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

#ifndef SCOREPRESS_ENGRAVERSTATE_HH
#define SCOREPRESS_ENGRAVERSTATE_HH

#include "score.hh"        // Score, value_t, std::list
#include "pageset.hh"      // Pageset, Plate, ScoreContext, StaffContext, VoiceContext
#include "pick.hh"         // Pick
#include "sprites.hh"      // Sprites
#include "engrave_info.hh" // StemInfo, BeamInfo, BeamInfoMap, TieInfo, TieInfoChord, TieInfoMap, SpaceInfo, LineInfo
#include "parameters.hh"   // EngraverParam, StyleParam, ViewportParam
#include "log.hh"          // Logging
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
    // typedefs for cleaner class interface
    typedef std::list<Plate::pNote>::iterator        pNoteIt;
    typedef std::list<Plate::pVoice>::iterator       pVoiceIt;
    typedef std::list<Plate::pLine>::iterator        pLineIt;
    typedef std::list<Pageset::pPage>::iterator      pPageIt;
    typedef std::list<Pageset::PlateInfo>::iterator  PlateInfoIt;
    
 private:
    // initial parameters
    const Sprites*       sprites;           // pointer to the sprite-library
    const EngraverParam* param;             // current engraver-parameters
    const StyleParam*    style;             // current staff-style
    const StyleParam&    default_style;     // default staff-style
    const ViewportParam* viewport;          // viewport-parameters (see "parameters.hh")
    
    // info structures
    BeamInfoMap beaminfo;           // beam-information for each voice
    TieInfoMap  tieinfo;            // tie positioning information
    SpaceInfo   spaceinfo;          // information about accidental- and cluster-spacing
    LineInfo    lineinfo;           // style information for the currently engraved line
    
    // internal pick instance
    Pick pick;
    
    // target instances
    Pageset*       pageset;         // target set of pages
    PlateInfoIt    plateinfo;       // target plate-info
    RefPtr<Plate>  plate;           // target plate
    pLineIt        pline;           // target on-plate line
    pVoiceIt       pvoice;          // target on-plate voice
    Plate::pNote*  pnote;           // target on-plate note
    
    // miscellaneous data
    pPageIt      page;              // current page
    mpx_t        head_height;       // precalculated head-height
    unsigned int pagecnt;           // page counter
    unsigned int barcnt;            // bar counter
    value_t      start_time;        // line start time
    value_t      end_time;          // line end time
    
 private:
    void engrave();             // engrave current note-object
    void create_lineend();      // calculate line-end info/line's rightmost border (see "Plate::pLine::line_end")
    void apply_offsets();       // apply all non-accumulative offsets
    
    void begin_durable(const Durable& source, DurableInfo& target, const Plate::pVoice& voice, Plate::pNote& note);
    void end_durable(const Durable& source, Plate::pDurable& target, const Plate::pVoice& voice, const Plate::pNote& pnote);
    void engrave_stems();       // engrave all stems within beams in the current line
    void engrave_attachables(); // engrave all attachables within the current line
    void engrave_braces();      // engrave braces and brackets for the current line
    
    void justify_line();        // justify the given line to fit into the score-area
    
    Pageset::ScoreDimension dimtopx(const ScoreDimension& dim); // convert score-dimension from micrometer to millipixel
    
 private:
    // break all ties at the specified x-position
    static void break_ties(TieInfoChord& tieinfo, const mpx_t endpos, const mpx_t restartpos, const mpx_t head_height);
    
    // calculate the graphical box for (1. the voices within a line; 2. the given bezier spline)
    static void calculate_gphBox(Plate::pLine& line);
    static Plate::GphBox calculate_gphBox(Position<mpx_t> p1, Position<mpx_t> c1, Position<mpx_t> c2, Position<mpx_t> p2, mpx_t w0 = 0, mpx_t w1 = 0);
    
 public:
    // constructor (will erase "score" from the "pageset" and prepare for engraving)
    EngraverState(const Score&         score,       // score object to be engraved
                  const unsigned int   start_page,  // page to begin the score on
                        Pageset&       pageset,     // target pageset
                  const Sprites&       sprites,     // sprite set
                  const EngraverParam& parameters,  // engraver parameters
                  const StyleParam&    style,       // default staff-style (may be overridden by score)
                  const ViewportParam& viewport);   // viewport parameters
    
    // state access
    inline const Sprites&       get_sprites()      const {return *sprites;};
    inline const EngraverParam& get_parameters()   const {return *param;};
    inline const StyleParam&    get_style()        const {return *style;};
    inline const ViewportParam& get_viewport()     const {return *viewport;};
    inline const Score&         get_score()        const {return pick.get_score();};
    
    inline const Plate::pLine&  get_target_line()  const {return *pline;};
    inline const Plate::pVoice& get_target_voice() const {return *pvoice;};
    inline const Plate::pNote&  get_target()       const {return *pnote;};
    
    inline const StaffObject*   get_note()         const {return &*pick.get_cursor();};
    inline const Staff&         get_staff()        const {return pick.get_cursor().staff();};
    inline const Voice&         get_voice()        const {return pick.get_cursor().voice();};
    inline const value_t&       get_time()         const {return pick.get_cursor().time;};
    inline const mpx_t&         get_head_height()  const {return head_height;};
    
    inline Plate::pLine&        get_target_line()  {return *pline;};
    inline Plate::pVoice&       get_target_voice() {return *pvoice;};
    inline Plate::pNote&        get_target()       {return *pnote;};
    inline ScoreContext&        get_scorectx()     {return pline->context;};
    inline StaffContext&        get_staffctx()     {return pline->staffctx[&get_staff()];};
    inline VoiceContext&        get_voicectx()     {return pvoice->context;};
    
    inline mpx_t min_distance()     const {return viewport->umtopx_h(param->min_distance);};
    inline mpx_t barline_distance() const {return viewport->umtopx_h(param->barline_distance);};
    
    inline bool eos() {return pick.eos();};
    
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
    void log_set(Log& log);
    void log_unset();
};

// inline method implementations
inline void     EngraverState::add_tieinfo(const TiedHead& thead)          {tieinfo[&get_voice()][thead.tone].source = &thead; tieinfo[&get_voice()][thead.tone].target = &pnote->ties.back();}
inline bool     EngraverState::has_tie(const Head& head)                   {return (tieinfo[&get_voice()].find(head.tone) != tieinfo[&get_voice()].end());}
inline TieInfo& EngraverState::get_tieinfo(const Head& head)               {return tieinfo[&get_voice()][head.tone];}
inline void     EngraverState::erase_tieinfo(const Head& head)             {tieinfo[&get_voice()].erase(head.tone);}
inline void     EngraverState::erase_tieinfo()                             {tieinfo[&get_voice()].clear();}
inline void     EngraverState::break_ties()                                {break_ties(tieinfo[&get_voice()], pnote->gphBox.pos.x, pnote->gphBox.right(), head_height);}
inline void     EngraverState::add_distance_after(mpx_t dst, value_t time) {pick.add_distance_after(dst, time);}
inline bool     EngraverState::has_cluster_space()
    {if (spaceinfo.leftcluster_host == &*pick.get_cursor()) return false;
     spaceinfo.leftcluster_host = &*pick.get_cursor();
     return true;}
inline bool EngraverState::has_accidental_space()
    {if (spaceinfo.accidental_time == pick.get_cursor().time) return false;
     spaceinfo.accidental_time = pick.get_cursor().time;
     return true;}

} // end namespace

#endif


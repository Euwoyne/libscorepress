
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

#ifndef SCOREPRESS_SPRITES_HH
#define SCOREPRESS_SPRITES_HH

#include <string>       // std::string
#include <vector>       // std::vector
#include <map>          // std::map
#include <deque>        // std::deque

#include "sprite_id.hh" // SpriteId
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API SpriteInfo;    // sprite graphic information
class SCOREPRESS_API SpriteSet;     // set of sprites
class SCOREPRESS_API Sprites;       // set of sprite-sets


//
//     class SpriteInfo
//    ==================
//
// Meta-information structure for one sprite graphic.
// This contains the dimension as well as all provided attributes for a single
// graphic.
//
class SCOREPRESS_API SpriteInfo
{
 public:
    // sprite type
    enum Type {HEADS_LONGA, HEADS_BREVE, HEADS_WHOLE, HEADS_HALF, HEADS_QUARTER,
               RESTS_LONGA, RESTS_BREVE, RESTS_WHOLE, RESTS_HALF, RESTS_QUARTER,
               FLAGS_NOTE, FLAGS_OVERLAY, FLAGS_REST, FLAGS_BASE,
               ACCIDENTALS_NATURAL,
               ACCIDENTALS_DOUBLEFLAT, ACCIDENTALS_FLATANDAHALF, ACCIDENTALS_FLAT, ACCIDENTALS_HALFFLAT,
               ACCIDENTALS_DOUBLESHARP, ACCIDENTALS_SHARPANDAHALF, ACCIDENTALS_SHARP, ACCIDENTALS_HALFSHARP,
               BRACE, BRACKET,
               DOT,
               TIMESIG, TIMESIG_DIGIT,
               CLEF, ARTICULATION, SYMBOL, SYMBOL_STR, GLYPH};
    
    Type type;          // sprite type
    int width;          // sprite width
    int height;         // sprite height
    std::string path;   // svg path-name
    
    std::map<std::string, std::string> name;    // sprite name (internationalized; UTF8)
    std::map<std::string, std::string> text;    // text properties
    std::map<std::string, double>      real;    // floating-point properties
    std::map<std::string, int>         integer; // integer properties
    
    SpriteInfo(Type type);  // constructor
    
    // easy access methods (with default returns for non-existing values)
    bool has_text   (const std::string& key) const;
    bool has_real   (const std::string& key) const;
    bool has_integer(const std::string& key) const;
    
    const std::string& get_text   (const std::string& key) const;
          double       get_real   (const std::string& key) const;
          int          get_integer(const std::string& key) const;
};

inline bool SpriteInfo::has_text   (const std::string& key) const {return text.find(key) != text.end();}
inline bool SpriteInfo::has_real   (const std::string& key) const {return real.find(key) != real.end();}
inline bool SpriteInfo::has_integer(const std::string& key) const {return integer.find(key) != integer.end();}


//
//     class SpriteSet
//    =================
//
// This class is a set of sprites provided by one svg-file.
//
class SCOREPRESS_API SpriteSet : public std::vector<SpriteInfo>
{
 public:
    // sprite typeface
    struct Typeface
    {
        std::string id;                             // typeface id
        std::map<std::string, std::string> name;    // typeface name (internationalized; UTF8)
        double ascent;                              // typeface ascent
        double descent;                             // typeface descent (usually negative)
        bool general_use;                           // allow use in text-fields
        bool custom_use;                            // allow creation of custom symbols
        std::map<std::string, size_t> glyphs;       // glyph sprite ids
    };
    
    // symbol group
    struct Group
    {
        std::string id;                             // group id
        std::map<std::string, std::string> name;    // group name (internationalized; UTF8)
        std::vector<size_t> sprites;                // group content (sprite-ids)
    };
    
 public:
    // set information
    std::string file;                           // source file
    std::string title;                          // set title
    unsigned int head_height;                   // the sprite set's internal head-height (in pixel)
    unsigned int timesig_digit_space;           // space between timesignature-digits (in pixel)
    
    std::map<std::string, std::string> info;    // meta information
    
    std::vector<Group> groups;                  // group list
    std::vector<Typeface> typefaces;            // typeface list
    
    std::map<std::string, size_t> ids;          // map sprite-ids to index
    std::map<std::string, size_t> gids;         // group-ids
    std::map<std::string, size_t> fids;         // typeface-ids
    
    // default-symbol ids
    size_t heads_longa;     // longa head
    size_t heads_breve;     // breve head
    size_t heads_whole;     // whole-note head
    size_t heads_half;      // half-note head
    size_t heads_quarter;   // quarter-note head
    
    size_t rests_longa;     // longa rest
    size_t rests_breve;     // breve rest
    size_t rests_whole;     // whole rest
    size_t rests_half;      // half rest
    size_t rests_quarter;   // quarter rest
    
    size_t flags_note;      // flag for short notes
    size_t flags_overlay;   // upper flag for short notes (to be overlaid by other flags)
    size_t flags_rest;      // flag for short rests
    size_t flags_base;      // truncation sprite for short rests
    
    size_t accidentals_double_flat;     // double flat accidental
    size_t accidentals_flat_andahalf;   // flat and a half accidental
    size_t accidentals_flat;            // flat accidental
    size_t accidentals_half_flat;       // half flat accidental
    size_t accidentals_natural;         // natural accidental
    size_t accidentals_half_sharp;      // half sharp accidental
    size_t accidentals_sharp;           // sharp accidental
    size_t accidentals_sharp_andahalf;  // sharp and a half accidental
    size_t accidentals_double_sharp;    // double sharp accidental
    
    size_t brace;               // curly staff brace
    size_t bracket;             // instrument group bracket
    size_t dot;                 // dot (for dotted notes)
    
    size_t digits_time[10];     // time-signature digits
    
    size_t undefined_symbol;    // symbol for undefined sprites
    
    // public methods
    SpriteSet();                        // default constructor (initializing ids with UNDEFINED)
    void clear();                       // erase the vector and reset sprite-ids
    
    size_t index(const std::string& id) const;              // get index by sprite-id
    
          SpriteInfo& get(const std::string& id);           // get info by sprite-id
    const SpriteInfo& get(const std::string& id) const;
    
          SpriteInfo& operator[] (const size_t& idx);       // get info by index
    const SpriteInfo& operator[] (const size_t& idx) const;
};

// get info by sprite-id
inline SpriteInfo& SpriteSet::get(const std::string& id) {
    return SpriteSet::operator[](index(id));}

inline const SpriteInfo& SpriteSet::get(const std::string& id) const {
    return SpriteSet::operator[](index(id));}

// get info by index
inline SpriteInfo& SpriteSet::operator[] (const size_t& idx) {
    return std::vector<SpriteInfo>::operator[]((idx < size()) ? idx : undefined_symbol);}

inline const SpriteInfo& SpriteSet::operator[] (const size_t& idx) const {
    return std::vector<SpriteInfo>::operator[]((idx < size()) ? idx : undefined_symbol);}


//
//     class Sprites
//    ===============
//
// A set of sprite-sets used to render a document.
//
class SCOREPRESS_API Sprites : public std::deque<SpriteSet>
{
 public:
    inline       SpriteSet& operator[] (const size_t& idx);
    inline const SpriteSet& operator[] (const size_t& idx) const;
    
    inline       SpriteInfo& operator[] (const SpriteId& id);
    inline const SpriteInfo& operator[] (const SpriteId& id) const;
    
    inline unsigned int head_height(const SpriteId& id) const;
    inline          int head_width (const SpriteId& id) const;
};

inline SpriteSet& Sprites::operator[] (const size_t& idx) {
    return std::deque<SpriteSet>::operator[](idx);}

inline const SpriteSet& Sprites::operator[] (const size_t& idx) const {
    return std::deque<SpriteSet>::operator[](idx);}

inline SpriteInfo& Sprites::operator[] (const SpriteId& id) {
    return std::deque<SpriteSet>::operator[](id.setid)[id.spriteid];}

inline const SpriteInfo& Sprites::operator[] (const SpriteId& id) const {
    return std::deque<SpriteSet>::operator[](id.setid)[id.spriteid];}

inline unsigned int Sprites::head_height(const SpriteId& id) const {
    return std::deque<SpriteSet>::operator[](id.setid).head_height;}

inline int Sprites::head_width (const SpriteId& id) const {
    return std::deque<SpriteSet>::operator[](id.setid)[std::deque<SpriteSet>::operator[](id.setid).heads_quarter].width;}

} // end namespace

#endif


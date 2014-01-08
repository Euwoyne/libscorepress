
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2014 Dominik Lehmann
  
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

#ifndef SCOREPRESS_SCORE_HH
#define SCOREPRESS_SCORE_HH

#include <list>             // std::list
#include <map>              // std::map
#include "classes.hh"       // [score classes]
#include "parameters.hh"    // StyleParam
#include "smartptr.hh"      // SmartPtr
#include "meta.hh"          // Meta, DocumentMeta
#include "error.hh"         // Error
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API Staff;        // staff object (inheriting one main-voice)
class SCOREPRESS_API Score;        // score object (one musical score on consequent pages)
class SCOREPRESS_API Document;     // document object (several musical scores and score-independent objects)


// staff object (inheriting one main-voice)
class SCOREPRESS_API Staff : public MainVoice
{
 public:
    typedef SmartPtr<StyleParam> StyleParamPtr;
    
 public:
    int offset_y;               // in promille of head-height
    
    unsigned int head_height;   // in micrometer
    unsigned int line_count;    // number of lines in this staff
    bool long_barlines;         // draw barlines down to the next staff?
    bool curlybrace;            // curly brace for connecting staves of one instrument?
    bool bracket;               // angular bracket for grouping instruments?
    unsigned int brace_pos;     // distance of the brace to the staff 
    unsigned int bracket_pos;   // distance of the bracket to the staff
    StyleParamPtr style;        // optional staff specific style parameters
    Newline layout;             // staff layout on the first page
    
    Staff();                    // default constructor
};

// score object (one musical score on consequent pages)
class SCOREPRESS_API Score
{
 public:
    class StaffNotFound : public ScorePress::Error  // thrown by staff-finding methods, if the given staff does not exist
    {public: StaffNotFound();};
    
    struct Layout   // first page layout
    {
        // TODO: user defined brace scale parameters
        ScoreDimension dimension;   // layout information
        SpriteId brace_sprite;      // sprite for braces
        SpriteId bracket_sprite;    // sprite for brackets
        MovableList attached;       // objects attached to the page
    };
    
    typedef SmartPtr<EngraverParam> EngraverParamPtr;
    typedef SmartPtr<StyleParam>    StyleParamPtr;
    
 public:
    std::list<Staff> staves;    // the staves within the score
    Layout layout;              // layout information for the first page
                                // (consequent page information given with pagebreak-objects)
    EngraverParamPtr param;     // optional engraver parameters
    StyleParamPtr style;        // optional style parameters
    Meta meta;                  // meta information
    
 public:
    // get an iterator for a given staff
    std::list<Staff>::const_iterator get_staff(const Staff& staff) const throw(StaffNotFound);
    
    // check, whether the given staves belong to one instrument or instrument-group respectively
    bool same_instrument(const Staff& staff1, const Staff& staff2) const throw(StaffNotFound);
    bool same_group(const Staff& staff1, const Staff& staff2) const throw(StaffNotFound);
};

// document object (several musical scores and score-independent objects)
class SCOREPRESS_API Document
{
 public:
    // page dimension data
    class PageDimension
    {
     public:
        unsigned int width;     // in micrometer
        unsigned int height;    // in micrometer
        struct
        {
            unsigned int top;       // in micrometer
            unsigned int bottom;    // in micrometer
            unsigned int left;      // in micrometer
            unsigned int right;     // in micrometer
        } margin;
        
        // A4: 210 x 297; NE: 231 x 303
        PageDimension() : width(210000), height(297000) {margin.top = margin.bottom = 15000; margin.left = margin.right = 10000;};
    };
    
    // score object within the document
    class Score
    {
     public:
        unsigned int start_page;    // document-page number of the first score-page
        ScorePress::Score score;    // score object
        
        Score(unsigned int _page) : start_page(_page) {};
    };
    
    // attachable object on the page
    class Attached
    {
     public:
        unsigned int page;  // page which the object is attached to
        Movable* object;    // attached object
        
        Attached(unsigned int _page, Movable* _object) : page(_page), object(_object) {};
    };
    
    // list typedefs
    typedef std::list<Attached> AttachedList;
    typedef std::list<Score>    ScoreList;
    
 private:
    AttachedList   attached;        // objects attached to the document
    
 public:
    PageDimension  page_layout;     // page layout
    ScoreList      scores;          // scores within the document
    DocumentMeta   meta;            // meta information
    
    // on-page object parameters
    unsigned int head_height;       // default head-height (in micrometer)
    unsigned int stem_width;        // default stem-width  (in micrometer)
    
 public:
    // attached object interface
    void add_attached(Movable* object, unsigned int page);  // adds attachable (ownership transferred to this instance)
    void remove_attached(std::list<Attached>::iterator it); // removes attached object (this invalidates the iterator)
    const std::list<Attached>& get_attached() const;        // returns an iterator for the attached objects
    
    // destructor (deleting attached objects)
    virtual ~Document();
};

inline const std::list<Document::Attached>& Document::get_attached() const {return attached;}

} // end namespace

#endif


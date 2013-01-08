
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2011 Dominik Lehmann
  
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

#ifndef SCOREPRESS_PAGESET_HH
#define SCOREPRESS_PAGESET_HH

#include <list>            // std::list

#include "plate.hh"        // Plate
#include "parameters.hh"    // ViewportParam

namespace ScorePress
{
//  CLASSES
// ---------
class PageSet;    // set of plates for all pages of a document


//
//     class PageSet
//    ===============
//
// A set of plates used to render one document,
// subdivided by sets of plates for each page.
//
class PageSet
{
 public:
    // dimension of the page
    class PageDimension
    {
     public:
        mpx_t width;
        mpx_t height;
        struct
        {
            mpx_t top;
            mpx_t bottom;
            mpx_t left;
            mpx_t right;
        } margin;
        
        // constructors
        PageDimension() : width(0), height(0) {margin.top = margin.bottom = margin.left = margin.right = 0;};
        PageDimension(const Document::PageDimension& dim, const ViewportParam& viewport) {set(dim, viewport);};
        
        // set data from page-dimension in micrometer
        void set(const Document::PageDimension& dim, const ViewportParam& viewport);
    };
    
    // dimension of the on-page score-object
    class ScoreDimension
    {
     public:
        Position<mpx_t> position;
        mpx_t width;
        mpx_t height;
    
     public:
        ScoreDimension() : width(0), height(0) {};
        ScoreDimension(mpx_t x, mpx_t y, mpx_t w, mpx_t h) : position(x, y), width(w), height(h) {};
        bool contains(const Position<mpx_t>& pos) const; // check, if the score-object contains a given point
    };
    
    // plate with information for the press on how to render it
    class PlateInfo
    {
     public:
        unsigned int pageno;                // pagenumber relative to the score-object's beginning
        const Score* score;                 // pointer to the score object
        const ScoreDimension dimension;     // score dimension
        Plate plate;                        // plate object for the given page of the given score
        
        PlateInfo(const unsigned int _pno, const Score& _score, const ScoreDimension& _dim);    // constructor
    };
    
    // all rendering information for one page
    class pPage
    {
     public:
        std::list<PlateInfo> plates;                // plates for each visible score-object
        std::list<Plate::pAttachable> attachables;  // independent movable objects on the page
        
        // find a plate belonging to a given score on this page
        std::list<PlateInfo>::iterator get_plate_by_score(const Score& score);
        
        // find a plate containing the given graphical coordinate (relative to the page)
        std::list<PlateInfo>::iterator get_plate_by_pos(const Position<>& pos);
    };
    
    // page layout
    PageDimension page_layout;
    
    // on-page object parameters
    mpx_t head_height;  // default head-height
    mpx_t stem_width;   // default stem-width
    
    // list of all pages within the document
    std::list<pPage> pages;
    inline void erase() {pages.clear();}
    
    // remove plates of the given score from all pages 
    void erase_score(const Score& score);
    
    // get the page with the given index (creating non-existing pages)
    std::list<pPage>::iterator get_page(unsigned int pageno);
    
    // remove empty pages from the end of the pageset
    void remove_empty_pages();
};

} // end namespace

#endif


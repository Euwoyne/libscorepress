
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

#include "pageset.hh"
using namespace ScorePress;


//
//     class PageSet
//    ===============
//
// A set of plates used to render one document,
// subdivided by sets of plates for each page.
//

// set data from page-dimension in micrometer
void PageSet::PageDimension::set(const Document::PageDimension& dim, const ViewportParam& viewport)
{
    width = viewport.umtopx_h(dim.width);
    height = viewport.umtopx_v(dim.height);
    margin.top = viewport.umtopx_v(dim.margin.top);
    margin.bottom = viewport.umtopx_v(dim.margin.bottom);
    margin.left = viewport.umtopx_h(dim.margin.left);
    margin.right = viewport.umtopx_h(dim.margin.right);
}

// check, if the score-object contains a given point
bool PageSet::ScoreDimension::contains(const Position<mpx_t>& pos) const
{
    return (pos.x >= position.x && pos.y >= position.y && pos.x < position.x + width && pos.y < position.y + height);
}

// plate-info constructor
PageSet::PlateInfo::PlateInfo(const unsigned int _pageno, const Score& _score, const ScoreDimension& _dim)
    : pageno(_pageno), score(&_score), dimension(_dim) {}

// find a plate belonging to a given score on this page
std::list<PageSet::PlateInfo>::iterator PageSet::pPage::get_plate_by_score(const Score& score)
{
    for (std::list<PlateInfo>::iterator i = plates.begin(); i != plates.end(); ++i) // check each plate
    {
        if (i->score == &score) return i;   // if it refers to the given score, return
    };
    return plates.end();    // if it cannot be found, return invalid iterator
}

// find a plate containing the given graphical coordinate (relative to the page)
std::list<PageSet::PlateInfo>::iterator PageSet::pPage::get_plate_by_pos(const Position<>& pos)
{
    for (std::list<PlateInfo>::iterator i = plates.begin(); i != plates.end(); ++i) // check each plate
    {
        if (i->dimension.position.x <= pos.x && // if the given position is within the score object
            i->dimension.position.x + static_cast<int>(i->dimension.width) >= pos.x &&
            i->dimension.position.y <= pos.y &&
            i->dimension.position.y + static_cast<int>(i->dimension.height) >= pos.y)
            return i;       // return the iterator
    };
    return plates.end();    // if it cannot be found, return invalid iterator
}

// remove plates of the given score from all pages 
void PageSet::erase_score(const Score& score)
{
    std::list<PageSet::PlateInfo>::iterator info;   // plate to be removed
    for (std::list<pPage>::iterator i = pages.begin(); i != pages.end(); ++i)   // for each page
    {
        info = i->get_plate_by_score(score);                // get the plate for the given score
        if (info != i->plates.end()) i->plates.erase(info); // if it exists, remove the plate
    };
}

// get the page with the given index (creating non-existing pages)
std::list<PageSet::pPage>::iterator PageSet::get_page(unsigned int pageno)
{
    unsigned int idx = 0;   // page index
    for (std::list<pPage>::iterator i = pages.begin(); i != pages.end(); ++i)   // iterate through pages
    {
        if (idx == pageno) return i;    // if we have got the correct page, return
        idx++;                          // count the page
    }
    do pages.push_back(pPage()); while (idx++ < pageno);    // append enough pages to be able to return requested page
    return --pages.end();                                   // return page
}

// remove empty pages from the end of the pageset
void PageSet::remove_empty_pages()
{
    while (!pages.empty() && pages.back().plates.empty() && pages.back().attachables.empty())
    {
        pages.pop_back();
    };
}


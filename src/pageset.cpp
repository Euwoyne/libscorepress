
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

#include "pageset.hh"
using namespace ScorePress;


//
//     class Pageset
//    ===============
//
// A set of plates used to render one document,
// subdivided by sets of plates for each page.
//

// set data from page-dimension in micrometer
void Pageset::PageDimension::set(const Document::PageDimension& dim, const ViewportParam& viewport)
{
    width = viewport.umtopx_h(dim.width);
    height = viewport.umtopx_v(dim.height);
    margin.top = viewport.umtopx_v(dim.margin.top);
    margin.bottom = viewport.umtopx_v(dim.margin.bottom);
    margin.left = viewport.umtopx_h(dim.margin.left);
    margin.right = viewport.umtopx_h(dim.margin.right);
}

// check, if the score-object contains a given point
bool Pageset::ScoreDimension::contains(const Position<mpx_t>& pos) const
{
    return (pos.x >= position.x && pos.y >= position.y && pos.x < position.x + width && pos.y < position.y + height);
}

// plate-info constructor
Pageset::PlateInfo::PlateInfo(const size_t _pageno, const size_t _start, const Score& _score, const ScoreDimension& _dim)
    : pageno(_pageno), start_page(_start), score(&_score), dimension(_dim), plate(new Plate()) {}

// find a plate belonging to a given score on this page
Pageset::pPage::Iterator Pageset::pPage::get_plate_by_score(const Score& score)
{
    for (Iterator i = plates.begin(); i != plates.end(); ++i)   // check each plate
    {
        if (i->score == &score) return i;   // if it refers to the given score, return
    };
    return plates.end();    // if it cannot be found, return invalid iterator
}

Pageset::pPage::const_Iterator Pageset::pPage::get_plate_by_score(const Score& score) const
{
    for (const_Iterator i = plates.begin(); i != plates.end(); ++i)   // check each plate
    {
        if (i->score == &score) return i;   // if it refers to the given score, return
    };
    return plates.end();    // if it cannot be found, return invalid iterator
}

// find a plate containing the given graphical coordinate (relative to the page)
Pageset::pPage::Iterator Pageset::pPage::get_plate_by_pos(const Position<mpx_t>& pos)
{
    for (Iterator i = plates.begin(); i != plates.end(); ++i)   // check each plate
    {
        if (i->dimension.contains(pos)) // if the given position is within the score object
            return i;                   // return the iterator
    };
    return plates.end();    // if it cannot be found, return invalid iterator
}

Pageset::pPage::const_Iterator Pageset::pPage::get_plate_by_pos(const Position<mpx_t>& pos) const
{
    for (const_Iterator i = plates.begin(); i != plates.end(); ++i)   // check each plate
    {
        if (i->dimension.contains(pos)) // if the given position is within the score object
            return i;                   // return the iterator
    };
    return plates.end();    // if it cannot be found, return invalid iterator
}

// remove plates of the given score from all pages 
void Pageset::erase(const Score& score)
{
    Pageset::pPage::Iterator info;      // plate to be removed
    for (Iterator i = pages.begin(); i != pages.end(); ++i)    // for each page
    {
        info = i->get_plate_by_score(score);                // get the plate for the given score
        if (info != i->plates.end()) i->plates.erase(info); // if it exists, remove the plate
    };
    remove_empty_pages();               // remove pages left empty
}

// append a new page to the list (and return iterator)
Pageset::Iterator Pageset::add_page()
{
    if (pages.empty())
    {
        pages.push_back(pPage(0));
        return pages.begin();
    };
    pages.push_back(pPage(pages.back().pageno + 1));
    return --pages.end();
}

// get the page with the given index (creates non-existing pages)
Pageset::Iterator Pageset::get_page(size_t pageno)
{
    unsigned int idx = 0;   // page index
    for (Iterator i = pages.begin(); i != pages.end(); ++i) // iterate through pages
    {
        if (i->pageno == pageno) return i;  // if we have got the correct page, return
        ++idx;                              // count the page
    };
    do pages.push_back(pPage(idx)); while (++idx < pageno); // append enough pages to be able to return requested page
    return --pages.end();                                   // return page
}

// get the page with the given index (on not existing page, returns pages.end())
Pageset::const_Iterator Pageset::get_page(size_t pageno) const
{
    for (const_Iterator i = pages.begin(); i != pages.end(); ++i)   // iterate through pages
        if (i->pageno == pageno) return i;                          // if we have got the correct page, return
    return pages.end();                                             // otherwise, return invalid iterator
}

// get the first page with the fiven score object
Pageset::Iterator Pageset::get_first_page(const Score& score)
{
    pPage::Iterator plate;
    for (Iterator i = pages.begin(); i != pages.end(); ++i) // iterate through pages
    {
        plate = i->get_plate_by_score(score);   // look for the score
        if (plate != i->plates.end())
            return i;
    };
    return pages.end();
}

// remove empty pages from the end of the pageset
void Pageset::remove_empty_pages()
{
    while (!pages.empty() && pages.back().plates.empty() && pages.back().attached.empty())
    {
        pages.pop_back();
    };
}


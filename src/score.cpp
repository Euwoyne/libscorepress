
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

#include "score.hh"     // Staff, Score, List, [score classes]
using namespace ScorePress;

// staff default constructor
Staff::Staff() : offset_y(0), head_height(10), line_count(5),
                 long_barlines(false), curlybrace(false), bracket(false),
                 brace_pos(1000), bracket_pos(1000),
                 style(NULL) {}

// get an iterator for a given staff
std::list<Staff>::const_iterator Score::get_staff(const Staff& staff) const throw(StaffNotFound)
{
    std::list<Staff>::const_iterator out = staves.begin();  // initialize iterator
    while (out != staves.end() && &*out != &staff) ++out;   // iterate until we find the staff
    if (out == staves.end()) throw StaffNotFound();         // if we did not find the given staff, throw exception
    return out;                                             // return staff-iterator
}

// check, whether the given staves belong to one instrument
bool Score::same_instrument(const Staff& staff1, const Staff& staff2) const throw(StaffNotFound)
{
    char state = 0; // algorithm state (0: wait for one of the given staves
                    //                  1: got "staff1", wait for second one
                    //                  2: got "staff2", wait for first one)
    // iterate all staves
    for (std::list<Staff>::const_iterator i = staves.begin(); i != staves.end(); ++i)
    {
        if (state == 0)         // wait for one of the given staves
        {
            if      (&*i == &staff1) state = 1; // if we got "staff1", goto state 1
            else if (&*i == &staff2) state = 2; // if we got "staff2", goto state 2
        };
        
        if (state == 1)         // if we got "staff1"
        {
            if (&*i == &staff2) return true;    // and we find the second one, they are in one group
            if (!i->curlybrace) return false;   // if the group ends, they are in different groups
        }
        else if (state == 2)    // if we got "staff2"
        {
            if (&*i == &staff1) return true;    // and we find the first one, they are in one group
            if (!i->curlybrace) return false;   // if the group ends, they are in different groups
        };
    };
    
    if (state == 0) throw StaffNotFound();  // if we did not find any of the given staves, throw exception
    return false;                           // otherwise just return "false"
}

// check, whether the given staves belong to one instrument-group
bool Score::same_group(const Staff& staff1, const Staff& staff2) const throw(StaffNotFound)
{
    char state = 0; // algorithm state (0: wait for one of the given staves
                    //                  1: got "staff1", wait for second one
                    //                  2: got "staff2", wait for first one)
    // iterate all staves
    for (std::list<Staff>::const_iterator i = staves.begin(); i != staves.end(); ++i)
    {
        if (state == 0)         // wait for one of the given staves
        {
            if      (&*i == &staff1) state = 1; // if we got "staff1", goto state 1
            else if (&*i == &staff2) state = 2; // if we got "staff2", goto state 2
        };
        
        if (state == 1)         // if we got "staff1"
        {
            if (&*i == &staff2) return true;    // and we find the second one, they are in one group
            if (!i->bracket) return false;      // if the group ends, they are in different groups
        }
        else if (state == 2)    // if we got "staff2"
        {
            if (&*i == &staff1) return true;    // and we find the first one, they are in one group
            if (!i->bracket) return false;      // if the group ends, they are in different groups
        };
    };
    
    if (state == 0) throw StaffNotFound();  // if we did not find any of the given staves, throw exception
    return false;                           // otherwise just return "false"
}

// inserts the given attachable into the list, ordering by page
void Document::add_attached(Movable* object, unsigned int page)
{
    for (std::list<Attached>::iterator i = attached.begin(); i != attached.end(); ++i)  // iterate all objects
    {
        if (i->page > page) // if we got the first object on the next page
        {
            attached.insert(i, Attached(page, object)); // insert as last object on the page
            return;                                     // and return
        };
    };
    attached.push_back(Attached(page, object)); // if we never found the given page, just append the object
}

// removes the attached object pointed to by the iterator (this invalidates the iterator)
void Document::remove_attached(std::list<Attached>::iterator it)
{
    delete it->object;
    attached.erase(it);
}

// destructor (deleting attached objects)
Document::~Document()
{
    for (std::list<Attached>::iterator i = attached.begin(); i != attached.end(); ++i)
    {
        delete i->object;
    };
}



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

#ifndef SCOREPRESS_META_HH
#define SCOREPRESS_META_HH

#include <string>   // std::string
#include <vector>   // std::vector
#include <map>      // std::map
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
struct SCOREPRESS_API Meta;            // structure of meta information for a score
struct SCOREPRESS_API DocumentMeta;    // structure of meta information for a document


//
//     struct Meta
//    =============
//
// Structure of meta information for a score.
//
struct SCOREPRESS_API Meta
{
    typedef std::string                  Field;
    typedef std::vector<Field>           List;
    typedef std::map<std::string, Field> Map;
    
    Field title;        // title
    Field subtitle;     // subtitle
    Field artist;       // artist
    Field key;          // main key
    Field date;         // date of completion
    Field number;       // number in collection
    Map   misc;         // additional metadata
    
    inline Meta() : title("Untitled") {};
};

//
//     struct DocumentMeta
//    =====================
//
// Structure of meta information for a document.
//
struct SCOREPRESS_API DocumentMeta : public Meta
{
    Field transcriptor;             // transcriptor (empty, if not a transcription)
    List  instrumentation;          // instrumentation
    List  original_instrumentation; // original instrumentation
    Field opus;                     // opus
};


} // end namespace

#endif

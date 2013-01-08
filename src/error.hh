
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2012 Dominik Lehmann
  
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

#ifndef SCOREPRESS_ERROR_HH
#define SCOREPRESS_ERROR_HH

#include <string>   // std::string

namespace ScorePress
{
//  CLASSES
// ---------
class Error;    // exception base class, containing an error message


//
//     class Error
//    =============
//
// The base class for all exceptions thrown by this library.
// It inherits the implementation of "std::string" to contain a human-
// readable error message.
//
class Error : public std::string
{
 public:
    Error() : std::string("Unknown Error within module \"libscorepress\"") {};
    Error(const std::string msg) : std::string(msg) {};
};

}

#endif



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

#ifndef SCOREPRESS_ERROR_HH
#define SCOREPRESS_ERROR_HH

#include <string>   // std::string
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API Error;    // exception base class, containing an error message


//
//     class Error
//    =============
//
// The base class for all exceptions thrown by this library.
// It inherits the implementation of "std::string" to contain a human-
// readable error message.
//
class SCOREPRESS_API Error : public std::string
{
 public:
    Error();                        // default constructor (generic message)
    Error(const std::string msg);   // constructor with specific message
};

//
//     class MissingDefaultConstructor
//    =================================
//
// This exception shall be thrown from the default constructor of classes, that
// do not wish to implement a default constructor.
// (NOTE: Python bindings require a default constructor to be present).
//
class SCOREPRESS_API MissingDefaultConstructor : public Error
{
 public:
    MissingDefaultConstructor(const std::string classname = "???");
};

}

#endif


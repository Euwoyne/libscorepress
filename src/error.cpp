
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

#include "error.hh" // Error

using namespace ScorePress;

Error::Error() : std::string("Unknown Error within module \"libscorepress\"") {}
Error::Error(const std::string msg) : std::string(msg) {}

MissingDefaultConstructor::MissingDefaultConstructor(const std::string classname) : Error("Class ")
{
    append(classname); append(" has no default constructor!");
}


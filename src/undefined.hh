
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

#ifndef SCOREPRESS_UNDEFINED_HH
#define SCOREPRESS_UNDEFINED_HH

// define "UNDEFINED" macro, resolving to the largest value "size_t" can contain.
// this number is interpreted as an undefined value
#define UNDEFINED (static_cast<size_t>(-1))

// ANNOTATION: This header file should not be included by any header file,
//             to avoid exporting macros and thus exporting symbols outside
//             the "ScorePress" namespace.

#endif


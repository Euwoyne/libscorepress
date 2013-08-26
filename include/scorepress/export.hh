
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

#ifndef SCOREPRESS_EXPORT_HH
#define SCOREPRESS_EXPORT_HH

#if defined _WIN32 || defined __CYGWIN__
    #define _SCOREPRESS_IMPORT __declspec(dllimport)
    #define _SCOREPRESS_EXPORT __declspec(dllexport)
    #define _SCOREPRESS_LOCAL
#elif __GNUC__ >= 4
    #define _SCOREPRESS_IMPORT __attribute__ ((visibility ("default")))
    #define _SCOREPRESS_EXPORT __attribute__ ((visibility ("default")))
    #define _SCOREPRESS_LOCAL  __attribute__ ((visibility ("hidden")))
#else
    #define _SCOREPRESS_IMPORT
    #define _SCOREPRESS_EXPORT
    #define _SCOREPRESS_LOCAL
#endif

#ifdef SCOREPRESS_SO            // defined if compiled as a shared library
  #ifdef SCOREPRESS_EXPORTS     //     defined during library compilation (as opposed to usage)
    #define SCOREPRESS_API _SCOREPRESS_EXPORT
  #else                         //     defined if the library is used
    #define SCOREPRESS_API _SCOREPRESS_IMPORT
  #endif
  #define SCOREPRESS_LOCAL _SCOREPRESS_LOCAL
#else                           // compiled as static library
  #define SCOREPRESS_API
  #define SCOREPRESS_LOCAL
#endif

#endif


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

#include "config.hh"

#ifndef PREFIX
#   define PREFIX     "/usr/local"
#endif
#ifndef LIBDIR 
#   define LIBDIR     PREFIX "/lib"
#endif
#ifndef INCLUDEDIR
#   define INCLUDEDIR PREFIX "/include/" LIBSCOREPRESS_PACKAGE_NAME "-" LIBSCOREPRESS_VERSION_STRING
#endif
#ifndef DATADIR
#   define DATADIR    PREFIX "/share/libscorepress"
#endif
#ifndef DESTDIR
#   define DESTDIR    ""
#endif

const struct ScorePress_Config scorepress_config =
{
    PREFIX,
    LIBDIR,
    INCLUDEDIR,
    DATADIR,
    DESTDIR
};



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

#include <iostream> // std::cout, std::cerr

#include "log.hh"

using namespace ScorePress;


//
//     class Log
//    ===========
//
// Class providing logging services.
//

// set default flags
Log::Log(bool nolog)
{
    _flags.ECHO_INFO    = (nolog?0:1);  // print information messages
    _flags.ECHO_DEBUG   = 0;            // print debug messages
    _flags.ECHO_VERBOSE = 0;            // print verbose-mode messages
    _flags.ECHO_WARN    = (nolog?0:1);  // print warnings
    _flags.ECHO_ERROR   = (nolog?0:1);  // print errors
    _flags.LOG_INFO     = 0;            // log information messages
    _flags.LOG_DEBUG    = 0;            // log debug messages
    _flags.LOG_VERBOSE  = 0;            // log verbose-mode messages
    _flags.LOG_WARN     = (nolog?0:1);  // log warnings
    _flags.LOG_ERROR    = (nolog?0:1);  // log errors
}

// handle log file
bool Log::open(const char* filename)
{
    file.open(filename, std::ofstream::out | std::ofstream::app);
    if (file.fail())
    {
        file.clear();
        return false;
    };
    return true;
}

// log informational message
void Log::info(const char* msg)
{
    if (_flags.ECHO_INFO)
        std::cout << msg << std::endl;
    if (_flags.LOG_INFO && file.is_open())
        file << msg << std::endl;
}

// log debug message
void Log::debug(const char* msg)
{
    if (_flags.ECHO_DEBUG)
        std::cout << msg << std::endl;
    if (_flags.LOG_DEBUG && file.is_open())
        file << msg << std::endl << std::flush;
}

// log verbose output
void Log::verbose(const char* msg)
{
    if (_flags.ECHO_VERBOSE)
        std::cout << msg << std::endl;
    if (_flags.LOG_VERBOSE && file.is_open())
        file << msg << std::endl;
}

// log warning
void Log::warn(const char* msg)
{
    if (_flags.ECHO_WARN)
        std::cout << "WARNING: " << msg << std::endl;
    if (_flags.LOG_WARN && file.is_open())
        file << "WARNING: " << msg << std::endl;
}

// log error message
void Log::error(const char* msg)
{
    if (_flags.ECHO_ERROR)
        std::cout << "ERROR: " << msg << std::endl;
    if (_flags.LOG_ERROR && file.is_open())
        file << "ERROR: " << msg << std::endl << std::flush;
}

// write message to log file only
void Log::noprint(const char* msg)
{
    if (file.is_open())
        file << msg << std::endl;
}

// destructor (closing log file)
Log::~Log()
{
    if (file.is_open()) file.close();
}


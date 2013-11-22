
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

#ifndef SCOREPRESS_LOG_HH
#define SCOREPRESS_LOG_HH

#include <fstream>      // std::ofstream
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API Log;  // message logging class with memory and output


//
//     class Log
//    ============
//
// Class providing logging services.
//
class SCOREPRESS_API Log
{
 private:
    // behaviour flags
    struct
    {
        unsigned short ECHO_INFO    : 1;    // print information messages
        unsigned short ECHO_DEBUG   : 1;    // print debug messages
        unsigned short ECHO_VERBOSE : 1;    // print verbose-mode messages
        unsigned short ECHO_WARN    : 1;    // print warnings
        unsigned short ECHO_ERROR   : 1;    // print errors
        unsigned short LOG_INFO     : 1;    // log information messages
        unsigned short LOG_DEBUG    : 1;    // log debug messages
        unsigned short LOG_VERBOSE  : 1;    // log verbose-mode messages
        unsigned short LOG_WARN     : 1;    // log warnings
        unsigned short LOG_ERROR    : 1;    // log errors
    } _flags;
    
    // log file
    std::ofstream file;
    
 public:
    // constructor
    Log(bool nolog = false);    // preset flags (default values or no logging at all)
    
    // handle log file
           bool open(const char* filename);
    inline bool is_open()                  {return file.is_open();};
    inline void close()                    {file.close();};
    
    // messaging methods (logging the given message)
    void info(const char* msg);
    void debug(const char* msg);
    void verbose(const char* msg);
    void warn(const char* msg);
    void error(const char* msg);
    void noprint(const char* msg);      // write message to log file only
    
    // flag setting methods
    inline void echo_info(bool v = true)    {_flags.ECHO_INFO    = (v?1:0);};
    inline void echo_debug(bool v = true)   {_flags.ECHO_DEBUG   = (v?1:0);};
    inline void echo_verbose(bool v = true) {_flags.ECHO_VERBOSE = (v?1:0);};
    inline void echo_warn(bool v = true)    {_flags.ECHO_WARN    = (v?1:0);};
    inline void echo_error(bool v = true)   {_flags.ECHO_ERROR   = (v?1:0);};
    
    inline void log_info(bool v = true)     {_flags.LOG_INFO    = (v?1:0);};
    inline void log_debug(bool v = true)    {_flags.LOG_DEBUG   = (v?1:0);};
    inline void log_verbose(bool v = true)  {_flags.LOG_VERBOSE = (v?1:0);};
    inline void log_warn(bool v = true)     {_flags.LOG_WARN    = (v?1:0);};
    inline void log_error(bool v = true)    {_flags.LOG_ERROR   = (v?1:0);};
    
    // destructor (closing log file)
    ~Log();
};

//
//     class Logging
//    ===============
//
// Standard interface for classes using an external log.
//
class SCOREPRESS_API Logging
{
 protected:
    mutable Log* logging_log;
    
 public:
    inline Logging() : logging_log(NULL) {};
    inline void log_set(Log& log)        {logging_log = &log;};
    inline void log_set(Logging& log)    {logging_log = log.logging_log;};
    inline void log_unset()              {logging_log = NULL;};
    
    inline void log_info(const char* msg)    const {if (logging_log) logging_log->info(msg);};
    inline void log_debug(const char* msg)   const {if (logging_log) logging_log->debug(msg);};
    inline void log_verbose(const char* msg) const {if (logging_log) logging_log->verbose(msg);};
    inline void log_warn(const char* msg)    const {if (logging_log) logging_log->warn(msg);};
    inline void log_error(const char* msg)   const {if (logging_log) logging_log->error(msg);};
    inline void log_noprint(const char* msg) const {if (logging_log) logging_log->noprint(msg);};
};

} // end namespace

#endif


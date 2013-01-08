
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

#ifndef SCOREPRESS_LOG_HH
#define SCOREPRESS_LOG_HH

#include <list>   // std::list

namespace ScorePress
{
//  CLASSES
// ---------
class Log;  // message logging class with memory and output


//
//     class Log
//    ============
//
// Singleton class providing logging services.
// All messages will be saved internally, and optionally printed to
// the standard output stream.
//
class Log
{
 public:
    // the message structure
    struct Message
    {
        enum enuType {INFO, DEBUG, VERBOSE, WARN, ERROR};
        
        enuType type;   // type of the message
        char* msg;      // message content
    };
    
 private:
    // behaviour flags
    struct
    {
        unsigned short ECHO_INFO    : 1;    // print information messages
        unsigned short ECHO_DEBUG   : 1;    // print debug messages
        unsigned short ECHO_VERBOSE : 1;    // print verbose-mode messages
        unsigned short ECHO_WARN    : 1;    // print warnings
        unsigned short ECHO_ERROR   : 1;    // print errors
        unsigned short LOG_INFO     : 1;    // save information messages
        unsigned short LOG_DEBUG    : 1;    // save debug messages
        unsigned short LOG_VERBOSE  : 1;    // save verbose-mode messages
        unsigned short LOG_WARN     : 1;    // save warnings
        unsigned short LOG_ERROR    : 1;    // save errors
    } _flags;
    
    // instance variables
    std::list<Message> _msgs;     // logged messages
    void (*_agent)(const Message&); // logging agent
    
    // singleton instance (created as static object of this method)
    static Log& _instance();
    
    // private methods
    void _push(const char* msg, const Message::enuType type);   // push the given message onto the vector
    
    // private constructors to prevent instanciation
    Log();
    Log(Log const& log);
    Log& operator = (Log const& log);
    
 public:
    // messaging methods (logging the given message)
    inline static void info(const char* msg)    {_instance()._push(msg, Message::INFO);};
    inline static void debug(const char* msg)   {_instance()._push(msg, Message::DEBUG);};
    inline static void verbose(const char* msg) {_instance()._push(msg, Message::VERBOSE);};
    inline static void warn(const char* msg)    {_instance()._push(msg, Message::WARN);};
    inline static void error(const char* msg)   {_instance()._push(msg, Message::ERROR);};
    
    // log access
    inline static const std::list<Message>& messages() {return _instance()._msgs;};
    inline static void clear() {_instance()._msgs.clear();};
    
    // flag setting methods
    inline static void echo_info(bool v)    {_instance()._flags.ECHO_INFO    = (v?1:0);};
    inline static void echo_debug(bool v)   {_instance()._flags.ECHO_DEBUG   = (v?1:0);};
    inline static void echo_verbose(bool v) {_instance()._flags.ECHO_VERBOSE = (v?1:0);};
    inline static void echo_warn(bool v)    {_instance()._flags.ECHO_WARN    = (v?1:0);};
    inline static void echo_error(bool v)   {_instance()._flags.ECHO_ERROR   = (v?1:0);};
    
    inline static void log_info(bool v)     {_instance()._flags.LOG_INFO    = (v?1:0);};
    inline static void log_debug(bool v)    {_instance()._flags.LOG_DEBUG   = (v?1:0);};
    inline static void log_verbose(bool v)  {_instance()._flags.LOG_VERBOSE = (v?1:0);};
    inline static void log_warn(bool v)     {_instance()._flags.LOG_WARN    = (v?1:0);};
    inline static void log_error(bool v)    {_instance()._flags.LOG_ERROR   = (v?1:0);};
    
    // logging agent setup
    inline static void setup_agent(void (*agent)(const Message&)) {_instance()._agent = agent;};
    
    // destructor (deleting all logged messages)
    ~Log();
};

} // end namespace

#endif


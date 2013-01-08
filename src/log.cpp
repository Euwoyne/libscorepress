
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

#include <cstring>  // memcpy, strlen
#include <iostream> // std::cout, std::cerr

#include "log.hh"   // Log, std::vector
using namespace ScorePress;


//
//     class Log
//    ===========
//
// Singleton class providing logging services.
// All messages will be saved internally, and optionally printed to
// the standard output stream.
//

// set default flags
Log::Log() : _msgs(), _agent(NULL)
{
    _flags.ECHO_INFO    = 1;    // print information messages
    _flags.ECHO_DEBUG   = 0;    // print debug messages
    _flags.ECHO_VERBOSE = 0;    // print verbose-mode messages
    _flags.ECHO_WARN    = 1;    // print warnings
    _flags.ECHO_ERROR   = 1;    // print errors
    _flags.LOG_INFO     = 0;    // save information messages
    _flags.LOG_DEBUG    = 0;    // save debug messages
    _flags.LOG_VERBOSE  = 0;    // save verbose-mode messages
    _flags.LOG_WARN     = 1;    // save warnings
    _flags.LOG_ERROR    = 1;    // save errors
}

// singleton instance (created as static object of this method)
Log& Log::_instance()
{
    static Log instance;    // create singleton instance
    return instance;        // return instance
}

// push the given message onto the vector
void Log::_push(const char* msg, const Message::enuType type)
{
    // message output
    switch (type)    // check for message type
    {
    case Message::INFO:    if (_flags.ECHO_INFO) std::cout << msg << "\n";  // echo if requested
                           if ((_flags.LOG_INFO) == 0) return;              // abort if log not requested
                           break;
    case Message::DEBUG:   if (_flags.ECHO_DEBUG) std::cout << msg << "\n"; // echo if requested
                           if ((_flags.LOG_DEBUG) == 0) return;             // abort if log not requested
                           break;
    case Message::VERBOSE: if (_flags.ECHO_VERBOSE) std::cout << msg << "\n";   // echo if requested
                           if ((_flags.LOG_VERBOSE) == 0) return;           // abort if log not requested
                           break;
    case Message::WARN:    if (_flags.ECHO_WARN) std::cout << "WARNING: " << msg << "\n";   // echo if requested
                           if ((_flags.LOG_WARN) == 0) return;              // abort if log not requested
                           break;
    case Message::ERROR:   if (_flags.ECHO_ERROR) std::cerr << "ERROR: " << msg << "\n";    // echo if requested
                           if ((_flags.LOG_ERROR) == 0) return;             // abort if log not requested
                           break;
    };
    
    // log the message
    _msgs.push_back(Message());                     // push new message object
    _msgs.back().type = type;                       // set message type
    _msgs.back().msg = new char[strlen(msg) + 1];   // allocate memory for message
    memcpy(_msgs.back().msg, msg, strlen(msg) + 1); // copy message
    
    // call logging agent
    if (_agent) (*_agent)(_msgs.back());
}

// destructor (deleting all logged messages)
Log::~Log()
{
    if (_flags.ECHO_DEBUG || _flags.ECHO_VERBOSE)
        std::cout << "killed Log instance\n";
    for (std::list<Message>::iterator i = _msgs.begin(); i != _msgs.end(); i++)
    {   // delete every message in the log-vector
        delete[] i->msg;
    };
}


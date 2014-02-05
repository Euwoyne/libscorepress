
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

#ifndef SCOREPRESS_FILEWRITER_HH
#define SCOREPRESS_FILEWRITER_HH

#include <string>        // std::string
#include <vector>        // std::vector
#include "document.hh"   // Document
#include "parameters.hh" // EngraverParam, PressParam, StyleParam, InterfaceParam
#include "error.hh"      // Error
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API FileWriter;        // abstract base-class for file-parsers (writer interface)
class SCOREPRESS_API DocumentWriter;    // abstract base-class for document-parsers (writer interface)


//
//     class FileWriter
//    ==================
//
// This class defines the abstract interface of writers for any supported file-
// type. It predefines meta information about what files the implementation is
// able to write (such as MIME type and preferred file-extensions).
// The actual writing method is declared in different child-classes, since its
// signature depends on the source structure.
//
class SCOREPRESS_API FileWriter
{
 public:
    // exception classes
    class SCOREPRESS_API Error : public ScorePress::Error
        {public: Error(const std::string& msg) : ScorePress::Error(msg) {};};
    class SCOREPRESS_API IOException : public Error     // thrown, if the file cannot be read
        {public: IOException(const std::string& msg) : Error(msg) {};};
    class SCOREPRESS_API FormatError : public Error     // thrown, if the file contains illegal syntax
        {public: FormatError(const std::string& msg) : Error(msg) {};};
    
 public:
    const std::string        name;              // file-type name
    std::vector<std::string> mime_types;        // file mime-type
    std::vector<std::string> file_extensions;   // extension filter
    
 public:
    // constructor
    FileWriter(const std::string& name);
    FileWriter(const std::string& name, const std::string& mime_type, const std::string& file_extension);
    
    // type information access
    const std::string&             get_name() const;
    const std::vector<std::string> get_mime_types() const;
    const std::vector<std::string> get_file_extensions() const;
    
    void add_mime_type(const std::string mime);             // add mime-type
    void add_file_extension(const std::string extension);   // add file-extension
    
    // virtual writer interface
    virtual void open(const char* filename) = 0;    // open file for writing
    virtual void close() = 0;                       // close file
    
    virtual bool is_open() const = 0;               // check if a file is opened
    virtual const char* get_filename() const = 0;   // return the filename (or NULL)
};

// inline method implementations
inline FileWriter::FileWriter(const std::string& _name) : name(_name) {}
inline FileWriter::FileWriter(const std::string& _name, const std::string& mime_type, const std::string& file_extension)
            : name(_name), mime_types(1, mime_type), file_extensions(1, file_extension) {}

inline const std::string&             FileWriter::get_name()            const {return name;}
inline const std::vector<std::string> FileWriter::get_mime_types()      const {return mime_types;}
inline const std::vector<std::string> FileWriter::get_file_extensions() const {return file_extensions;}

inline void FileWriter::add_mime_type(const std::string mime)           {mime_types.push_back(mime);}
inline void FileWriter::add_file_extension(const std::string extension) {file_extensions.push_back(extension);}


//
//     class DocumentWriter
//    ======================
//
// The DocumentWriter adds a writer-function for Documents to the FileWriter
// interface.
//
class SCOREPRESS_API DocumentWriter : public FileWriter
{
 public:
    // constructor
    DocumentWriter(const std::string& name);
    DocumentWriter(const std::string& name, const std::string& mime_type, const std::string& file_extension);
    
    // virtual writer interface
    virtual void open(const char* filename) = 0;                // open file for reading
    virtual void close() = 0;                                   // close file
    
    virtual bool is_open() const = 0;                           // check if a file is opened
    virtual const char* get_filename() const = 0;               // return the filename (or NULL)
    
    virtual void write_document(const Document& source) = 0;    // document writer
};

inline DocumentWriter::DocumentWriter(const std::string& _name) : FileWriter(_name) {}
inline DocumentWriter::DocumentWriter(const std::string& _name, const std::string& mime_type, const std::string& file_extension)
            : FileWriter(_name, mime_type, file_extension) {}


//
//     class ParameterWriter
//    =======================
//
// The ParameterWriter adds a writer-function for all parameter structures to
// the FileWriter interface.
//
class SCOREPRESS_API ParameterWriter : public FileWriter
{
 public:
    // constructor
    ParameterWriter(const std::string& name);
    ParameterWriter(const std::string& name, const std::string& mime_type, const std::string& file_extension);
    
    // virtual writer interface
    virtual void open(const char* filename) = 0;    // open file for writing
    virtual void close() = 0;                       // close file
    
    virtual bool is_open() const = 0;               // check if a file is opened
    virtual const char* get_filename() const = 0;   // return the filename (or NULL)
    
    virtual void write_parameter(const EngraverParam&  engraver_param,
                                 const PressParam&     press_param,
                                 const StyleParam&     style_param,
                                 const InterfaceParam& interface_param) = 0;
};

inline ParameterWriter::ParameterWriter(const std::string& _name) : FileWriter(_name) {}
inline ParameterWriter::ParameterWriter(const std::string& _name, const std::string& mime_type, const std::string& file_extension)
            : FileWriter(_name, mime_type, file_extension) {}
}

#endif


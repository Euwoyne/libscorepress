
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

#ifndef SCOREPRESS_FILEREADER_HH
#define SCOREPRESS_FILEREADER_HH

#include <string>        // std::string
#include <vector>        // std::vector
#include "score.hh"      // Document
#include "parameters.hh" // EngraverParam, PressParam, StyleParam, InterfaceParam
#include "sprites.hh"    // SpriteSet
#include "error.hh"      // Error
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API FileReader;        // abstract base-class for file-parsers (reader interface)
class SCOREPRESS_API DocumentReader;    // abstract base-class for document-parsers
class SCOREPRESS_API ParameterReader;   // abstract base-class for parameter-parsers
class SCOREPRESS_API SpritesetReader;   // abstract base-class for spriteset-description-parsers


//
//     class FileReader
//    ==================
//
// This class defines the abstract interface of parsers for any supported file-
// type. It predefines meta information about what files the implementation is
// able to parse (such as MIME type and preferred file-extensions).
// The actual parser method is declared in different child-classes, since its
// signature depends on the target structure.
//
class SCOREPRESS_API FileReader
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
    FileReader(const std::string& name);
    FileReader(const std::string& name, const std::string& mime_type, const std::string& file_extension);
    virtual ~FileReader() {};
    
    // type information access
    const std::string&             get_name() const;
    const std::vector<std::string> get_mime_types() const;
    const std::vector<std::string> get_file_extensions() const;
    
    void add_mime_type(const std::string& mime);            // add mime-type
    void add_file_extension(const std::string& extension);  // add file-extension
    
    // virtual parser interface
    virtual void open(const char* data, const std::string& filename) = 0;   // use memory for reading
    virtual void open(const std::string& filename) = 0;     // open file for reading
    virtual void close() = 0;                               // close file
    
    virtual bool is_open() const = 0;                       // check if a file is opened
    virtual const char* get_filename() const = 0;           // return the filename (or NULL)
};

// inline method implementations
inline FileReader::FileReader(const std::string& _name) : name(_name) {}
inline FileReader::FileReader(const std::string& _name, const std::string& mime_type, const std::string& file_extension)
            : name(_name), mime_types(1, mime_type), file_extensions(1, file_extension) {}

inline const std::string&             FileReader::get_name()            const {return name;}
inline const std::vector<std::string> FileReader::get_mime_types()      const {return mime_types;}
inline const std::vector<std::string> FileReader::get_file_extensions() const {return file_extensions;}

inline void FileReader::add_mime_type(const std::string& mime)           {mime_types.push_back(mime);}
inline void FileReader::add_file_extension(const std::string& extension) {file_extensions.push_back(extension);}


//
//     class DocumentReader
//    ======================
//
// The DocumentReader adds a parser-function for Documents to the FileReader
// interface.
//
class SCOREPRESS_API DocumentReader : virtual public FileReader
{
 public:
    // constructor
    DocumentReader(const std::string& name);
    DocumentReader(const std::string& name, const std::string& mime_type, const std::string& file_extension);
    
    // virtual parser interface
    virtual void parse_document(Document& target) = 0;  // document parser
};

inline DocumentReader::DocumentReader(const std::string& _name) : FileReader(_name) {}
inline DocumentReader::DocumentReader(const std::string& _name, const std::string& mime_type, const std::string& file_extension)
            : FileReader(_name, mime_type, file_extension) {}


//
//     class ParameterReader
//    =======================
//
// The ParameterReader adds a parser-function for all parameter structures to
// the FileReader interface.
//
class SCOREPRESS_API ParameterReader : virtual public FileReader
{
 public:
    // constructor
    ParameterReader(const std::string& name);
    ParameterReader(const std::string& name, const std::string& mime_type, const std::string& file_extension);
    
    // virtual parser interface
    virtual void parse_parameter(EngraverParam&  engraver_param,
                                 PressParam&     press_param,
                                 StyleParam&     style_param,
                                 InterfaceParam& interface_param) = 0;
};

inline ParameterReader::ParameterReader(const std::string& _name) : FileReader(_name) {}
inline ParameterReader::ParameterReader(const std::string& _name, const std::string& mime_type, const std::string& file_extension)
            : FileReader(_name, mime_type, file_extension) {}


//
//     class SpritesetReader
//    =======================
//
// The SpritesetReader adds a parser-function for SpriteSets to the FileReader
// interface.
//
class SCOREPRESS_API SpritesetReader : virtual public FileReader
{
 public:
    // constructor
    SpritesetReader(const std::string& name);
    SpritesetReader(const std::string& name, const std::string& mime_type, const std::string& file_extension);
    
    // virtual parser interface
    virtual void parse_spriteset(SpriteSet&   target,       // sprite-set parser
                                 Renderer&    renderer,
                                 const size_t setid) = 0;
};

inline SpritesetReader::SpritesetReader(const std::string& _name) : FileReader(_name) {}
inline SpritesetReader::SpritesetReader(const std::string& _name, const std::string& mime_type, const std::string& file_extension)
            : FileReader(_name, mime_type, file_extension) {}
}
#endif


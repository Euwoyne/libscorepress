
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

#ifndef SCOREPRESS_FILEPARSER_HH
#define SCOREPRESS_FILEPARSER_HH

#include <string>
#include "file_reader.hh"   // FileReader
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
struct               xmlReaderForMemory;    // libxml2 prototype
class SCOREPRESS_API XMLFileReader;         // document-reader implementation for the default XML-format


//
//     class XMLFileReader
//    =====================
//
class SCOREPRESS_API XMLFileReader : public DocumentReader
{
 protected:
    // throwing functions (combining the given data to a error message, which is then thrown)
    void mythrow(const char* trns, const std::string& filename) throw(IOException);
    void mythrow(const char* trns, const std::string& symbol, const std::string& filename, const int line, const int column) throw(FormatError);
    void mythrow(const char* trns, const std::string& filename, const int line, const int column) throw(FormatError);
    
 private:
    xmlReaderForMemory* parser;
    std::string         filename;
    
 public:
    XMLFileReader();                                // constructor
    ~XMLFileReader();                               // destructor
    
    virtual void open(const char* filename);        // open file for reading
    virtual void close();                           // close file
    
    virtual bool is_open() const;                   // check if a file is opened
    virtual const char* get_filename() const;       // return the filename (or NULL)
    
    virtual void parse_document(Document& target);  // document parser
};
}

#endif


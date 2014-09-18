
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2014 Dominik Lehmann
  
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

#ifndef SCOREPRESS_FILEFORMAT_HH
#define SCOREPRESS_FILEFORMAT_HH

#include <string>           // std::string
#include <map>              // std::map
#include "file_reader.hh"   // FileReader
#include "file_writer.hh"   // FileWriter
#include "export.hh"

// libxml2 prototype
struct _xmlTextReader;

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API XMLFileReader;         // document-reader implementation for the default XML-format


//
//     class XMLFileReader
//    =====================
//
class SCOREPRESS_API XMLFileReader : virtual public FileReader
{
 public:
    // error class thrown on syntax errors within the sprites meta information
    class SCOREPRESS_API ExpectedEOF : public FileReader::FormatError
        {public: ExpectedEOF(const std::string& msg) : FileReader::FormatError(msg) {};};
    
 protected:
    // throwing functions (combining the given data to a error message, which is then thrown)
    static void mythrow(const char* trns, const std::string& filename) throw(IOException);
    static void mythrow(const char* trns, const std::string& symbol, const std::string& filename, const int line, const int column) throw(FormatError);
    static void mythrow(const char* trns, const std::string& filename, const int line, const int column) throw(FormatError);
    static void mythrow_eof(const char* trns, const std::string& filename, const int line, const int column) throw(ExpectedEOF);
    
    // reading helper
    void read_int(int& target, const char* tag);
    void read_double(double& target, const char* tag);
    void read_string(std::string& target, const char* tag, bool empty_ok = false);
    void read_i18n(std::map<std::string, std::string>& target, const char* tag, const char* def = "en", bool empty_ok = false);
    void read_names(std::map<std::string, std::string>& target);
    
 protected:
    _xmlTextReader* parser;
    std::string     filename;
    
 public:
    XMLFileReader();
    virtual ~XMLFileReader();
    
    virtual void open(const char* data, const std::string& filename);   // use memory for reading
    virtual void open(const std::string& filename);                     // open file for reading
    virtual void close();                                               // close file
    
    void xopen(_xmlTextReader* reader, const std::string& filename);    // use existing libxml2 text-reader instance
    void xclose();                                                      // reset instance, do not close reader
    
    virtual bool is_open() const;                       // check if a file is opened
    virtual const char* get_filename() const;           // return the filename (or NULL)
};

//
//     class XMLDocumentReader
//    =========================
//
class SCOREPRESS_API XMLDocumentReader : public DocumentReader, public XMLFileReader
{
 public:
    XMLDocumentReader();                                // constructor
    virtual void parse_document(Document& target);      // document parser
};

//
//     class XMLSpritesetReader
//    ==========================
//
class SCOREPRESS_API XMLSpritesetReader : public SpritesetReader, public XMLFileReader
{
 public:
    XMLSpritesetReader();                               // constructor
    virtual void parse_spriteset(SpriteSet&   target,   // sprite-set parser
                                 Renderer&    renderer,
                                 const size_t setid);
};
}
#endif


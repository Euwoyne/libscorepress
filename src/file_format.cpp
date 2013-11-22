
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

#include <libxml/xmlreader.h>   // xmlReaderForFile, ...
#include "file_format.hh"

#define STR_CAST(str)    reinterpret_cast<const xmlChar*>(str)
#define XML_CAST(xmlstr) reinterpret_cast<const char*>(xmlstr)

using namespace ScorePress;


//
//     class XMLFileReader
//    =====================
//
// ___
//

// throwing function (general file error)
void XMLFileReader::mythrow(const char* trns, const std::string& filename) throw(IOException)
{
    char* msg = new char[strlen(trns) + filename.size() + 1];   // allocate memory
    sprintf(msg, trns, filename.c_str());                       // assemble message
    std::string s(msg);                                         // convert to string
    delete[] msg;                                               // delete buffer
    throw IOException(s);                                       // throw message
}

// throwing function (symbol related error)
void XMLFileReader::mythrow(const char* trns, const std::string& symbol, const std::string& filename, const int line, const int column) throw(FormatError)
{
    char* msg = new char[strlen(trns) + symbol.size() + filename.size() +   // allocate memory
                         static_cast<int>(log10(line) + log10(column)) + 3];
    sprintf(msg, trns, symbol.c_str(), filename.c_str(), line, column);     // assemble message
    std::string s(msg);                                 // convert to string
    delete[] msg;                                       // delete buffer
    throw FormatError(s);                               // throw message
}

// throwing function (error with file position)
void XMLFileReader::mythrow(const char* trns, const std::string& filename, const int line, const int column) throw(FormatError)
{
    char* msg = new char[strlen(trns) + filename.size() +   // allocate memory
                         static_cast<int>(log10(line) + log10(column)) + 3];
    sprintf(msg, trns, filename.c_str(), line, column);     // assemble message
    std::string s(msg);                                     // convert to string
    delete[] msg;                                           // delete buffer
    throw FormatError(s);                                   // throw message
}

// default constructor
XMLFileReader::XMLFileReader() : parser(NULL), filename(NULL) {}

// destructor
XMLFileReader::~XMLFileReader()
{
    if (parser)   xmlFreeTextReader(parser);
    if (filename) delete[] filename;
}

void XMLFileReader::open(const char* _filename)
{
    // close previous parser
    if (parser) xmlFreeTextReader(parser);
    
    // create parser
    filename = _filename;
    parser = xmlReaderForFile(_filename, NULL, XML_PARSE_NOBLANKS | XML_PARSE_NOENT | XML_PARSE_NONET);
    
    // report failed parser instanciation
    if (parser == NULL) mythrow("Unable to open file \"%s\"", filename);
}

void XMLFileReader::close()
{
    if (!parser) return;        // do nothing, of no file is open
    xmlFreeTextReader(parser);  // delete parser instance
    parser = NULL;              // erase parser
}
    
bool XMLFileReader::is_open() const
{
    return (parser != NULL);
}

const char* XMLFileReader::get_filename() const
{
    return (parser ? filename.c_str() : NULL);
}

void XMLFileReader::parse_document(Document& target)
{
    // check, if a file is open
    if (!parser)
        throw Error("FileReader has no open file (please call 'XMLFileReader::open' first).")
    
    // strip the filename of its path (for the use in error messages)
    std::string err_file = (filename.rfind('/') != std::string::npos) ?
                    filename.substr(filename.rfind('/') + 1) :
                    filename;
    
    // begin parsing
    try
    {
        // parser state enumerator
        enum enuState
        {   BEGIN, END,
            DOCUMENT,
            META, SPRITESETS, PAGE, ATTACHED, SCORES
        };
        enuState state = BEGIN;     // set current state
        enuState prestate = BEGIN;  // set the previous state (used for returning from a state)
        
        const xmlChar* tag = NULL;  // tag string
        xmlChar* ver = NULL;        // version string
        xmlChar* attr = NULL;       // attribute string
        
        // start finite state machine (running while the parser has got data)
        while ((parser_return = xmlTextReaderRead(parser)) == 1)
        {
            // ignore <!DOCTYPE> and comments
            if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_DOCUMENT_TYPE) continue;
            if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
            
            tag = xmlTextReaderConstName(parser);   // get the current tag
            
            switch (state)  // consider the state
            {
            //  state at the root of the document
            // -----------------------------------
            case BEGIN:
                // expecting <document> to be the root tag
                if (xmlStrEqual(tag, STR_CAST("document")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_ELEMENT) mythrow("Expected tag <document> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // checking "version" attribute
                ver = xmlTextReaderGetAttribute(parser, STR_CAST("version"));
                if (ver == NULL) mythrow("Missing \"version\"-attribute for <document>-tag (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                if (xmlStrEqual(tag, STR_CAST("1.0")) != 1) mythrow("Unsupported version \"%s\" in <document>-tag (in file \"%s\", at line %i:%i)", XML_CAST(ver), err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                free(attr);
                
                // changing state
                state = DOCUMENT;
                break;
                
            //  state after the final </document>, expecting the end of data (should not occur)
            // ----------------------------------
            case END:
                mythrow("Expected EOF (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                break;
                
            //  <document> root section
            // -------------------------
            case DOCUMENT:
                // the document's <meta> section
                if (xmlStrEqual(tag, STR_CAST("meta")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    state = META;   // parse the section
                }
                
                // the <spritesets> section
                else if (xmlStrEqual(tag, STR_CAST("spritesets")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // get "unit" attribute (i.e. head-height)
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("unit"));
                    if (attr == NULL) mythrow("Missing \"unit\"-attribute for <sprites> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.head_height = atoi(XML_CAST(attr));
                    free(attr);
                    
                    // parse the sprite information
                    state = SPRITES;
                }
                
                // end tag </symbols>
                else if (xmlStrEqual(tag, STR_CAST("symbols")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    // the info should end with that
                    state = END;
                }
                
                // any other tag is illegal here
                else mythrow("Expected one of <info>, <sprites> or </symbols> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                break;
    }
    catch (...) // catch any parser errors
    {
        if (parser != NULL) xmlFreeTextReader(parser);  // delete parser instance
        parser = NULL;
        throw;                                          // throw parser error
    };
    
    // parsing successful
    xmlFreeTextReader(parser);  // delete parser instance
}


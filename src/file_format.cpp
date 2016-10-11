
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

#include "file_format.hh"
#include "renderer.hh"          // Renderer

#include <cstdlib>              // atoi, strtof
#include <cstring>              // strlen, sprintf
#include <cmath>                // log10
#include <limits>               // numeric_limits
#include <libxml/xmlreader.h>   // xmlReaderForFile, ...

#define STR_CAST(str)    reinterpret_cast<const xmlChar*>(str)
#define XML_CAST(xmlstr) reinterpret_cast<const char*>(xmlstr)

using namespace ScorePress;


//
//     class XMLFileReader
//    =====================
//
//
//

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
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

// throwing function (expected eof error with file position)
void XMLFileReader::mythrow_eof(const char* trns, const std::string& filename, const int line, const int column) throw(ExpectedEOF)
{
    char* msg = new char[strlen(trns) + filename.size() +   // allocate memory
                         static_cast<int>(log10(line) + log10(column)) + 3];
    sprintf(msg, trns, filename.c_str(), line, column);     // assemble message
    std::string s(msg);                                     // convert to string
    delete[] msg;                                           // delete buffer
    throw ExpectedEOF(s);                                   // throw message
}
#pragma clang diagnostic pop

void XMLFileReader::read_int(int& target, const char* tag)
{
    // check EOF
    if (xmlTextReaderRead(parser) != 1)
        mythrow("Expected </%s> (in file \"%s\", at line %i:%i)", tag, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    
    // read content
    std::string buffer;
    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
    {
        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
        if (xmlTextReaderRead(parser) != 1)
            mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    };
    
    // check end tag
    if (xmlStrEqual(xmlTextReaderConstName(parser), STR_CAST(tag)) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT)
        mythrow("Expected </%s> (in file \"%s\", at line %i:%i)", tag, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    
    // parse the number
    target = atoi(buffer.c_str());
}

void XMLFileReader::read_double(double& target, const char* tag)
{
    // check EOF
    if (xmlTextReaderRead(parser) != 1)
        mythrow("Expected </%s> (in file \"%s\", at line %i:%i)", tag, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    
    // read content
    std::string buffer;
    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
    {
        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
        if (xmlTextReaderRead(parser) != 1)
            mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    };
    
    // check end tag
    if (xmlStrEqual(xmlTextReaderConstName(parser), STR_CAST(tag)) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT)
        mythrow("Expected </%s> (in file \"%s\", at line %i:%i)", tag, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    
    // parse the number
    target = strtof(buffer.c_str(), NULL);
}

void XMLFileReader::read_string(std::string& target, const char* tag, bool empty_ok)
{
    // check EOF
    if (xmlTextReaderRead(parser) != 1)
        mythrow("Expected </%s> (in file \"%s\", at line %i:%i)", tag, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    
    // check empty tag (i.e. <tag/>)
    if (xmlTextReaderIsEmptyElement(parser))
    {
        if (!empty_ok)
            mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
        target.clear();
        return;
    };
    
    // read content
    target.clear();
    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
    {
        target.append(XML_CAST(xmlTextReaderConstValue(parser)));
        if (xmlTextReaderRead(parser) != 1)
            mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    };
    
    // check end tag
    if (xmlStrEqual(xmlTextReaderConstName(parser), STR_CAST(tag)) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT)
        mythrow("Expected </%s> (in file \"%s\", at line %i:%i)", tag, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
}

void XMLFileReader::read_i18n(std::map<std::string, std::string>& target, const char* tag, const char* def, bool empty_ok)
{
    // check language
    xmlChar* attr = xmlTextReaderGetAttribute(parser, STR_CAST("lang"));
    std::string& str = attr ? target[XML_CAST(attr)] : target[def];
    xmlFree(attr);
    
    // check EOF
    if (xmlTextReaderRead(parser) != 1)
        mythrow("Expected </%s> (in file \"%s\", at line %i:%i)", tag, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    
    // check empty tag (i.e. <tag/>)
    if (xmlTextReaderIsEmptyElement(parser))
    {
        if (!empty_ok)
            mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
        str.clear();
        return;
    };
    
    // read content (text node)
    str.clear();
    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
    {
        str.append(XML_CAST(xmlTextReaderConstValue(parser)));
        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    };
    
    // check end tag
    if (xmlStrEqual(xmlTextReaderConstName(parser), STR_CAST(tag)) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT)
        mythrow("Expected </%s> (in file \"%s\", at line %i:%i)", tag, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
}

// constructor
XMLFileReader::XMLFileReader() : FileReader("XMl File", "application/xml", "xml"), parser(NULL) {}

// destructor
XMLFileReader::~XMLFileReader()
{
    if (parser) xmlFreeTextReader(parser);
}

// use memory for reading
void XMLFileReader::open(const char* data, const std::string& _filename)
{
    // close previous parser
    if (parser) xmlFreeTextReader(parser);
    
    // create parser
    filename = _filename;
    parser = xmlReaderForMemory(data, static_cast<int>(strlen(data)), filename.c_str(), NULL, XML_PARSE_NOBLANKS | XML_PARSE_NOENT | XML_PARSE_NONET);
    
    // report failed parser instanciation
    if (parser == NULL) mythrow("Unable to open file \"%s\"", filename);
}

// open file for reading
void XMLFileReader::open(const std::string& _filename)
{
    // close previous parser
    if (parser) xmlFreeTextReader(parser);
    
    // create parser
    filename = _filename;
    parser = xmlReaderForFile(filename.c_str(), NULL, XML_PARSE_NOBLANKS | XML_PARSE_NOENT | XML_PARSE_NONET);
    
    // report failed parser instanciation
    if (parser == NULL) mythrow("Unable to open file \"%s\"", filename);
}

// close file
void XMLFileReader::close()
{
    if (!parser) return;        // do nothing, of no file is open
    xmlFreeTextReader(parser);  // delete parser instance
    parser = NULL;              // erase parser
}

// use existsing libxml2 text-reader instance
void XMLFileReader::xopen(_xmlTextReader* reader, const std::string& _filename)
{
    // close previous parser
    if (parser) xmlFreeTextReader(parser);
    
    // copy data
    filename = _filename;
    parser = reader;
}

// reset instance, do not close reader
void XMLFileReader::xclose()
{
    parser = NULL;
}

// check if a file is opened
bool XMLFileReader::is_open() const
{
    return (parser != NULL);
}

// return the filename (or NULL)
const char* XMLFileReader::get_filename() const
{
    return (parser ? filename.c_str() : NULL);
}

//
//  document parser
// =================
//
XMLDocumentReader::XMLDocumentReader() : FileReader("ScorePress XML")
{
    add_mime_type("application/x-scorepress+xml");
    add_mime_type("application/xml");
    add_mime_type("text/xml");
    add_file_extension("*.xml");
    add_file_extension("*.xscorepress");
}

void XMLDocumentReader::parse_document(Document& target)
{
    // check, if a file is open
    if (!parser)
        throw Error("FileReader has no open file (please call 'XMLFileReader::open' first).");
    
    // strip the filename of its path (for the use in error messages)
    const std::string err_file = (filename.rfind('/') != std::string::npos) ?
                                    filename.substr(filename.rfind('/') + 1) :
                                    filename;
    
    // parser state enumerator
    enum enuState
    {   BEGIN, END,
        DOCUMENT,
        META, SPRITESETS, PAGE, ATTACHED, SCORES
    };
    enuState state = BEGIN;     // set current state
    //enuState prestate = BEGIN;  // set the previous state (used for returning from a state)
    
    const xmlChar* tag = NULL;  // tag string
    xmlChar* ver = NULL;        // version string
    xmlChar* attr = NULL;       // attribute string
    
    int parser_return = 0;      // parser return value buffer
    
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
            xmlFree(attr);
            
            // changing state
            state = DOCUMENT;
            break;
            
        //  state after the final </document>, expecting the end of data
        // ----------------------------------
        case END:
            mythrow_eof("Expected EOF (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            
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
                // parse the sprite information
                state = SPRITESETS;
            }
            
            // end tag </symbols>
            else if (xmlStrEqual(tag, STR_CAST("document")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
            {
                // the info should end with that
                state = END;
            }
            
            // any other tag is illegal here
            else mythrow("Expected one of <meta>, <spritesets> or </documnet> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            break;
        
        //  <meta> information
        // --------------------
        case META:
            // document main <title>
            if (xmlStrEqual(tag, STR_CAST("title")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(target.meta.title, "title");
            
            // document's <subtitle>
            else if (xmlStrEqual(tag, STR_CAST("subtitle")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(target.meta.subtitle, "subtitle");
            
            // <artist> name
            else if (xmlStrEqual(tag, STR_CAST("artist")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(target.meta.artist, "artist");
            
            // main <key> (or keys)
            else if (xmlStrEqual(tag, STR_CAST("key")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(target.meta.key, "key");
            
            // <date> of completion
            else if (xmlStrEqual(tag, STR_CAST("date")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(target.meta.date, "date");
            
            // <number> in a collection
            else if (xmlStrEqual(tag, STR_CAST("number")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(target.meta.number, "number");
            
            // <transcriptor> name
            else if (xmlStrEqual(tag, STR_CAST("transcriptor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(target.meta.transcriptor, "transcriptor");
            
            // <opus> number
            else if (xmlStrEqual(tag, STR_CAST("opus")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(target.meta.opus, "opus");
            
            // end tag </meta>
            else if (xmlStrEqual(tag, STR_CAST("meta")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                state = DOCUMENT;
            
            // any other tag will be inserted into the "misc" map
            else
                read_string(target.meta.misc[XML_CAST(tag)], XML_CAST(tag));
            
            break;
        
        // TODO: continue document parsing
        case SPRITESETS:
        case PAGE:
        case ATTACHED:
        case SCORES: break;
        };
    };
    
    // check for return values
    if (parser_return == 1)     // if there is more data behind the complete structure
        mythrow_eof("Expected EOF (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    if (parser_return == -1)    // if there occured an XML-syntax error
        mythrow("XML-Syntax Error in description (in file \"%s\", near EOF)", filename);
}


//
//     class XMLSpritesetReader
//    ==========================
//
// This class implements a parser for the sprite-file's meta-information,
// preparing a "Spriteset" object.
//
XMLSpritesetReader::XMLSpritesetReader() : FileReader("ScorePress Spriteset")
{
    add_mime_type("application/xml");
    add_mime_type("text/xml");
    add_file_extension("*.xml");
}

void XMLSpritesetReader::parse_spriteset(SpriteSet& spriteset, Renderer& renderer, const size_t setid)
{
    // check, if a file is open
    if (!parser)
        throw Error("FileReader has no open file (please call 'XMLFileReader::open' first).");
    
    // erase spriteset
    spriteset.clear();
    spriteset.file = filename;
    
    // strip the filename of its path (for the use in error messages)
    const std::string err_file = (filename.rfind('/') != std::string::npos) ?
                                    filename.substr(filename.rfind('/') + 1) :
                                    filename;
    
    // parser state enumerator
    enum enuState {BEGIN, END, SYMBOLS, INFO, SPRITES, BASE, MOVABLES, GROUP, TYPEFACE};
    enuState state = BEGIN;     // set current state
    enuState prestate = BEGIN;  // set the previous state (used for returning from a state)
    
    const xmlChar* tag = NULL;  // tag string
    xmlChar* attr = NULL;       // attribute string
    
    int parser_return = 0;      // parser return value buffer
    
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
            // expecting <symbols> to be the root tag
            if (xmlStrEqual(tag, STR_CAST("symbols")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_ELEMENT) mythrow("Expected tag <symbols> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            
            // checking "id" attribute
            attr = xmlTextReaderGetAttribute(parser, STR_CAST("id"));
            if (attr == NULL) mythrow("Missing \"id\"-attribute for <symbols>-tag (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            spriteset.info["id"] = XML_CAST(attr);
            xmlFree(attr);
            
            // changing state
            if (!xmlTextReaderIsEmptyElement(parser)) state = SYMBOLS;
            break;
            
        //  state after the final </symbols>, expecting the end of data
        // ----------------------------------
        case END:
            mythrow_eof("Expected EOF (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            
        //  <symbols> root section
        // ------------------------
        case SYMBOLS:
            // the libraries <info> section
            if (xmlStrEqual(tag, STR_CAST("info")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                if (!xmlTextReaderIsEmptyElement(parser)) state = INFO;   // parse the section
            }
            
            // the <sprites> section
            else if (xmlStrEqual(tag, STR_CAST("sprites")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // get "unit" attribute (i.e. head-height)
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("unit"));
                if (attr == NULL) mythrow("Missing \"unit\"-attribute for <sprites> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.head_height = atoi(XML_CAST(attr));
                if (spriteset.head_height == 0) mythrow("Illegal value for \"unit\"-attribute for <sprites> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                xmlFree(attr);
                
                // parse the sprite information
                if (!xmlTextReaderIsEmptyElement(parser)) state = SPRITES;
            }
            
            // end tag </symbols>
            else if (xmlStrEqual(tag, STR_CAST("symbols")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
            {
                // the info should end with that
                state = END;
            }
            
            // any other tag is illegal here
            else mythrow("Expected one of <info>, <sprites> or </symbols> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            break;
            
        //  <info> structure
        // ------------------
        case INFO:
            // <author> information
            if (xmlStrEqual(tag, STR_CAST("author")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(spriteset.info["author"], "author");
            
            // <copyright> information
            else if (xmlStrEqual(tag, STR_CAST("copyright")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(spriteset.info["copyright"], "copyright");
            
            // <license> information
            else if (xmlStrEqual(tag, STR_CAST("license")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(spriteset.info["license"], "license");
            
            // <description> string
            else if (xmlStrEqual(tag, STR_CAST("description")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(spriteset.info["description"], "description");
            
            // <date> string
            else if (xmlStrEqual(tag, STR_CAST("date")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_string(spriteset.info["date"], "date");
            
            // end tag </info>
            else if (xmlStrEqual(tag, STR_CAST("info")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                state = SYMBOLS;
            
            // any other tag is illegal here
            else mythrow("Expected one of <author>, <copyright>, <license>, <description> or </info> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            break;
            
        //  <sprites> section
        // -------------------
        case SPRITES:
            // <base> sprites
            if (xmlStrEqual(tag, STR_CAST("base")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                if (!xmlTextReaderIsEmptyElement(parser)) state = BASE;
            }
            
            // user defined <movable> sprites
            else if (xmlStrEqual(tag, STR_CAST("movables")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                if (!xmlTextReaderIsEmptyElement(parser)) state = MOVABLES;
            }
            
            // end tag </sprites>
            else if (xmlStrEqual(tag, STR_CAST("sprites")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
            {
                if (!xmlTextReaderIsEmptyElement(parser)) state = SYMBOLS;
            }
            
            // any other tag is illegal here
            else mythrow("Expected one of <base>, <movables> or </sprites> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            break;
            
        //  <base> sprites section
        // ------------------------
        case BASE:
            // <head> sprite
            if (xmlStrEqual(tag, STR_CAST("head")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check head type
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                if (attr == NULL) mythrow("Missing \"type\"-attribute for <head> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // create sprite
                if (xmlStrcasecmp(attr, STR_CAST("longa")) == 0)        // longa head
                {
                    spriteset.heads_longa = spriteset.size();
                    spriteset.ids["base.heads.longa"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::HEADS_LONGA));
                    spriteset.back().text[" class "] = "base.heads.longa";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("breve")) == 0)   // breve head
                {
                    spriteset.heads_breve = spriteset.size();
                    spriteset.ids["base.heads.breve"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::HEADS_BREVE));
                    spriteset.back().text[" class "] = "base.heads.breve";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("whole")) == 0)   // whole head
                {
                    spriteset.heads_whole = spriteset.size();
                    spriteset.ids["base.heads.whole"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::HEADS_WHOLE));
                    spriteset.back().text[" class "] = "base.heads.whole";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("half")) == 0)    // half head
                {
                    spriteset.heads_half = spriteset.size();
                    spriteset.ids["base.heads.half"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::HEADS_HALF));
                    spriteset.back().text[" class "] = "base.heads.half";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("quarter")) == 0) // quarter head
                {
                    spriteset.heads_quarter = spriteset.size();
                    spriteset.ids["base.heads.quarter"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::HEADS_QUARTER));
                    spriteset.back().text[" class "] = "base.heads.quarter";
                }
                else    // illegal head type (throw error message)
                {
                    xmlFree(attr);  // free attribute buffer
                    mythrow("Illegal value of \"type\"-attribute for <head> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                xmlFree(attr);
                
                // read path attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <head> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <head> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read anchor attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read stem attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("stem-x"));
                if (attr != NULL) spriteset.back().real["stem.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("stem-y"));
                if (attr != NULL) spriteset.back().real["stem.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("head")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </head> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // <rest> sprite
            else if (xmlStrEqual(tag, STR_CAST("rest")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check rest type
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                if (attr == NULL) mythrow("Missing \"type\"-attribute for <rest> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // create sprite
                if (xmlStrcasecmp(attr, STR_CAST("longa")) == 0)        // longa rest
                {
                    spriteset.rests_longa = spriteset.size();
                    spriteset.ids["base.rests.longa"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::RESTS_LONGA));
                    spriteset.back().text[" class "] = "base.rests.longa";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("breve")) == 0)   // breve rest
                {
                    spriteset.rests_breve = spriteset.size();
                    spriteset.ids["base.rests.breve"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::RESTS_BREVE));
                    spriteset.back().text[" class "] = "base.rests.breve";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("whole")) == 0)   // whole rest
                {
                    spriteset.rests_whole = spriteset.size();
                    spriteset.ids["base.rests.whole"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::RESTS_WHOLE));
                    spriteset.back().text[" class "] = "base.rests.whole";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("half")) == 0)    // half rest
                {
                    spriteset.rests_half = spriteset.size();
                    spriteset.ids["base.rests.half"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::RESTS_HALF));
                    spriteset.back().text[" class "] = "base.rests.half";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("quarter")) == 0) // quarter rest
                {
                    spriteset.rests_quarter = spriteset.size();
                    spriteset.ids["base.rests.quarter"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::RESTS_QUARTER));
                    spriteset.back().text[" class "] = "base.rests.quarter";
                }
                else    // illegal rest type (throw error message)
                {
                    xmlFree(attr);  // free attribute buffer
                    mythrow("Illegal value of \"type\"-attribute for <rest> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                xmlFree(attr);
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <rest> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <rest> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "line" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("line"));
                if (attr != NULL) spriteset.back().integer["line"] = atoi(XML_CAST(attr));
                xmlFree(attr);
                
                // read "anchor" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("rest")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </rest> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // <flag> sprite
            else if (xmlStrEqual(tag, STR_CAST("flag")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check flag type (note or rest)
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                if (attr == NULL) mythrow("Missing \"type\"-attribute for <flag> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // create sprite
                if (xmlStrcasecmp(attr, STR_CAST("note")) == 0)       // note flag
                {
                    spriteset.flags_note = spriteset.size();
                    spriteset.ids["base.flags.note"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::FLAGS_NOTE));
                    spriteset.back().text[" class "] = "base.flags.note";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("rest")) == 0)  // rest flag
                {
                    spriteset.flags_rest = spriteset.size();
                    spriteset.ids["base.flags.rest"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::FLAGS_REST));
                    spriteset.back().text[" class "] = "base.flags.rest";
                }
                else    // illegal flag type (throw error message)
                {
                    xmlFree(attr);  // free attribute buffer
                    mythrow("Illegal value of \"type\"-attribute for <flag> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                xmlFree(attr);
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <flag> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <flag> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "anchor" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // further attributes
                if (spriteset.back().type == SpriteInfo::FLAGS_NOTE)
                {
                    // read "overlay" attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("overlay"));
                    if (attr != NULL) spriteset.back().text["overlay"] = XML_CAST(attr);
                    xmlFree(attr);
                    
                    // read "distance" attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("distance"));
                    if (attr != NULL) spriteset.back().real["distance"] = strtod(XML_CAST(attr), NULL);
                    xmlFree(attr);
                }
                else
                {
                    // read "base" attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("base"));
                    if (attr != NULL) spriteset.back().text["restbase"] = XML_CAST(attr);
                    xmlFree(attr);
                    
                    // read "line" attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("line"));
                    if (attr != NULL) spriteset.back().integer["line"] = atoi(XML_CAST(attr));
                    xmlFree(attr);
                    
                    // read "stem-top" attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("stem-top"));
                    if (attr != NULL)
                    {
                        char* ptr;
                        spriteset.back().real["stem.top.x1"] = strtod(XML_CAST(attr), &ptr);
                        spriteset.back().real["stem.top.y1"] = strtod(ptr,            &ptr);
                        spriteset.back().real["stem.top.x2"] = strtod(ptr,            &ptr);
                        spriteset.back().real["stem.top.y2"] = strtod(ptr,            NULL);
                    };
                    xmlFree(attr);
                    
                    // read "stem-bottom" attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("stem-bottom"));
                    if (attr != NULL)
                    {
                        char* ptr;
                        spriteset.back().real["stem.bottom.x1"] = strtod(XML_CAST(attr), &ptr);
                        spriteset.back().real["stem.bottom.y1"] = strtod(ptr,            &ptr);
                        spriteset.back().real["stem.bottom.x2"] = strtod(ptr,            &ptr);
                        spriteset.back().real["stem.bottom.y2"] = strtod(ptr,            NULL);
                    };
                    xmlFree(attr);
                    
                    // read "stem-minlen" attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("stem-minlen"));
                    if (attr != NULL) spriteset.back().real["stem.minlen"] = strtod(XML_CAST(attr), NULL);
                    xmlFree(attr);
                    
                    // read "stem-slope" attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("stem-slope"));
                    if (attr != NULL) spriteset.back().real["stem.slope"] = strtod(XML_CAST(attr), NULL);
                    xmlFree(attr);
                };
                
                // prepare additional paths
                if (spriteset.back().type == SpriteInfo::FLAGS_NOTE)
                {
                    // create sprite-info for the overlay-path
                    if (spriteset.back().text.find("overlay") != spriteset.back().text.end())
                    {
                        spriteset.flags_overlay = spriteset.size();                 // save id
                        spriteset.ids["base.flag.overlay"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::FLAGS_OVERLAY)); // add sprite
                        spriteset.back().text[" class "] = "base.flag.overlay";
                        spriteset.back().path = spriteset[spriteset.size() - 2].text["overlay"];
                        if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for \"overlay\" in <flag> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    }
                    // if there is no overlay sprite, use the same sprite as for the flag
                    else spriteset.flags_overlay = spriteset.size() - 1;
                }
                else
                {
                    // create sprite-info for the base-path
                    if (spriteset.back().text.find("restbase") != spriteset.back().text.end())
                    {
                        spriteset.flags_base = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::FLAGS_BASE));
                        spriteset.back().text[" class "] = "base.flag.restbase";
                        spriteset.back().path = spriteset[spriteset.size() - 2].text["restbase"];
                        if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for \"restbase\" in <flag> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                };
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("flag")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </flag> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // <dot> symbol
            else if (xmlStrEqual(tag, STR_CAST("dot")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // create sprite
                spriteset.dot = spriteset.size();
                spriteset.ids["base.dot"] = spriteset.size();
                spriteset.push_back(SpriteInfo(SpriteInfo::DOT));
                spriteset.back().text[" class "] = "base.dot";
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <dot> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <dot> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "anchor" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "distance" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("distance"));
                if (attr != NULL) spriteset.back().real["distance"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "offset" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("offset"));
                if (attr != NULL) spriteset.back().real["offset"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("dot")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </dot> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // <accidental> sprite
            else if (xmlStrEqual(tag, STR_CAST("accidental")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check accidental type
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                if (attr == NULL) mythrow("Missing \"type\"-attribute for <accidental> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // create sprite
                if (xmlStrcasecmp(attr, STR_CAST("double-sharp")) == 0)         // double sharp
                {
                    spriteset.accidentals_double_sharp = spriteset.size();
                    spriteset.ids["base.accidentals.double-sharp"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_DOUBLESHARP));
                    spriteset.back().text[" class "] = "base.accidentals.double-sharp";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("sharp-andahalf")) == 0)  // 1 1/2 sharp
                {
                    spriteset.accidentals_sharp_andahalf = spriteset.size();
                    spriteset.ids["base.accidentals.sharp-andahalf"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_SHARPANDAHALF));
                    spriteset.back().text[" class "] = "base.accidentals.sharp-andahalf";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("sharp")) == 0)           // sharp
                {
                    spriteset.accidentals_sharp = spriteset.size();
                    spriteset.ids["base.accidentals.sharp"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_SHARP));
                    spriteset.back().text[" class "] = "base.accidentals.sharp";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("half-sharp")) == 0)      // half sharp
                {
                    spriteset.accidentals_half_sharp = spriteset.size();
                    spriteset.ids["base.accidentals.half-sharp"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_HALFSHARP));
                    spriteset.back().text[" class "] = "base.accidentals.half-sharp";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("natural")) == 0)         // natural
                {
                    spriteset.accidentals_natural = spriteset.size();
                    spriteset.ids["base.accidentals.natural"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_NATURAL));
                    spriteset.back().text[" class "] = "base.accidentals.natural";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("half-flat")) == 0)       // half flat
                {
                    spriteset.accidentals_half_flat = spriteset.size();
                    spriteset.ids["base.accidentals.half-flat"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_HALFFLAT));
                    spriteset.back().text[" class "] = "base.accidentals.half-flat";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("flat")) == 0)            // flat
                {
                    spriteset.accidentals_flat = spriteset.size();
                    spriteset.ids["base.accidentals.flat"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_FLAT));
                    spriteset.back().text[" class "] = "base.accidentals.flat";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("flat-andahalf")) == 0)   // 1 1/2 flat
                {
                    spriteset.accidentals_flat_andahalf = spriteset.size();
                    spriteset.ids["base.accidentals.flat-andahalf"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_FLATANDAHALF));
                    spriteset.back().text[" class "] = "base.accidentals.flat-andahalf";
                }
                else if (xmlStrcasecmp(attr, STR_CAST("double-flat")) == 0)     // double flat
                {
                    spriteset.accidentals_double_flat = spriteset.size();
                    spriteset.ids["base.accidentals.double-flat"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_DOUBLEFLAT));
                    spriteset.back().text[" class "] = "base.accidentals.double-flat";
                }
                else    // illegal accidental type (throw error message)
                {
                    xmlFree(attr);  // free attribute buffer
                    mythrow("Illegal value of \"type\"-attribute for <accidental> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                xmlFree(attr);
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <accidental> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <accidental> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "anchor" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "offset" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("offset"));
                if (attr != NULL) spriteset.back().real["offset"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("accidental")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </accidental> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // <brace> sprite
            else if (xmlStrEqual(tag, STR_CAST("brace")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // create sprite
                spriteset.brace = spriteset.size();
                spriteset.ids["base.brace"] = spriteset.size();
                spriteset.push_back(SpriteInfo(SpriteInfo::BRACE));     // create sprite
                spriteset.back().text[" class "] = "base.brace";        // set class
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <brace> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <brace> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "anchor" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "hmin" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("hmin"));
                if (attr != NULL) spriteset.back().real["hmin"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "hmax" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("hmax"));
                if (attr != NULL) spriteset.back().real["hmax"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "low" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("low"));
                if (attr != NULL) spriteset.back().real["low"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "high" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("high"));
                if (attr != NULL) spriteset.back().real["high"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("brace")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </brace> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // <bracket> sprite
            else if (xmlStrEqual(tag, STR_CAST("bracket")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // create sprite
                spriteset.bracket = spriteset.size();
                spriteset.ids["base.bracket"] = spriteset.size();
                spriteset.push_back(SpriteInfo(SpriteInfo::BRACKET));   // create sprite
                spriteset.back().text[" class "] = "base.bracket";      // set class
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <bracket> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <bracket> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "anchor" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "line-width" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("line-width"));
                if (attr == NULL) mythrow("Missing \"line-width\"-attribute for <bracket> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().real["linewidth"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("bracket")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </bracket> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // <articulation> symbol
            else if (xmlStrEqual(tag, STR_CAST("articulation")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check symbol id
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("id"));
                if (attr == NULL) mythrow("Missing \"id\"-attribute for <articulation> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.push_back(SpriteInfo(SpriteInfo::ARTICULATION));  // create sprite
                spriteset.back().text[" class "] = "articulation.";         // set class
                spriteset.back().text[" class "].append(XML_CAST(attr));
                xmlFree(attr);                                              // free attribute buffer
                if (spriteset.ids.find(spriteset.back().text[" class "]) != spriteset.ids.end())
                    mythrow("Redefinition of articulation symbol \"%s\" (in file \"%s\", at line %i:%i)", spriteset.back().text[" class "], err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.ids[spriteset.back().text[" class "]] = spriteset.size() - 1;
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <articulation> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <articulation> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "anchor" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "offset" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("offset"));
                if (attr != NULL) spriteset.back().real["offset"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "valuemod" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("valuemod"));
                if (attr != NULL) spriteset.back().integer["valuemod"] = atoi(XML_CAST(attr));
                xmlFree(attr);
                
                // read "volumemod" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("volumemod"));
                if (attr != NULL) spriteset.back().integer["volumemod"] = atoi(XML_CAST(attr));
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("articulation")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </articulation> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // <clef> sprite
            else if (xmlStrEqual(tag, STR_CAST("clef")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check symbol id
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("id"));
                if (attr == NULL) mythrow("Missing \"id\"-attribute for <clef> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.push_back(SpriteInfo(SpriteInfo::ARTICULATION));  // create sprite
                spriteset.back().text[" class "] = "clef.";                 // set class
                spriteset.back().text[" class "].append(XML_CAST(attr));
                xmlFree(attr);                                              // free attribute buffer
                if (spriteset.ids.find(spriteset.back().text[" class "]) != spriteset.ids.end())
                    mythrow("Redefinition of clef symbol \"%s\" (in file \"%s\", at line %i:%i)", spriteset.back().text[" class "], filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.ids[spriteset.back().text[" class "]] = spriteset.size() - 1;
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <clef> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <clef> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "anchor" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "basenote" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("basenote"));
                if (attr == NULL) mythrow("Missing \"basenote\"-attribute for <clef> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().integer["basenote"] = atoi(XML_CAST(attr));
                xmlFree(attr);
                
                // read "line" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("line"));
                if (attr != NULL) spriteset.back().integer["line"] = atoi(XML_CAST(attr));
                xmlFree(attr);
                
                // read "keybound-sharp" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("keybound-sharp"));
                if (attr != NULL) spriteset.back().integer["keybound.sharp"] = atoi(XML_CAST(attr));
                xmlFree(attr);
                
                // read "keybound-flat" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("keybound-flat"));
                if (attr != NULL) spriteset.back().integer["keybound.flat"] = atoi(XML_CAST(attr));
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("clef")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </clef> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // <timesig> sprite
            else if (xmlStrEqual(tag, STR_CAST("timesig")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check time-signature type (digit or symbol)
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                if (attr == NULL) mythrow("Missing \"type\"-attribute for <timesig> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // create sprite
                if (xmlStrcasecmp(attr, STR_CAST("digit")) == 0)        // time-signature digit
                {
                    // get "digit" attribute
                    xmlFree(attr);
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("digit"));
                    if (attr == NULL) mythrow("Missing \"digit\"-attribute for <timesig> of digit type (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    const int digit = atoi(XML_CAST(attr));
                    xmlFree(attr);
                    if (digit < 0 || digit > 9) mythrow("\"digit\"-attribute for <timesig> out of range (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // create sprite
                    spriteset.push_back(SpriteInfo(SpriteInfo::TIMESIG_DIGIT));
                    spriteset.back().text[" class "] = "timesig.digit_";
                    spriteset.back().text[" class "].push_back(('0' + digit) & 0x7F);
                    spriteset.back().integer["digit"] = digit;
                    spriteset.digits_time[digit] = spriteset.size() - 1; 
                    spriteset.ids[spriteset.back().text[" class "]] = spriteset.size() - 1;
                }
                else if (xmlStrcasecmp(attr, STR_CAST("symbol")) == 0)  // time-signature symbol
                {
                    // create sprite
                    spriteset.flags_rest = spriteset.size();
                    spriteset.ids["base.flags.rest"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::TIMESIG));
                    spriteset.back().text[" class "] = "timesig.symbol_";
                    
                    // get number and beat from "time"
                    xmlFree(attr);
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("time"));
                    if (attr == NULL) mythrow("Missing \"time\"-attribute for <timesig> of symbol type (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    char* ptr;
                    const long tmp = strtol(XML_CAST(attr), &ptr, 0);
                    if (*ptr != '/' || *++ptr == 0 || tmp > std::numeric_limits<int>::max() || tmp < 0)
                    {
                        xmlFree(attr);
                        mythrow("Syntax error in \"time\"-attribute for <timesig> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    spriteset.back().integer["number"] = static_cast<int>(tmp);
                    spriteset.back().integer["beat"] = atoi(ptr);
                    xmlFree(attr);
                    
                    // compose class name
                    char* buffer = new char[static_cast<int>(log10(spriteset.back().integer["number"])) + static_cast<int>(log10(spriteset.back().integer["beat"])) + 4];
                    sprintf(buffer, "%i_%i", spriteset.back().integer["number"], spriteset.back().integer["beat"]);
                    spriteset.back().text[" class "].append(buffer);
                    delete[] buffer;
                    spriteset.back().text[" class "].push_back('_');
                    spriteset.back().text[" class "].append(spriteset.back().path);
                    spriteset.ids[spriteset.back().text[" class "]] = spriteset.size() - 1;
                }
                else    // illegal timesig type (throw error message)
                {
                    xmlFree(attr);  // free attribute buffer
                    mythrow("Illegal value of \"type\"-attribute for <timesig> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <timesig> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <timesig> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "anchor" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("timesig")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </timesig> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // end tag </base>
            else if (xmlStrEqual(tag, STR_CAST("base")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
            {
                state = SPRITES;
            }
            
            // any other tag is illegal here
            else mythrow("Expected one of <head>, <flag>, <rest>, <dot>, <accidental>, <brace>, <bracket>, <articulation>, <clef>, <timesig> or </base> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            break;
        
        //  <movables> user defined symbols
        // --------------------------------
        case MOVABLES:
            // sprite <group> definition
            if (xmlStrEqual(tag, STR_CAST("group")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check id attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("id"));
                if (attr == NULL) mythrow("Missing \"id\"-attribute for <group> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.groups.push_back(SpriteSet::Group());
                spriteset.groups.back().id = XML_CAST(attr);
                spriteset.gids[spriteset.groups.back().id] = spriteset.groups.size() - 1;
                xmlFree(attr);
                
                // parse group contents
                if (xmlTextReaderIsEmptyElement(parser)) break;
                state = GROUP;
            }
            
            // symbol <typeface> definition
            else if (xmlStrEqual(tag, STR_CAST("typeface")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check emtpy tag
                if (xmlTextReaderIsEmptyElement(parser)) break;
                
                // check id attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("id"));
                if (attr == NULL) mythrow("Missing \"id\"-attribute for <typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.typefaces.push_back(SpriteSet::Typeface());
                spriteset.typefaces.back().id = XML_CAST(attr);
                spriteset.fids[spriteset.typefaces.back().id] = spriteset.typefaces.size() - 1;
                xmlFree(attr);
                
                // read "ascent" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("ascent"));
                if (attr == NULL) mythrow("Missing \"ascent\"-attribute for <typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.typefaces.back().ascent = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "descent" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("descent"));
                if (attr == NULL) mythrow("Missing \"descent\"-attribute for <typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.typefaces.back().descent = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "general-use" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("general-use"));
                if (attr == NULL)
                    spriteset.typefaces.back().general_use = true;      // typefaces outside any group are for general use
                else if (xmlStrcasecmp(attr, STR_CAST("yes")) == 0 || xmlStrcasecmp(attr, STR_CAST("true")) == 0)
                    spriteset.typefaces.back().general_use = true;
                else if (xmlStrcasecmp(attr, STR_CAST("no")) == 0 || xmlStrcasecmp(attr, STR_CAST("false")) == 0)
                    spriteset.typefaces.back().general_use = false;
                else
                {
                    xmlFree(attr);
                    mythrow("Illegal value of \"general-use\"-attribute for <typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                xmlFree(attr);
                
                // read "custom-use" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("custom-use"));
                if (attr == NULL)
                    spriteset.typefaces.back().custom_use = false;
                else if (xmlStrcasecmp(attr, STR_CAST("yes")) == 0 || xmlStrcasecmp(attr, STR_CAST("true")) == 0)
                    spriteset.typefaces.back().custom_use = true;
                else if (xmlStrcasecmp(attr, STR_CAST("no")) == 0 || xmlStrcasecmp(attr, STR_CAST("false")) == 0)
                    spriteset.typefaces.back().custom_use = false;
                else
                {
                    xmlFree(attr);
                    mythrow("Illegal value of \"custom-use\"-attribute for <typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                xmlFree(attr);
                
                // parse typeface contents
                if (xmlTextReaderIsEmptyElement(parser)) break;
                prestate = MOVABLES;
                state = TYPEFACE;
            }
            
            // end tag </movables>
            else if (xmlStrEqual(tag, STR_CAST("movables")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                state = SPRITES;
            
            // any other tag is illegal here
            else mythrow("Expected <group>, <typeface> or </movables> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            break;
        
        //  sprite <group> definition
        // ---------------------------
        case GROUP:
            // the group's <name>
            if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_i18n(spriteset.groups.back().name, "name");
            
            // <symbol> sprites
            else if (xmlStrEqual(tag, STR_CAST("symbol")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check id attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("id"));
                if (attr == NULL) mythrow("Missing \"id\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // create sprite
                spriteset.push_back(SpriteInfo(SpriteInfo::SYMBOL));
                spriteset.back().text[" class "] = "symbol.";
                spriteset.back().text[" class "].append(spriteset.groups.back().id);
                spriteset.back().text[" class "].push_back('.');
                spriteset.back().text[" class "].append(XML_CAST(attr));
                spriteset.ids[spriteset.back().text[" class "]] = spriteset.size() - 1;
                xmlFree(attr);
                
                // get symbol type
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                if (attr == NULL || xmlStrcasecmp(attr, STR_CAST("single")) == 0)
                {
                    xmlFree(attr);
                    spriteset.back().integer["is_string"] = 0;
                }
                else if (xmlStrcasecmp(attr, STR_CAST("string")) == 0)
                {
                    xmlFree(attr);
                    spriteset.back().integer["is_string"] = 1;
                    
                    // read "face" attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("face"));
                    if (attr != NULL) 
                        spriteset.back().text["face"] = XML_CAST(attr);
                    else if (spriteset.fids.find(spriteset.groups.back().id) != spriteset.fids.end())
                        spriteset.back().text["face"] = spriteset.groups.back().id; // if not present, use the group name as typeface (if the typeface exists)
                    else
                    {
                        xmlFree(attr);
                        mythrow("Missing \"face\"-attribute for <symbol> of \"string\" type (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                }
                else
                {
                    xmlFree(attr);  // free attribute buffer
                    mythrow("Illegal value of \"type\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!spriteset.back().integer["is_string"] && !renderer.exist(spriteset.back().path, setid))
                    mythrow("Unable to find path \"%s\" for <symbol> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "anchor" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-x"));
                if (attr != NULL) spriteset.back().real["anchor.x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("anchor-y"));
                if (attr != NULL) spriteset.back().real["anchor.y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "tempo-type" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("tempo-type"));
                if (attr == NULL)                                        spriteset.back().integer["tempo.type"] = static_cast<int>(ContextChanging::NONE);
                else if (xmlStrcasecmp(attr, STR_CAST("none"))     == 0) spriteset.back().integer["tempo.type"] = static_cast<int>(ContextChanging::NONE);
                else if (xmlStrcasecmp(attr, STR_CAST("abs"))      == 0) spriteset.back().integer["tempo.type"] = static_cast<int>(ContextChanging::ABSOLUTE);
                else if (xmlStrcasecmp(attr, STR_CAST("absolute")) == 0) spriteset.back().integer["tempo.type"] = static_cast<int>(ContextChanging::ABSOLUTE);
                else if (xmlStrcasecmp(attr, STR_CAST("rel"))      == 0) spriteset.back().integer["tempo.type"] = static_cast<int>(ContextChanging::RELATIVE);
                else if (xmlStrcasecmp(attr, STR_CAST("relative")) == 0) spriteset.back().integer["tempo.type"] = static_cast<int>(ContextChanging::RELATIVE);
                else if (xmlStrcasecmp(attr, STR_CAST("promille")) == 0) spriteset.back().integer["tempo.type"] = static_cast<int>(ContextChanging::PROMILLE);
                else mythrow("Illegal value of \"tempo-type\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                xmlFree(attr);
                
                // read "tempo" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("tempo"));
                if (attr != NULL)
                {
                    char* end;
                    const long tmp = strtol(XML_CAST(attr), &end, 0);
                    if ((*end != '\0' && *end != '%') || tmp > std::numeric_limits<int>::max() || tmp < std::numeric_limits<int>::min())
                    {
                        xmlFree(attr);
                        mythrow("Illegal value of \"tempo\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    spriteset.back().integer["tempo"] = static_cast<int>(tmp);
                    if (*end == '%')
                    {
                        xmlFree(attr);
                        if (spriteset.back().integer.find("tempo.type") == spriteset.back().integer.end())
                            spriteset.back().integer["tempo.type"] = static_cast<int>(ContextChanging::PROMILLE);
                        else if (spriteset.back().integer["tempo.type"] != static_cast<int>(ContextChanging::PROMILLE))
                            mythrow("Unexpected percent sign in \"tempo\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        spriteset.back().integer["tempo"] *= 10;
                    }
                };
                
                // read "volume-type" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("volume-type"));
                if (attr == NULL) /* NOOP */;
                else if (xmlStrcasecmp(attr, STR_CAST("none"))     == 0) spriteset.back().integer["volume.type"] = static_cast<int>(ContextChanging::NONE);
                else if (xmlStrcasecmp(attr, STR_CAST("abs"))      == 0) spriteset.back().integer["volume.type"] = static_cast<int>(ContextChanging::ABSOLUTE);
                else if (xmlStrcasecmp(attr, STR_CAST("absolute")) == 0) spriteset.back().integer["volume.type"] = static_cast<int>(ContextChanging::ABSOLUTE);
                else if (xmlStrcasecmp(attr, STR_CAST("rel"))      == 0) spriteset.back().integer["volume.type"] = static_cast<int>(ContextChanging::RELATIVE);
                else if (xmlStrcasecmp(attr, STR_CAST("relative")) == 0) spriteset.back().integer["volume.type"] = static_cast<int>(ContextChanging::RELATIVE);
                else if (xmlStrcasecmp(attr, STR_CAST("promille")) == 0) spriteset.back().integer["volume.type"] = static_cast<int>(ContextChanging::PROMILLE);
                else mythrow("Illegal value of \"volume-type\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                xmlFree(attr);
                
                // check "volume-scope" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("volume-scope"));
                if (attr == NULL) /* NOOP */;
                else if (xmlStrcasecmp(attr, STR_CAST("voice"))      == 0) spriteset.back().integer["volume.scope"] = static_cast<int>(ContextChanging::VOICE);
                else if (xmlStrcasecmp(attr, STR_CAST("staff"))      == 0) spriteset.back().integer["volume.scope"] = static_cast<int>(ContextChanging::STAFF);
                else if (xmlStrcasecmp(attr, STR_CAST("instrument")) == 0) spriteset.back().integer["volume.scope"] = static_cast<int>(ContextChanging::INSTRUMENT);
                else if (xmlStrcasecmp(attr, STR_CAST("group"))      == 0) spriteset.back().integer["volume.scope"] = static_cast<int>(ContextChanging::GROUP);
                else if (xmlStrcasecmp(attr, STR_CAST("score"))      == 0) spriteset.back().integer["volume.scope"] = static_cast<int>(ContextChanging::SCORE);
                else mythrow("Illegal value of \"volume-scope\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                xmlFree(attr);
                
                // read "volume" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("volume"));
                if (attr != NULL)
                {
                    char* end;
                    const long tmp = strtol(XML_CAST(attr), &end, 0);
                    if ((*end != '\0' && *end != '%') || tmp > std::numeric_limits<int>::max() || tmp < std::numeric_limits<int>::min())
                    {
                        xmlFree(attr);
                        mythrow("Illegal value of \"volume\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    spriteset.back().integer["volume"] = static_cast<int>(tmp);
                    if (*end == '%')
                    {
                        xmlFree(attr);
                        if (spriteset.back().integer.find("volume.type") == spriteset.back().integer.end())
                            spriteset.back().integer["volume.type"] = static_cast<int>(ContextChanging::PROMILLE);
                        else if (spriteset.back().integer["volume.type"] != static_cast<int>(ContextChanging::PROMILLE))
                            mythrow("Unexpected percent sign in \"volume\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        spriteset.back().integer["volume"] *= 10;
                    }
                    else
                    {
                        xmlFree(attr);
                        if (spriteset.back().integer.find("volume.type") == spriteset.back().integer.end())
                            spriteset.back().integer["volume.type"] = static_cast<int>(ContextChanging::ABSOLUTE);
                    };
                };
                
                // check "value-scope" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("value-scope"));
                if (attr == NULL) /* NOOP */;
                else if (xmlStrcasecmp(attr, STR_CAST("voice"))      == 0) spriteset.back().integer["value.scope"] = static_cast<int>(ContextChanging::VOICE);
                else if (xmlStrcasecmp(attr, STR_CAST("staff"))      == 0) spriteset.back().integer["value.scope"] = static_cast<int>(ContextChanging::STAFF);
                else if (xmlStrcasecmp(attr, STR_CAST("instrument")) == 0) spriteset.back().integer["value.scope"] = static_cast<int>(ContextChanging::INSTRUMENT);
                else if (xmlStrcasecmp(attr, STR_CAST("group"))      == 0) spriteset.back().integer["value.scope"] = static_cast<int>(ContextChanging::GROUP);
                else if (xmlStrcasecmp(attr, STR_CAST("score"))      == 0) spriteset.back().integer["value.scope"] = static_cast<int>(ContextChanging::SCORE);
                else mythrow("Illegal value of \"value-scope\"-attribute for <symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                xmlFree(attr);
                
                // read "value" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("value"));
                if (attr != NULL) spriteset.back().integer["value"] = atoi(XML_CAST(attr));
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("symbol")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </symbol> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // symbol <typeface> definition
            else if (xmlStrEqual(tag, STR_CAST("typeface")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check emtpy tag
                if (xmlTextReaderIsEmptyElement(parser)) break;
                
                // check id attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("id"));
                if (attr == NULL) mythrow("Missing \"id\"-attribute for <typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.typefaces.push_back(SpriteSet::Typeface());
                spriteset.typefaces.back().id = XML_CAST(attr);
                spriteset.fids[spriteset.typefaces.back().id] = spriteset.typefaces.size() - 1;
                xmlFree(attr);
                
                // read "ascent" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("ascent"));
                if (attr == NULL) mythrow("Missing \"ascent\"-attribute for <typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.typefaces.back().ascent = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "descent" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("descent"));
                if (attr == NULL) mythrow("Missing \"descent\"-attribute for <typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.typefaces.back().descent = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "general-use" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("general-use"));
                if (attr == NULL)
                    spriteset.typefaces.back().general_use = false;         // typefaces within a group are generally not for general use
                else if (xmlStrcasecmp(attr, STR_CAST("yes")) == 0 || xmlStrcasecmp(attr, STR_CAST("true")) == 0)
                    spriteset.typefaces.back().general_use = true;
                else if (xmlStrcasecmp(attr, STR_CAST("no")) == 0 || xmlStrcasecmp(attr, STR_CAST("false")) == 0)
                    spriteset.typefaces.back().general_use = false;
                else
                {
                    xmlFree(attr);
                    mythrow("Illegal value of \"general-use\"-attribute for <typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                xmlFree(attr);
                
                // read "custom-use" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("custom-use"));
                if (attr == NULL)
                    spriteset.typefaces.back().custom_use = false;
                else if (xmlStrcasecmp(attr, STR_CAST("yes")) == 0 || xmlStrcasecmp(attr, STR_CAST("true")) == 0)
                    spriteset.typefaces.back().custom_use = true;
                else if (xmlStrcasecmp(attr, STR_CAST("no")) == 0 || xmlStrcasecmp(attr, STR_CAST("false")) == 0)
                    spriteset.typefaces.back().custom_use = false;
                else
                {
                    xmlFree(attr);
                    mythrow("Illegal value of \"custom-use\"-attribute for <typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                xmlFree(attr);
                
                // parse typeface contents
                if (xmlTextReaderIsEmptyElement(parser)) break;
                prestate = GROUP;
                state = TYPEFACE;
            }
            
            // end tag </group>
            else if (xmlStrEqual(tag, STR_CAST("group")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                state = MOVABLES;
            
            // any other tag is illegal here
            else mythrow("Expected one of <name>, <symbol>, <glyph> or </group> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            break;
        
        //  symbol <typeface>
        // -------------------
        case TYPEFACE:
            // the typeface's <name>
            if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                read_i18n(spriteset.typefaces.back().name, "name");
            
            // <glyph> of the typeface
            else if (xmlStrEqual(tag, STR_CAST("glyph")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
            {
                // check char attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("char"));
                if (attr == NULL) mythrow("Missing \"char\"-attribute for <glyph> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                if (xmlUTF8Strlen(attr) != 1) mythrow("Illegal length of \"char\"-attribute for <glyph> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                xmlChar* c = xmlUTF8Strndup(attr, 1);
                xmlFree(attr);
                
                if (spriteset.typefaces.back().glyphs.find(XML_CAST(c)) != spriteset.typefaces.back().glyphs.end()) mythrow("Redefinition of character by <glyph> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.typefaces.back().glyphs[XML_CAST(c)] = spriteset.size();
                
                // create sprite
                spriteset.push_back(SpriteInfo(SpriteInfo::GLYPH));     // create sprite
                spriteset.back().text[" class "] = "glyph.";            // set class
                spriteset.back().text[" class "].append(spriteset.typefaces.back().id);
                spriteset.back().text[" class "].push_back('.');
                spriteset.back().text[" class "].append(XML_CAST(c));
                spriteset.ids[spriteset.back().text[" class "]] = spriteset.size() - 1;
                spriteset.back().text["char"] = XML_CAST(c);
                
                // read "path" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("path"));
                if (attr == NULL) mythrow("Missing \"path\"-attribute for <glyph> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.back().path = XML_CAST(attr);
                xmlFree(attr);
                if (!renderer.exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" for <glyph> (requested in file \"%s\", at line %i:%i)", spriteset.back().path, err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // read "bearing-x" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("bearing-x"));
                if (attr != NULL) spriteset.back().real["bearing-x"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "bearing-y" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("bearing-y"));
                if (attr != NULL) spriteset.back().real["bearing-y"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // read "advance" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("advance"));
                if (attr != NULL) spriteset.back().real["advance"] = strtod(XML_CAST(attr), NULL);
                xmlFree(attr);
                
                // check for name entries
                if (xmlTextReaderIsEmptyElement(parser)) break;
                while (xmlTextReaderRead(parser) == 1)
                {
                    tag = xmlTextReaderConstName(parser);
                    if (xmlTextReaderNodeType(parser) == XML_READER_TYPE_COMMENT) continue;
                    if (xmlStrEqual(tag, STR_CAST("name")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                        read_i18n(spriteset.back().name, "name", "en", true);
                    else if (xmlStrEqual(tag, STR_CAST("glyph")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                        break;
                    else
                        mythrow("Expected either <name> or </glyph> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
            }
            
            // end tag </typeface>
            else if (xmlStrEqual(tag, STR_CAST("typeface")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                state = prestate;
            
            // any other tag is illegal here
            else mythrow("Expected one of <name>, <glyph> or </typeface> (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
            break;
        };
    };
    
    // check for return values
    if (parser_return == 1)     // if there is more data behind the complete structure
        mythrow_eof("Expected EOF (in file \"%s\", at line %i:%i)", err_file, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
    if (parser_return == -1)    // if there occured an XML-syntax error
        mythrow("XML-Syntax Error in description (in file \"%s\", near EOF)", err_file);
}



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

#include "file_format.hh"
#include <cstdlib>              // atoi, strtof
#include <cstring>              // strlen, sprintf
#include <cmath>                // log10, sqrt
#include <libxml/xmlreader.h>   // xmlReaderForFile, ...

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
XMLFileReader::XMLFileReader() : DocumentReader("ScorePress XML"), parser(NULL), filename(NULL)
{
    add_mime_type("application/x-scorepress+xml");
    add_mime_type("application/xml");
    add_mime_type("text/xml");
    add_file_extension("*.xml");
    add_file_extension("*.xscorepress");
}

// destructor
XMLFileReader::~XMLFileReader()
{
    if (parser) xmlFreeTextReader(parser);
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
        throw Error("FileReader has no open file (please call 'XMLFileReader::open' first).");
    
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
                    // parse the sprite information
                    state = SPRITESETS;
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
            
            //  <meta> information
            // --------------------
            case META:
                // document main <title>
                if (xmlStrEqual(tag, STR_CAST("title")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    target.meta.title.clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        target.meta.title.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("Unexpected EOF (in file \"%s\")", filename);
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("title")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </title> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // document's <subtitle>
                else if (xmlStrEqual(tag, STR_CAST("subtitle")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    target.meta.subtitle.clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        target.meta.subtitle.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("Unexpected EOF (in file \"%s\")", filename);
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("subtitle")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </subtitle> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // <artist> name
                else if (xmlStrEqual(tag, STR_CAST("artist")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    target.meta.artist.clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        target.meta.artist.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("Unexpected EOF (in file \"%s\")", filename);
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("artist")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </artist> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // main <key> (or keys)
                else if (xmlStrEqual(tag, STR_CAST("key")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    target.meta.key.clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        target.meta.key.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("Unexpected EOF (in file \"%s\")", filename);
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("key")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </key> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // <date> of completion
                else if (xmlStrEqual(tag, STR_CAST("date")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    target.meta.date.clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        target.meta.date.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("Unexpected EOF (in file \"%s\")", filename);
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("date")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </date> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // <number> in a collection
                else if (xmlStrEqual(tag, STR_CAST("number")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    target.meta.number.clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        target.meta.number.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("Unexpected EOF (in file \"%s\")", filename);
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("number")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </number> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // <transcriptor> name
                else if (xmlStrEqual(tag, STR_CAST("transcriptor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    target.meta.transcriptor.clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        target.meta.transcriptor.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("Unexpected EOF (in file \"%s\")", filename);
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("transcriptor")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </transcriptor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // <opus> number
                else if (xmlStrEqual(tag, STR_CAST("opus")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    target.meta.opus.clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        target.meta.opus.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("Unexpected EOF (in file \"%s\")", filename);
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("opus")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </opus> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </meta>
                else if (xmlStrEqual(tag, STR_CAST("meta")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                    state = DOCUMENT;
                
                // any other tag will be inserted into the "misc" map
                else
                {
                    const std::string meta_tag(XML_CAST(tag));
                    target.meta.misc[meta_tag].clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        target.meta.misc[meta_tag].append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("Unexpected EOF (in file \"%s\")", filename);
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST(meta_tag.c_str())) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </%s> (in file \"%s\", at line %i:%i)", meta_tag, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
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
            mythrow("Expected EOF (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
        if (parser_return == -1)    // if there occured an XML-syntax error
            mythrow("XML-Syntax Error in description (in file \"%s\", near EOF)", filename);
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


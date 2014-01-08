
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

#include <cstdlib>              // atoi, strtof
#include <cstring>              // strlen, sprintf
#include <cmath>                // log10, sqrt
#include <libxml/xmlreader.h>   // xmlReaderForMemory, ...
#include <iostream>             // std::cout

#include "renderer.hh"          // Renderer, Sprites, std::string

#define STR_CAST(str)    reinterpret_cast<const xmlChar*>(str)
#define XML_CAST(xmlstr) reinterpret_cast<const char*>(xmlstr)

using namespace ScorePress;


//
//     class Renderer
//    ================
//
// This is an (partially) abstract renderer interface.
// It is used by the engine to render the prepared score, such that the engine
// is independent of the used frontend. Furthermore this class implements a
// parser for the sprite-file's meta-information, preparing a "Sprites"
// object.
//

// error class thrown on syntax errors within the sprites meta information
Renderer::Error::Error(const std::string& msg) : ScorePress::Error(msg) {}

// throwing function (general file error)
void Renderer::mythrow(const char* trns, const std::string& filename) throw(Error)
{
    char* msg = new char[strlen(trns) + filename.size() + 1];   // allocate memory
    sprintf(msg, trns, filename.c_str());                       // assemble message
    std::string s(msg);                                         // convert to string
    delete[] msg;                                               // delete buffer
    throw Error(s);                                             // throw message
}

// throwing function (symbol related error)
void Renderer::mythrow(const char* trns, const std::string& symbol, const std::string& filename, const int line, const int column) throw(Error)
{
    char* msg = new char[strlen(trns) + symbol.size() + filename.size() +   // allocate memory
                         static_cast<int>(log10(line) + log10(column)) + 3];
    sprintf(msg, trns, symbol.c_str(), filename.c_str(), line, column);     // assemble message
    std::string s(msg);                                 // convert to string
    delete[] msg;                                       // delete buffer
    throw Error(s);                                     // throw message
}

// throwing function (error with file position)
void Renderer::mythrow(const char* trns, const std::string& filename, const int line, const int column) throw(Error)
{
    char* msg = new char[strlen(trns) + filename.size() +   // allocate memory
                         static_cast<int>(log10(line) + log10(column)) + 3];
    sprintf(msg, trns, filename.c_str(), line, column);     // assemble message
    std::string s(msg);                                     // convert to string
    delete[] msg;                                           // delete buffer
    throw Error(s);                                         // throw message
}

// parser for the svg's meta-information (filename only used for error msgs)
void Renderer::parse(const char* spriteinfo, const std::string& _filename, const size_t setid) throw(Error)
{
    // get target structure
    SpriteSet& spriteset = sprites[setid];
    spriteset.clear();
    
    // strip the filename of its path (for the use in error messages)
    std::string filename = (_filename.rfind('/') != std::string::npos) ?
                    _filename.substr(_filename.rfind('/') + 1) :
                    _filename;
    
    // prepare parsing
    xmlTextReader* parser = NULL;   // parser instance
    int parser_return = 0;          // parser return value buffer
    
    // parse the given "spriteinfo" xml-structure
    try
    {
        // if we have no spriteinfo
        if (*spriteinfo == 0)
        {
            // try to open the XML-file which is named as the spriteset-file
            std::string infofile(_filename, 0, _filename.rfind('/') + 1);   // extract the full path
            if (filename.rfind('.') != std::string::npos)                   // append filename
                infofile.append(filename, 0, filename.rfind('.'));          //    without extension
            else
                infofile.append(filename);
            infofile.insert(0, "file://");                                  // insert protocol
            infofile.append(".xml");                                        // append new extension
            
            // create parser
            parser = xmlReaderForFile(infofile.c_str(), NULL, XML_PARSE_NOBLANKS | XML_PARSE_NOENT | XML_PARSE_NONET);
            
            // report failed parser instanciation
            if (parser == NULL) mythrow("Unable to open file \"%s\"", infofile.substr(7));
        }
        
        // if the spriteinfo is external
        else if (strncmp(spriteinfo, "file://", 7) == 0)
        {
            // try to open the external XML-file
            std::string infofile(spriteinfo);
            if (spriteinfo[7] != '/' && _filename.rfind('/') != std::string::npos)  // interpret relative path
                infofile.insert(7, _filename, 0, _filename.rfind('/') + 1);
            
            // create parser
            parser = xmlReaderForFile(infofile.c_str(), NULL, XML_PARSE_NOBLANKS | XML_PARSE_NOENT | XML_PARSE_NONET);
            
            // report failed parser instanciation
            if (parser == NULL) mythrow("Unable to open file \"%s\"", infofile.substr(7));
        }
        
        // if the spriteinfo is within the description
        else
        {
            // create parser for the description itself
            parser = xmlReaderForMemory(spriteinfo,             // xml-source
                            strlen(spriteinfo),                 // data length
                            ("file://" + _filename).c_str(),    // base URL
                            NULL,                               // no encoding
                            XML_PARSE_NOBLANKS | XML_PARSE_NOENT | XML_PARSE_NONET
                        );
            
            // report failed parser instanciation
            if (parser == NULL) mythrow("Unable to parse description (in file \"%s\")", filename);
        };
        
        // parser state enumerator
        enum enuState
        {   BEGIN, END,
            SYMBOLS,
            INFO, SPRITES,
            BASE,
            HEAD, FLAG, REST, RESTFLAG, ACCIDENTAL, BRACE, BRACKET, DOT, ARTICULATION, CLEF, TIMESIG1, TIMESIG2,
            ANCHOR, STEM, KEYBOUND, RESTSTEM, RESTSTEMTOPPOS, RESTSTEMBOTTOMPOS,
            MOVABLE
        };
        enuState state = BEGIN;     // set current state
        enuState prestate = BEGIN;  // set the previous state (used for returning from a state)
        
        const xmlChar* tag = NULL;  // tag string
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
                // expecting <symbols> to be the root tag
                if (xmlStrEqual(tag, STR_CAST("symbols")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_ELEMENT) mythrow("Expected tag <symbols> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                
                // checking "id" attribute
                attr = xmlTextReaderGetAttribute(parser, STR_CAST("id"));
                if (attr == NULL) mythrow("Missing \"id\"-attribute for <symbols>-tag (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                spriteset.info["id"] = XML_CAST(attr);
                xmlFree(attr);
                
                // changing state
                state = SYMBOLS;
                break;
                
            //  state after the final </symbols>, expecting the end of data (should not occur)
            // ----------------------------------
            case END:
                mythrow("Expected EOF (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                break;
                
            //  <symbols> root section
            // ------------------------
            case SYMBOLS:
                // the libraries <info> section
                if (xmlStrEqual(tag, STR_CAST("info")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    state = INFO;   // parse the section
                }
                
                // the <sprites> section
                else if (xmlStrEqual(tag, STR_CAST("sprites")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // get "unit" attribute (i.e. head-height)
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("unit"));
                    if (attr == NULL) mythrow("Missing \"unit\"-attribute for <sprites> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.head_height = atoi(XML_CAST(attr));
                    xmlFree(attr);
                    
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
                
            //  <info> structure
            // ------------------
            case INFO:
                // <author> information
                if (xmlStrEqual(tag, STR_CAST("author")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    spriteset.info["author"].clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.info["author"].append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("Unexpected EOF (in file \"%s\")", filename);
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("author")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </author> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // <copyright> information
                else if (xmlStrEqual(tag, STR_CAST("copyright")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    spriteset.info["copyright"].clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.info["copyright"].append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("copyright")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </copyright> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // <license> information
                else if (xmlStrEqual(tag, STR_CAST("license")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    spriteset.info["license"].clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.info["license"].append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("license")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </license> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // <description> string
                else if (xmlStrEqual(tag, STR_CAST("description")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    spriteset.info["description"].clear();
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.info["description"].append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("description")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </description> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </info>
                else if (xmlStrEqual(tag, STR_CAST("info")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                    state = SYMBOLS;
                
                // any other tag is illegal here
                else mythrow("Expected one of <author>, <copyright>, <license>, <description> or </info> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                break;
                
            //  <sprites> section
            // -------------------
            case SPRITES:
                // <base> sprites
                if (xmlStrEqual(tag, STR_CAST("base")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                    state = BASE;
                
                // user defined <movable> sprites
                else if (xmlStrEqual(tag, STR_CAST("movable")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                    state = MOVABLE;
                
                // end tag </sprites>
                else if (xmlStrEqual(tag, STR_CAST("sprites")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                    state = SYMBOLS;
                
                // any other tag is illegal here
                else mythrow("Expected one of <base> or </sprites> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                break;
                
            //  <base> sprites section
            // ------------------------
            case BASE:
                // <head> sprite
                if (xmlStrEqual(tag, STR_CAST("head")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check type and prepare section parser (i.e. state change)
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                    if (attr == NULL) mythrow("Missing \"type\"-attribute for <head> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    if (xmlStrEqual(attr, STR_CAST("longa")) == 1)        // longa head
                    {
                        spriteset.heads_longa = spriteset.size();
                        spriteset.ids["base.heads.longa"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::HEADS_LONGA));
                        spriteset.back().text[" class "] = "base.heads.longa";
                        state = HEAD;
                    }
                    else if (xmlStrEqual(attr, STR_CAST("breve")) == 1)   // breve head
                    {
                        spriteset.heads_breve = spriteset.size();
                        spriteset.ids["base.heads.breve"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::HEADS_BREVE));
                        spriteset.back().text[" class "] = "base.heads.breve";
                        state = HEAD;
                    }
                    else if (xmlStrEqual(attr, STR_CAST("whole")) == 1)   // whole head
                    {
                        spriteset.heads_whole = spriteset.size();
                        spriteset.ids["base.heads.whole"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::HEADS_WHOLE));
                        spriteset.back().text[" class "] = "base.heads.whole";
                        state = HEAD;
                    }
                    else if (xmlStrEqual(attr, STR_CAST("half")) == 1)    // half head
                    {
                        spriteset.heads_half = spriteset.size();
                        spriteset.ids["base.heads.half"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::HEADS_HALF));
                        spriteset.back().text[" class "] = "base.heads.half";
                        state = HEAD;
                    }
                    else if (xmlStrEqual(attr, STR_CAST("quarter")) == 1) // quarter head
                    {
                        spriteset.heads_quarter = spriteset.size();
                        spriteset.ids["base.heads.quarter"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::HEADS_QUARTER));
                        spriteset.back().text[" class "] = "base.heads.quarter";
                        state = HEAD;
                    }
                    else    // illegal head type (throw error message)
                    {
                        xmlFree(attr);  // free attribute buffer
                        mythrow("Illegal value of \"type\"-attribute for <head> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // free attribute buffer
                    xmlFree(attr);
                }
                
                // <flag> sprite
                else if (xmlStrEqual(tag, STR_CAST("flag")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check type and prepare section parser (i.e. state change)
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                    if (attr == NULL) mythrow("Missing \"type\"-attribute for <flag> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    if (xmlStrEqual(attr, STR_CAST("note")) == 1)       // note flag
                    {
                        spriteset.flags_note = spriteset.size();
                        spriteset.ids["base.flags.note"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::FLAGS_NOTE));
                        spriteset.back().text[" class "] = "base.flags.note";
                        state = FLAG;
                    }
                    else if (xmlStrEqual(attr, STR_CAST("rest")) == 1)  // rest flag
                    {
                        spriteset.flags_rest = spriteset.size();
                        spriteset.ids["base.flags.rest"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::FLAGS_REST));
                        spriteset.back().text[" class "] = "base.flags.rest";
                        state = RESTFLAG;
                    }
                    else    // illegal flag type (throw error message)
                    {
                        xmlFree(attr);  // free attribute buffer
                        mythrow("Illegal value of \"type\"-attribute for <rest> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // free attribute buffer
                    xmlFree(attr);
                }
                
                // <rest> sprite
                else if (xmlStrEqual(tag, STR_CAST("rest")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check type and prepare section parser (i.e. state change)
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                    if (attr == NULL) mythrow("Missing \"type\"-attribute for <rest> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (xmlStrEqual(attr, STR_CAST("longa")) == 1)        // longa rest
                    {
                        spriteset.rests_longa = spriteset.size();
                        spriteset.ids["base.rests.longa"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::RESTS_LONGA));
                        spriteset.back().text[" class "] = "base.rests.longa";
                        state = REST;
                    }
                    else if (xmlStrEqual(attr, STR_CAST("breve")) == 1)   // breve rest
                    {
                        spriteset.rests_breve = spriteset.size();
                        spriteset.ids["base.rests.breve"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::RESTS_BREVE));
                        spriteset.back().text[" class "] = "base.rests.breve";
                        state = REST;
                    }
                    else if (xmlStrEqual(attr, STR_CAST("whole")) == 1)   // whole rest
                    {
                        spriteset.rests_whole = spriteset.size();
                        spriteset.ids["base.rests.whole"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::RESTS_WHOLE));
                        spriteset.back().text[" class "] = "base.rests.whole";
                        state = REST;
                    }
                    else if (xmlStrEqual(attr, STR_CAST("half")) == 1)    // half rest
                    {
                        spriteset.rests_half = spriteset.size();
                        spriteset.ids["base.rests.half"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::RESTS_HALF));
                        spriteset.back().text[" class "] = "base.rests.half";
                        state = REST;
                    }
                    else if (xmlStrEqual(attr, STR_CAST("quarter")) == 1)  // quarter rest
                    {
                        spriteset.rests_quarter = spriteset.size();
                        spriteset.ids["base.rests.quarter"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::RESTS_QUARTER));
                        spriteset.back().text[" class "] = "base.rests.quarter";
                        state = REST;
                    }
                    else    // illegal rest type (throw error message)
                    {
                        xmlFree(attr);  // free attribute buffer
                        mythrow("Illegal value of \"type\"-attribute for <rest> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // free attribute buffer
                    xmlFree(attr);
                }
                
                // <accidental> sprite
                else if (xmlStrEqual(tag, STR_CAST("accidental")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check type and prepare section parser (i.e. state change)
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                    if (attr == NULL) mythrow("Missing \"type\"-attribute for <accidental> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (xmlStrEqual(attr, STR_CAST("double-sharp")) == 1)         // double sharp
                    {
                        spriteset.accidentals_double_sharp = spriteset.size();
                        spriteset.ids["base.accidentals.double-sharp"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_DOUBLESHARP));
                        spriteset.back().text[" class "] = "base.accidentals.double-sharp";
                    }
                    else if (xmlStrEqual(attr, STR_CAST("sharp-andahalf")) == 1)  // 1 1/2 sharp
                    {
                        spriteset.accidentals_sharp_andahalf = spriteset.size();
                        spriteset.ids["base.accidentals.sharp-andahalf"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_SHARPANDAHALF));
                        spriteset.back().text[" class "] = "base.accidentals.sharp-andahalf";
                    }
                    else if (xmlStrEqual(attr, STR_CAST("sharp")) == 1)           // sharp
                    {
                        spriteset.accidentals_sharp = spriteset.size();
                        spriteset.ids["base.accidentals.sharp"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_SHARP));
                        spriteset.back().text[" class "] = "base.accidentals.sharp";
                    }
                    else if (xmlStrEqual(attr, STR_CAST("half-sharp")) == 1)      // half sharp
                    {
                        spriteset.accidentals_half_sharp = spriteset.size();
                        spriteset.ids["base.accidentals.half-sharp"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_HALFSHARP));
                        spriteset.back().text[" class "] = "base.accidentals.half-sharp";
                    }
                    else if (xmlStrEqual(attr, STR_CAST("natural")) == 1)         // natural
                    {
                        spriteset.accidentals_natural = spriteset.size();
                        spriteset.ids["base.accidentals.natural"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_NATURAL));
                        spriteset.back().text[" class "] = "base.accidentals.natural";
                    }
                    else if (xmlStrEqual(attr, STR_CAST("half-flat")) == 1)       // half flat
                    {
                        spriteset.accidentals_half_flat = spriteset.size();
                        spriteset.ids["base.accidentals.half-flat"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_HALFFLAT));
                        spriteset.back().text[" class "] = "base.accidentals.half-flat";
                    }
                    else if (xmlStrEqual(attr, STR_CAST("flat")) == 1)            // flat
                    {
                        spriteset.accidentals_flat = spriteset.size();
                        spriteset.ids["base.accidentals.flat"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_FLAT));
                        spriteset.back().text[" class "] = "base.accidentals.flat";
                    }
                    else if (xmlStrEqual(attr, STR_CAST("flat-andahalf")) == 1)   // 1 1/2 flat
                    {
                        spriteset.accidentals_flat_andahalf = spriteset.size();
                        spriteset.ids["base.accidentals.flat-andahalf"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_FLATANDAHALF));
                        spriteset.back().text[" class "] = "base.accidentals.flat-andahalf";
                    }
                    else if (xmlStrEqual(attr, STR_CAST("double-flat")) == 1)     // double flat
                    {
                        spriteset.accidentals_double_flat = spriteset.size();
                        spriteset.ids["base.accidentals.double-flat"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::ACCIDENTALS_DOUBLEFLAT));
                        spriteset.back().text[" class "] = "base.accidentals.double-flat";
                    }
                    else    // illegal accidental type (throw error message)
                    {
                        xmlFree(attr);  // free attribute buffer
                        mythrow("Illegal value of \"type\"-attribute for <accidental> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // free attribute buffer
                    xmlFree(attr);
                    
                    // goto <accidental> section parser
                    state = ACCIDENTAL;
                }
                
                // <brace> sprite
                else if (xmlStrEqual(tag, STR_CAST("brace")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    spriteset.brace = spriteset.size();
                    spriteset.ids["base.brace"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::BRACE));     // create sprite
                    spriteset.back().text[" class "] = "base.brace";        // set class
                    state = BRACE;
                }
                
                // <bracket> sprite
                else if (xmlStrEqual(tag, STR_CAST("bracket")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    spriteset.bracket = spriteset.size();
                    spriteset.ids["base.bracket"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::BRACKET));   // create sprite
                    spriteset.back().text[" class "] = "base.bracket";      // set class
                    state = BRACKET;
                }
                
                // <dot> symbol
                else if (xmlStrEqual(tag, STR_CAST("dot")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    spriteset.dot = spriteset.size();
                    spriteset.ids["base.dot"] = spriteset.size();
                    spriteset.push_back(SpriteInfo(SpriteInfo::DOT));       // create sprite
                    spriteset.back().text[" class "] = "base.dot";          // set class
                    state = DOT;        // goto <articulation> section parser
                }
                
                // <articulation> symbol
                else if (xmlStrEqual(tag, STR_CAST("articulation")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check type attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                    if (attr == NULL) mythrow("Missing \"type\"-attribute for <articulation> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.push_back(SpriteInfo(SpriteInfo::CLEF));      // create sprite
                    spriteset.back().text[" class "] = "articulation.";     // set class
                    spriteset.back().text[" class "].append(XML_CAST(attr));
                    spriteset.ids[spriteset.back().text[" class "]] = spriteset.size() - 1;
                    xmlFree(attr);          // free attribute buffer
                    state = ARTICULATION;   // goto <articulation> section parser
                }
                
                // <clef> sprite
                else if (xmlStrEqual(tag, STR_CAST("clef")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check type attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                    if (attr == NULL) mythrow("Missing \"type\"-attribute for <clef> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.push_back(SpriteInfo(SpriteInfo::CLEF));      // create sprite
                    spriteset.back().text[" class "] = "clef.";             // set class
                    spriteset.back().text[" class "].append(XML_CAST(attr));
                    xmlFree(attr);          // free attribute buffer
                    state = CLEF;           // goto <clef> section parser
                }
                
                // <timesig> sprite
                else if (xmlStrEqual(tag, STR_CAST("timesig")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check type attribute
                    attr = xmlTextReaderGetAttribute(parser, STR_CAST("type"));
                    if (attr == NULL) mythrow("Missing \"type\"-attribute for <timesig> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    if (xmlStrEqual(attr, STR_CAST("digit")) == 1)  // check type
                    {
                        state = TIMESIG1;   // and set next state appropriately
                    }
                    else if (xmlStrEqual(attr, STR_CAST("symbol")) == 1)
                    {
                        state = TIMESIG2;
                    }
                    else
                    {
                        mythrow("Illegal value of \"type\"-attribute for <timesig> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // create class name ("ids"-entry set afterwards)
                    spriteset.push_back(SpriteInfo((state == TIMESIG1) ? SpriteInfo::DIGITS_TIME : SpriteInfo::TIMESIG));    // create sprite
                    spriteset.back().text[" class "] = "timesig.";  // set class
                    spriteset.back().text[" class "].append(XML_CAST(attr));
                    
                    xmlFree(attr);          // free attribute buffer
                }
                
                // end tag </base>
                else if (xmlStrEqual(tag, STR_CAST("base")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    state = SPRITES;
                }
                
                // any other tag is illegal here
                else mythrow("Expected one of <head>, <flag>, <rest>, <accidental>, <clef>, <timesig> or </base> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                break;
                
            //  <head> section parser
            // -----------------------
            case HEAD:
                // the head's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i)", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the <anchor> position structure
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = HEAD;    // prepare jump back
                    state = ANCHOR;     // jump to anchor parser
                }
                
                // the <stem> position structure
                else if (xmlStrEqual(tag, STR_CAST("stem")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT) 
                {
                    if (spriteset.back().real.find("stem.x") != spriteset.back().real.end()) mythrow("Unexpected second <stem> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = HEAD;    // prepare jump back
                    state = STEM;       // jump to stem parser
                }
                
                // end tag </head>
                else if (xmlStrEqual(tag, STR_CAST("head")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    // check for path
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = BASE;       // return
                }
                
                // any other tag is illegal here
                else
                {
                    // request <path>
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // request <anchor> or end tag </head>
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end())
                    {
                        // if <stem> is not present, include in message
                        if (spriteset.back().real.find("stem.x") == spriteset.back().real.end()) mythrow("Expected <anchor>, <stem> or </head> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected <anchor> or </head> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // if <stem> is not present, include in message
                    if (spriteset.back().real.find("stem.x") == spriteset.back().real.end()) mythrow("Expected <stem> or </head> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </head> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <flag> section parser (for notes)
            // -----------------------------------
            case FLAG:
                // the flag's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i)", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the path for <overlay> flags
                else if (xmlStrEqual(tag, STR_CAST("overlay")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read overlay-path
                    if (spriteset.back().text.find("overlay") != spriteset.back().text.end()) mythrow("Unexpected second <overlay> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().text["overlay"].clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().text["overlay"].append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </overlay>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("overlay")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </overlay> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().text["overlay"].empty()) mythrow("Expected text-content for <overlay> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().text["overlay"], setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().text["overlay"], filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // flag <distance> parser
                else if (xmlStrEqual(tag, STR_CAST("distance")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read distance and empty tag
                    if (spriteset.back().real.find("distance") != spriteset.back().real.end()) mythrow("Unexpected <distance> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;     // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["distance"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </distance>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("distance")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </distance> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the flag's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = FLAG;    // prepare jump back
                    state = ANCHOR;     // jump to anchor parser
                }
                
                // end tag </flag>
                else if (xmlStrEqual(tag, STR_CAST("flag")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    // check for <path>
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // prepare <overlay> sprite
                    if (spriteset.back().text.find("overlay") != spriteset.back().text.end())
                    {
                        spriteset.flags_overlay = spriteset.size();                 // save id
                        spriteset.ids["base.flag.overlay"] = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::FLAGS_OVERLAY)); // add sprite
                        spriteset.back().text[" class "] = "base.flag.overlay";
                        spriteset.back().path = spriteset[spriteset.size() - 2].text["overlay"];
                    }
                    // if there is no overlay sprite, use the same sprite as for the flag
                    else spriteset.flags_overlay = spriteset.size() - 1;
                    
                    // return to <base> section
                    state = BASE;
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end())
                    {
                        if (spriteset.back().text.find("overlay") == spriteset.back().text.end()) mythrow("Expected <anchor>, <overlay> or </flag> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected <anchor> or </flag> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    if (spriteset.back().text.find("overlay") == spriteset.back().text.end()) mythrow("Expected <overlay> or </flag> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </flag> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <rest> section parser
            // -----------------------
            case REST:
                // the rest's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i)", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the rest's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = REST;    // prepare jump back
                    state = ANCHOR;     // jump to anchor parser
                }
                
                // default rest <line> parser
                else if (xmlStrEqual(tag, STR_CAST("line")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read <line> and empty tag
                    if (spriteset.back().real.find("line") != spriteset.back().real.end()) mythrow("Unexpected <line> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["line"] = atoi(buffer.c_str());
                    
                    // check for end tag </line>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("line")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </line> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </rest>
                else if (xmlStrEqual(tag, STR_CAST("rest")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    // check for <path>
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = BASE;   // return to <base> section
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end())
                    {
                        if (spriteset.back().integer.find("line") == spriteset.back().integer.end()) mythrow("Expected <anchor>, <line> or </rest> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected <anchor> or </rest> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    if (spriteset.back().integer.find("line") == spriteset.back().integer.end()) mythrow("Expected <line> or </rest> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </rest> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <flag> section parser (for rests)
            // -----------------------------------
            case RESTFLAG:
                // the flag's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the short rest's <restbase> piece
                else if (xmlStrEqual(tag, STR_CAST("restbase")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (spriteset.back().text.find("restbase") != spriteset.back().text.end()) mythrow("Unexpected second <restbase> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().text["restbase"].clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().text["restbase"].append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </restbase>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("restbase")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </restbase> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().text["restbase"].empty()) mythrow("Expected text-content for <restbase> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().text["restbase"], setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().text["bottom"], filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the flag's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = RESTFLAG;    // prepare jump back
                    state = ANCHOR;         // jump to anchor parser
                }
                
                // default rest <line> parser
                else if (xmlStrEqual(tag, STR_CAST("line")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read <line> and empty tag
                    if (spriteset.back().real.find("line") != spriteset.back().real.end()) mythrow("Unexpected <line> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["line"] = atoi(buffer.c_str());
                    
                    // check for end tag </line>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("line")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </line> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // parse the rest's <stem>-information
                else if (xmlStrEqual(tag, STR_CAST("stem")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("stem.top.x1") != spriteset.back().real.end()) mythrow("Unexpected second <stem> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = RESTSTEM;   // jump to stem parser
                }   // (this parser automatically jumps back here)
                
                // stem <slope> section
                else if (xmlStrEqual(tag, STR_CAST("slope")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read <slope> and empty tag
                    if (spriteset.back().real.find("slope") != spriteset.back().real.end()) mythrow("Unexpected <slope> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["slope"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </slope>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("slope")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </slope> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </flag>
                else if (xmlStrEqual(tag, STR_CAST("flag")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    // check for <path>
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // create sprite-info for the base-path
                    if (spriteset.back().text.find("restbase") != spriteset.back().text.end())
                    {
                        spriteset.flags_base = spriteset.size();
                        spriteset.push_back(SpriteInfo(SpriteInfo::FLAGS_BASE));
                        spriteset.back().text[" class "] = "base.flag.restbase";
                        spriteset.back().path = spriteset[spriteset.size() - 2].text["restbase"];
                        spriteset[spriteset.size() - 2].text.erase("restbase");
                    };
                    
                    // return to <base> section
                    state = BASE;
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end())
                    {
                        if (spriteset.back().integer.find("line") == spriteset.back().integer.end()) mythrow("Expected <anchor>, <line> or </flag> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected <anchor> or </flag> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    if (spriteset.back().integer.find("line") == spriteset.back().integer.end()) mythrow("Expected <line> or </flag> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </flag> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <accidental> section parser
            // -----------------------------
            case ACCIDENTAL:
                // the accidental's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the accidental's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = ACCIDENTAL;  // prepare jump back
                    state = ANCHOR;         // jump to anchor parser
                }
                
                // the default y-<offset>
                else if (xmlStrEqual(tag, STR_CAST("offset")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read offset
                    if (spriteset.back().real.find("offset") != spriteset.back().real.end()) mythrow("Unexpected second <offset> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["offset"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </offset>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("offset")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </offset> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </accidental>
                else if (xmlStrEqual(tag, STR_CAST("accidental")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = BASE;
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end()) mythrow("Expected <anchor> or </accidental> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </accidental> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <brace> section parser
            // ----------------------
            case BRACE:
                // the brace's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the brace's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = BRACE;       // prepare jump back
                    state = ANCHOR;         // jump to anchor parser
                }
                
                // the brace's minimal height <hmin>
                else if (xmlStrEqual(tag, STR_CAST("hmin")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read minimal height
                    if (spriteset.back().real.find("hmin") != spriteset.back().real.end()) mythrow("Unexpected second <hmin> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["hmin"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </hmin>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("hmin")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </hmin> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the brace's maximal height <hmax>
                else if (xmlStrEqual(tag, STR_CAST("hmax")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read maximal height
                    if (spriteset.back().real.find("hmax") != spriteset.back().real.end()) mythrow("Unexpected second <hmax> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["hmax"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </hmax>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("hmax")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </hmax> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the brace's <low> coefficient
                else if (xmlStrEqual(tag, STR_CAST("low")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read maximal height
                    if (spriteset.back().real.find("low") != spriteset.back().real.end()) mythrow("Unexpected second <low> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["low"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </low>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("low")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </low> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the brace's <high> coefficient
                else if (xmlStrEqual(tag, STR_CAST("high")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read maximal height
                    if (spriteset.back().real.find("high") != spriteset.back().real.end()) mythrow("Unexpected second <high> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["high"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </high>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("high")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </high> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </brace>
                else if (xmlStrEqual(tag, STR_CAST("brace")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = BASE;
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end()) mythrow("Expected <anchor>, <scalerange> or </brace> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected <scalerange> or </brace> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <bracket> section parser
            // ----------------------
            case BRACKET:
                // the bracket's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the bracket's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = BRACKET; // prepare jump back
                    state = ANCHOR;     // jump to anchor parser
                }
                
                // the bracket's <line> width
                else if (xmlStrEqual(tag, STR_CAST("line")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read line width
                    if (spriteset.back().real.find("line") != spriteset.back().real.end()) mythrow("Unexpected second <line> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["line"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </line>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("line")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </line> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </bracket>
                else if (xmlStrEqual(tag, STR_CAST("bracket")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = BASE;
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end())
                    {
                        if (spriteset.back().real.find("line") == spriteset.back().real.end())
                            mythrow("Expected one of <anchor>, <line> or </bracket> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected one of <anchor> or </bracket> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    if (spriteset.back().real.find("line") == spriteset.back().real.end())
                    {
                        mythrow("Expected <line> or </bracket> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    mythrow("Expected </bracket> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <dot> section parser
            // ----------------------
            case DOT:
                // the dot's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the dot's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = DOT;     // prepare jump back
                    state = ANCHOR;     // jump to anchor parser
                }
                
                // the <distance> between dots
                else if (xmlStrEqual(tag, STR_CAST("distance")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read distance
                    if (spriteset.back().real.find("distance") != spriteset.back().real.end()) mythrow("Unexpected second <distance> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["distance"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </distance>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("distance")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </distance> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the default <offset> from the head
                else if (xmlStrEqual(tag, STR_CAST("offset")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read offset
                    if (spriteset.back().real.find("offset") != spriteset.back().real.end()) mythrow("Unexpected second <offset> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["offset"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </offset>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("offset")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </offset> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </dot>
                else if (xmlStrEqual(tag, STR_CAST("dot")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = BASE;
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end())
                    {
                        if (spriteset.back().real.find("distance") == spriteset.back().real.end())
                        {
                            if (spriteset.back().real.find("offset") == spriteset.back().real.end())
                                mythrow("Expected one of <anchor>, <distance>, <offset> or </dot> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                            mythrow("Expected one of <anchor>, <distance> or </dot> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        };
                        if (spriteset.back().real.find("offset") == spriteset.back().real.end())
                            mythrow("Expected one of <anchor>, <offset> or </dot> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected <anchor> or </dot> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    if (spriteset.back().real.find("distance") == spriteset.back().real.end())
                    {
                        if (spriteset.back().real.find("offset") == spriteset.back().real.end())
                            mythrow("Expected one of <distance>, <offset> or </dot> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected <distance> or </dot> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    if (spriteset.back().real.find("offset") == spriteset.back().real.end())
                        mythrow("Expected <offset> or </dot> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </dot> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
            
            //  <articulation> section parser
            // -------------------------------
            case ARTICULATION:
                // the articulation symbol's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the articulation symbol's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = ARTICULATION;    // prepare jump back
                    state = ANCHOR;             // jump to anchor parser
                }
                
                // the default y-<offset>
                else if (xmlStrEqual(tag, STR_CAST("offset")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read offset
                    if (spriteset.back().real.find("offset") != spriteset.back().real.end()) mythrow("Unexpected second <offset> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["offset"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </offset>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("offset")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </offset> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the articulation symbol's <valuemod>-ificator
                else if (xmlStrEqual(tag, STR_CAST("valuemod")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read basenote
                    if (spriteset.back().integer.find("valuemod") != spriteset.back().integer.end()) mythrow("Unexpected second <valuemod> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["valuemod"] = atoi(buffer.c_str());
                    
                    // check for end tag </valuemod>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("valuemod")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </valuemod> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the articulation symbol's <volumemod>-ificator
                else if (xmlStrEqual(tag, STR_CAST("volumemod")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read basenote
                    if (spriteset.back().integer.find("volumemod") != spriteset.back().integer.end()) mythrow("Unexpected second <volumemod> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["volumemod"] = atoi(buffer.c_str());
                    
                    // check for end tag </valuemod>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("volumemod")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </volumemod> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </articulation>
                else if (xmlStrEqual(tag, STR_CAST("articulation")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = BASE;
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end())
                    {
                        if (spriteset.back().real.find("offset") == spriteset.back().real.end()) mythrow("Expected <anchor>, <offset> or </articulation> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected <anchor> or </articulation> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    }
                    if (spriteset.back().real.find("offset") == spriteset.back().real.end()) mythrow("Expected <offset> or </articulation> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </articulation> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
            
            //  <clef> section parser
            // -----------------------
            case CLEF:
                // the accidental's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the clef's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = CLEF;    // prepare jump back
                    state = ANCHOR;     // jump to anchor parser
                }
                
                // the <basenote> is the note, which lies on the line through the anchor-point
                else if (xmlStrEqual(tag, STR_CAST("basenote")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read basenote
                    if (spriteset.back().integer.find("basenote") != spriteset.back().integer.end()) mythrow("Unexpected second <basenote> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["basenote"] = atoi(buffer.c_str());
                    
                    // check for end tag </basenote>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("basenote")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </basenote> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // get the default <line> for the clef
                else if (xmlStrEqual(tag, STR_CAST("line")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read line
                    if (spriteset.back().real.find("line") != spriteset.back().real.end()) mythrow("Unexpected second <line> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["line"] = atoi(buffer.c_str());
                    
                    // check for end tag </line>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("line")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </line> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // parse key-signature boundaries within the tag <keybound>
                else if (xmlStrEqual(tag, STR_CAST("keybound")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().integer.find("keybound.sharp") != spriteset.back().integer.end()) mythrow("Unexpected second <keybound> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = KEYBOUND;   // jump to keybound parser
                }
                
                // end tag </clef>
                else if (xmlStrEqual(tag, STR_CAST("clef")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.ids[spriteset.back().text[" class "]] = spriteset.size() - 1;
                    state = BASE;
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end())
                    {
                        if (spriteset.back().integer.find("basenote") == spriteset.back().integer.end())
                        {
                            if (spriteset.back().integer.find("line") == spriteset.back().integer.end()) mythrow("Expected <anchor>, <basenote>, <line> or </clef> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                            mythrow("Expected <anchor>, <basenote> or </clef> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        };
                        if (spriteset.back().integer.find("line") == spriteset.back().integer.end()) mythrow("Expected <anchor>, <line> or </clef> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected <anchor> or </clef> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    if (spriteset.back().integer.find("basenote") == spriteset.back().integer.end())
                    {
                        if (spriteset.back().integer.find("line") == spriteset.back().integer.end()) mythrow("Expected <basenote>, <line> or </clef> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected <basenote> or </clef> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    if (spriteset.back().integer.find("line") == spriteset.back().integer.end()) mythrow("Expected <line> or </clef> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </clef> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <timesig> section parser (digit-type)
            // ---------------------------------------
            case TIMESIG1:
                // the time-signature's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // get the <digit> represented by this sprite
                else if (xmlStrEqual(tag, STR_CAST("digit")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read digit
                    if (spriteset.back().integer.find("digit") != spriteset.back().integer.end()) mythrow("Unexpected second <digit> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["digit"] = atoi(buffer.c_str());
                    
                    // check range
                    if (spriteset.back().integer["digit"] < 0 || spriteset.back().integer["digit"] > 9) mythrow("Value for <digit> out of range 0-9 (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // check for end tag </digit>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("digit")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </digit> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the digit's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = TIMESIG1;    // prepare jump back
                    state = ANCHOR;         // jump to anchor parser
                }
                
                // end tag </timesig>
                else if (xmlStrEqual(tag, STR_CAST("timesig")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().integer.find("digit") == spriteset.back().integer.end()) mythrow("Expected <digit> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().text[" class "].push_back(static_cast<char>(spriteset.back().integer["digit"] + 0x30));
                    spriteset.ids[spriteset.back().text[" class "]] = spriteset.size() - 1;
                    spriteset.digits_time[spriteset.back().integer["digit"]] = spriteset.size() - 1; 
                    state = BASE;
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().integer.find("digit") == spriteset.back().integer.end()) mythrow("Expected <digit> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end()) mythrow("Expected <anchor> or </timesig> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </timesig> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <timesig> section parser (symbol-type)
            // ----------------------------------------
            case TIMESIG2:
                // the time-signature's <path> name
                if (xmlStrEqual(tag, STR_CAST("path")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read path
                    if (!spriteset.back().path.empty()) mythrow("Unexpected second <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    spriteset.back().path.clear();
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read content (text node)
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        spriteset.back().path.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // check for end tag </path>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("path")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().path.empty()) mythrow("Expected text-content for <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (!exist(spriteset.back().path, setid)) mythrow("Unable to find path \"%s\" (requested in file \"%s\", at line %i:%i))", spriteset.back().path, filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // get the <enum> of the symbol's default time-signature
                else if (xmlStrEqual(tag, STR_CAST("enum")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read enumerator
                    if (spriteset.back().integer.find("enum") != spriteset.back().integer.end()) mythrow("Unexpected second <enum> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["enum"] = atoi(buffer.c_str());
                    
                    // check for end tag </enum>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("enum")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </enum> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // get the <denom> of the symbol's default time-signature
                else if (xmlStrEqual(tag, STR_CAST("denom")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check for already read enumerator
                    if (spriteset.back().integer.find("denom") != spriteset.back().integer.end()) mythrow("Unexpected second <denom> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["denom"] = atoi(buffer.c_str());
                    
                    // check for end tag </enum>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("denom")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </denom> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // the symbol's <anchor> point
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().real.find("anchor.x") != spriteset.back().real.end()) mythrow("Unexpected second <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    prestate = TIMESIG2;    // prepare jump back
                    state = ANCHOR;         // jump to anchor parser
                }
                
                // end tag </timesig>
                else if (xmlStrEqual(tag, STR_CAST("timesig")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    // check for neccessary data
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().integer.find("enum") == spriteset.back().integer.end())
                    {
                        if (spriteset.back().integer.find("denom") == spriteset.back().integer.end())
                        {
                            mythrow("Expected <enum> or <denom> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        };
                        
                        mythrow("Expected <enum> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    }
                    else if (spriteset.back().integer.find("denom") == spriteset.back().integer.end())
                    {
                        mythrow("Expected <denom> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // create class-name
                    char* buffer = new char[static_cast<int>(log10(spriteset.back().integer["enum"])) + 2];
                    sprintf(buffer, "%i", spriteset.back().integer["enum"]);
                    spriteset.back().text[" class "].append(buffer);
                    delete[] buffer;
                    spriteset.back().text[" class "] += "_";
                    buffer = new char[static_cast<int>(log10(spriteset.back().integer["denom"])) + 2];
                    sprintf(buffer, "%i", spriteset.back().integer["denom"]);
                    spriteset.back().text[" class "].append(buffer);
                    delete[] buffer;
                    spriteset.back().text[" class "] += "_" + spriteset.back().path;
                    spriteset.ids[spriteset.back().text[" class "]] = spriteset.size() - 1;
                    
                    // return to BASE-state
                    state = BASE;
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().path.empty()) mythrow("Expected <path> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end())
                    {
                        if (spriteset.back().integer.find("enum") == spriteset.back().integer.end())
                        {
                            if (spriteset.back().integer.find("denom") == spriteset.back().integer.end())
                            {
                                mythrow("Expected <enum>, <denom> or <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                            };
                        
                            mythrow("Expected <enum> or <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        }
                        else if (spriteset.back().integer.find("denom") == spriteset.back().integer.end())
                        {
                            mythrow("Expected <denom> or <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        };
                        mythrow("Expected <anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    if (spriteset.back().integer.find("enum") == spriteset.back().integer.end())
                    {
                        if (spriteset.back().integer.find("denom") == spriteset.back().integer.end())
                        {
                            mythrow("Expected <enum> or <denom> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        };
                    
                        mythrow("Expected <enum> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    }
                    else if (spriteset.back().integer.find("denom") == spriteset.back().integer.end())
                    {
                        mythrow("Expected <denom> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    mythrow("Expected </timesig> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  position parser for <anchor> and <stem> sections
            // --------------------------------------------------
            case ANCHOR:
            case STEM:
                // position's X-coordinate
                if (xmlStrEqual(tag, STR_CAST("x")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check if <x> was already read
                    if ((state == ANCHOR && spriteset.back().real.find("anchor.x") != spriteset.back().real.end())
                     || (state == STEM   && spriteset.back().real.find("stem.x")   != spriteset.back().real.end()))
                         mythrow("Unexpected <x> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    if (state == ANCHOR) spriteset.back().real["anchor.x"] = strtof(buffer.c_str(), NULL);
                    else spriteset.back().real["stem.x"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </x>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("x")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </x> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // position's Y-coordinate
                else if (xmlStrEqual(tag, STR_CAST("y")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check if <y> was already read
                    if ((state == ANCHOR && spriteset.back().real.find("anchor.y") != spriteset.back().real.end())
                     || (state == STEM   && spriteset.back().real.find("stem.y")   != spriteset.back().real.end()))
                         mythrow("Unexpected <y> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    if (state == ANCHOR) spriteset.back().real["anchor.y"] = strtof(buffer.c_str(), NULL);
                    else spriteset.back().real["stem.y"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </y>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("y")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </y> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </anchor>
                else if (xmlStrEqual(tag, STR_CAST("anchor")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (state == ANCHOR)    // if we are within an <anchor> section
                    {                       //     this is OK
                        if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end()) mythrow("Expected <x> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        if (spriteset.back().real.find("anchor.y") == spriteset.back().real.end()) mythrow("Expected <y> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        state = prestate;
                    }                       // if we are whithin a <stem> section, throw error
                    else mythrow("Unexpected </anchor> in <stem> section (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </stem>
                else if (xmlStrEqual(tag, STR_CAST("stem")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (state == STEM)      // if we are within an <stem> section
                    {                       //     this is OK
                        if (spriteset.back().real.find("stem.x") == spriteset.back().real.end()) mythrow("Expected <x> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        if (spriteset.back().real.find("stem.y") == spriteset.back().real.end()) mythrow("Expected <y> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        state = prestate;
                    }                       // if we are whithin an <anchor> section, throw error
                    else mythrow("Unexpected </stem> in <anchor> section (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // any other tag is illegal here
                else
                {
                    if (state == STEM)
                    {
                        if (spriteset.back().real.find("stem.x") == spriteset.back().real.end()) mythrow("Expected <x> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        if (spriteset.back().real.find("stem.y") == spriteset.back().real.end()) mythrow("Expected <y> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                        mythrow("Expected </stem> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    }
                    if (spriteset.back().real.find("anchor.x") == spriteset.back().real.end()) mythrow("Expected <x> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("anchor.y") == spriteset.back().real.end()) mythrow("Expected <y> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </anchor> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <keybound> section parser  (see <clef> section)
            // ---------------------------
            case KEYBOUND:
                // boundaries for <sharp> key-signatures
                if (xmlStrEqual(tag, STR_CAST("sharp")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check if <sharp> was already read
                    if (spriteset.back().integer.find("keybound.sharp") != spriteset.back().integer.end())
                         mythrow("Unexpected <sharp> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["keybound.sharp"] = atoi(buffer.c_str());
                    
                    // check for end tag </sharp>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("sharp")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </sharp> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // boundaries for <flat> key-signatures
                else if (xmlStrEqual(tag, STR_CAST("flat")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check if <flat> was already read
                    if (spriteset.back().integer.find("keybound.flat") != spriteset.back().integer.end())
                         mythrow("Unexpected <flat> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().integer["keybound.flat"] = atoi(buffer.c_str());
                    
                    // check for end tag </flat>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("flat")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </flat> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </keybound>
                else if (xmlStrEqual(tag, STR_CAST("keybound")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    // check, if keybound-information is complete
                    if (spriteset.back().integer.find("keybound.sharp") == spriteset.back().integer.end()) mythrow("Expected <sharp> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().integer.find("keybound.flat") == spriteset.back().integer.end()) mythrow("Expected <flat> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = CLEF;   // return to clef-parser
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().integer.find("keybound.sharp") == spriteset.back().integer.end()) mythrow("Expected <sharp> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().integer.find("keybound.flat") == spriteset.back().integer.end()) mythrow("Expected <flat> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </keybound> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <stem> section parser  (see the rest's <flag>-section)
            // -----------------------
            case RESTSTEM:
                // the two <top> anchor points on the flag (regarding the flag sprite)
                if (xmlStrEqual(tag, STR_CAST("top")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().integer.find("stem.top.x1") != spriteset.back().integer.end()) mythrow("Unexpected second <top> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = RESTSTEMTOPPOS;     // jump to top-position parser
                }
                
                // the two <bottom> anchor points on the flag (regarding the bottom sprite)
                else if (xmlStrEqual(tag, STR_CAST("bottom")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    if (spriteset.back().integer.find("stem.bottom.x1") != spriteset.back().integer.end()) mythrow("Unexpected second <bottom> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = RESTSTEMBOTTOMPOS;  // jump to bottom-position parser
                }
                
                // parse <minlen>, the minimal length of the short rests stem
                else if (xmlStrEqual(tag, STR_CAST("minlen")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check if <minlen> was already read
                    if (spriteset.back().real.find("stem.minlen") != spriteset.back().real.end()) mythrow("Unexpected second <minlen> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real["stem.minlen"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </minlen>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("minlen")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </minlen> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </stem>
                else if (xmlStrEqual(tag, STR_CAST("stem")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (spriteset.back().real.find("stem.top.x1") == spriteset.back().real.end()) mythrow("Expected <top> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.bottom.x1") == spriteset.back().real.end()) mythrow("Expected <bottom> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = RESTFLAG;   // return to <flag> parser
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().real.find("stem.top.x1") == spriteset.back().real.end()) mythrow("Expected <top> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.bottom.x1") == spriteset.back().real.end()) mythrow("Expected <bottom> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    mythrow("Expected </stem> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <top> and <bottom> sections (for rest-stems)
            // ----------------------------------------------
            case RESTSTEMTOPPOS:
            case RESTSTEMBOTTOMPOS:
                // read <x1> coordinate
                if (xmlStrEqual(tag, STR_CAST("x1")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check if <x1> was already read
                    if (spriteset.back().real.find((state == RESTSTEMTOPPOS) ? "stem.top.x1" : "stem.bottom.x1") != spriteset.back().real.end()) mythrow("Unexpected <x1> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real[(state == RESTSTEMTOPPOS) ? "stem.top.x1" : "stem.bottom.x1"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </x1>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("x1")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </x1> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // read <y1> coordinate
                else if (xmlStrEqual(tag, STR_CAST("y1")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check if <y1> was already read
                    if (spriteset.back().real.find((state == RESTSTEMTOPPOS) ? "stem.top.y1" : "stem.bottom.y1") != spriteset.back().real.end()) mythrow("Unexpected <y1> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real[(state == RESTSTEMTOPPOS) ? "stem.top.y1" : "stem.bottom.y1"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </y1>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("y1")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </y1> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // read <x2> coordinate
                else if (xmlStrEqual(tag, STR_CAST("x2")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check if <x2> was already read
                    if (spriteset.back().real.find((state == RESTSTEMTOPPOS) ? "stem.top.x2" : "stem.bottom.x2") != spriteset.back().real.end()) mythrow("Unexpected <x2> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real[(state == RESTSTEMTOPPOS) ? "stem.top.x2" : "stem.bottom.x2"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </x2>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("x2")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </x2> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // read <y2> coordinate
                else if (xmlStrEqual(tag, STR_CAST("y2")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_ELEMENT)
                {
                    // check if <y2> was already read
                    if (spriteset.back().real.find((state == RESTSTEMTOPPOS) ? "stem.top.y2" : "stem.bottom.y2") != spriteset.back().real.end()) mythrow("Unexpected <y2> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // empty tag?
                    if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    // read the tag content
                    std::string buffer;         // content buffer
                    while (xmlTextReaderNodeType(parser) == 3 && xmlTextReaderHasValue(parser) == 1)
                    {
                        buffer.append(XML_CAST(xmlTextReaderConstValue(parser)));
                        if (xmlTextReaderRead(parser) != 1) mythrow("XML-Syntax Error (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    };
                    
                    // parse the number
                    spriteset.back().real[(state == RESTSTEMTOPPOS) ? "stem.top.y2" : "stem.bottom.y2"] = strtof(buffer.c_str(), NULL);
                    
                    // check for end tag </y2>
                    tag = xmlTextReaderConstName(parser);
                    if (xmlStrEqual(tag, STR_CAST("y2")) != 1 || xmlTextReaderNodeType(parser) != XML_READER_TYPE_END_ELEMENT) mythrow("Expected </y2> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                }
                
                // end tag </top>
                else if (state == RESTSTEMTOPPOS && xmlStrEqual(tag, STR_CAST("top")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (spriteset.back().real.find("stem.top.x1") == spriteset.back().real.end()) mythrow("Expected <x1> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.top.y1") == spriteset.back().real.end()) mythrow("Expected <y1> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.top.x2") == spriteset.back().real.end()) mythrow("Expected <x2> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.top.y2") == spriteset.back().real.end()) mythrow("Expected <y2> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = RESTSTEM;   // return to <stem> parser
                }
                
                // end tag </bottom>
                else if (state == RESTSTEMBOTTOMPOS && xmlStrEqual(tag, STR_CAST("bottom")) == 1 && xmlTextReaderNodeType(parser) == XML_READER_TYPE_END_ELEMENT)
                {
                    if (spriteset.back().real.find("stem.bottom.x1") == spriteset.back().real.end()) mythrow("Expected <x1> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.bottom.y1") == spriteset.back().real.end()) mythrow("Expected <y1> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.bottom.x2") == spriteset.back().real.end()) mythrow("Expected <x2> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.bottom.y2") == spriteset.back().real.end()) mythrow("Expected <y2> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    state = RESTSTEM;   // return to <stem> parser
                }
                
                // any other tag is illegal here
                else
                {
                    if (spriteset.back().real.find("stem.top.x1") == spriteset.back().real.end() &&
                        spriteset.back().real.find("stem.bottom.x1") == spriteset.back().real.end()) mythrow("Expected <x1> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.top.y1") == spriteset.back().real.end() &&
                        spriteset.back().real.find("stem.bottom.y1") == spriteset.back().real.end()) mythrow("Expected <y1> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.top.x2") == spriteset.back().real.end() &&
                        spriteset.back().real.find("stem.bottom.x2") == spriteset.back().real.end()) mythrow("Expected <x2> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    if (spriteset.back().real.find("stem.top.y2") == spriteset.back().real.end() &&
                        spriteset.back().real.find("stem.bottom.y2") == spriteset.back().real.end()) mythrow("Expected <y2> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    
                    if (state == RESTSTEMTOPPOS) mythrow("Expected </top> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                    else mythrow("Expected </bottom> (in file \"%s\", at line %i:%i)", filename, xmlTextReaderGetParserLineNumber(parser), xmlTextReaderGetParserColumnNumber(parser));
                };
                break;
                
            //  <movable> user defined symbols
            // --------------------------------
            case MOVABLE:
                // TODO: parse <movable> section
                break;
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
        throw;                                          // throw parser error
    };
    
    // parsing successful
    xmlFreeTextReader(parser);  // delete parser instance
}

// dump sprite info to stdout
void Renderer::dump() const
{
    for (Sprites::const_iterator s = sprites.begin(); s != sprites.end(); s++)
    for (SpriteSet::const_iterator i = s->begin(); i != s->end(); i++)
    {
        for (std::map<std::string, std::string>::const_iterator t = i->text.begin(); t != i->text.end(); t++)
        {
            std::cout << t->first << "$ = \"" << t->second << "\"\n";
        };
        std::cout << "path$ = \"" << i->path << "\"\n";
        std::cout << "width% = " << i->width << "\n";
        std::cout << "height% = " << i->height << "\n";
        for (std::map<std::string, int>::const_iterator n = i->integer.begin(); n != i->integer.end(); n++)
        {
            std::cout << n->first << "% = " << n->second << "\n";
        };
        for (std::map<std::string, double>::const_iterator r = i->real.begin(); r != i->real.end(); r++)
        {
            std::cout << r->first << "& = " << r->second << "\n";
        };
        std::cout << "\n";
    };
}

// render a cubic bézier curve
void Renderer::bezier(double  x1, double  y1,
                      double cx1, double cy1,
                      double cx2, double cy2,
                      double  x2, double  y2)
{
    // calculate step
    const double step = 4.0 / (
        sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)) + 
        sqrt((cx1 - x1) * (cx1 - x1) + (cy1 - y1) * (cy1 - y1)) +
        sqrt((cx2 - cx1) * (cx2 - cx1) + (cy2 - cy1) * (cy2 - cy1)) + 
        sqrt((x2 - cx2) * (x2 - cx2) + (y2 - cy2) * (y2 - cy2)));
    
    // render bézier curve
    move_to(x1, y1);    // move to beginning
    for (double t = 0, s = 1; t <= 1 && s >= 0; t += step, s -= step)   // for each segment
    {
        // line to next point
        // B(t) = (1-t)^3 * p1 + 3*(1-t)^2*t * c1 + 3*(1-t)*t^2 * c2 + t^3 * p2
        line_to(s*s*(s*x1 + 3*t*cx1) + t*t*(3*s*cx2 + t*x2),
                s*s*(s*y1 + 3*t*cy1) + t*t*(3*s*cy2 + t*y2));
    };
    line_to(x2, y2);    // move to end
    stroke();           // render the path
}

// render a cubic bézier curve with different thickness in the center than at the ends
void Renderer::bezier_slur(double  x1, double  y1,
                           double cx1, double cy1,
                           double cx2, double cy2,
                           double  x2, double  y2,
                           double  w0, double  w1)
{
    // create local variables
    const double step = 4.0 / ( // calculate step
        sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)) + 
        sqrt((cx1 - x1) * (cx1 - x1) + (cy1 - y1) * (cy1 - y1)) +
        sqrt((cx2 - cx1) * (cx2 - cx1) + (cy2 - cy1) * (cy2 - cy1)) + 
        sqrt((x2 - cx2) * (x2 - cx2) + (y2 - cy2) * (y2 - cy2)));
    double _x1 = x1;        // the previous point (_x1, _y1)
    double _y1 = y1;
    double _x2 = x2;        // the current point (_x2, _y2)
    double _y2 = y2;
    double w = 0;           // half of the current curve-width (absolute value of the offset)
    double l = 0;           // length of the current segment
    
    // render upper bézier curve
    move_to(x1, y1);    // move to beginning
    for (double t = 0, s = 1; t <= 1 && s >= 0; t += step, s -= step)   // for each segment
    {
        // calculate next point
        // B(t) = (1-t)^3 * p1 + 3*(1-t)^2*t * c1 + 3*(1-t)*t^2 * c2 + t^3 * p2
        _x2 = s*s*(s*x1 + 3*t*cx1) + (3*s*cx2 + t*x2)*t*t;
        _y2 = s*s*(s*y1 + 3*t*cy1) + (3*s*cy2 + t*y2)*t*t;
        
        // calculate offset (w = linewidth / 2)
        w = 2 * (w1 - w0) * t * s + w0;
        
        // calculate segment length
        l = sqrt((_x2 - _x1) * (_x2 - _x1) + (_y2 - _y1) * (_y2 - _y1));
        
        // draw segment (add normal if the segment length is positive)
        line_to(_x2 + ((l > 0) ? (_y2 - _y1) * w / l : 0),
                _y2 + ((l > 0) ? (_x1 - _x2) * w / l : 0));
        
        // save point
        _x1 = _x2;
        _y1 = _y2;
    };
    
    // draw final segment
    l = sqrt((_x2 - x2) * (_x2 - x2) + (_y2 - y2) * (_y2 - y2));    // calculate segment length
    line_to(_x2 + ((l > 0) ? (_y2 - y2) * w / l : 0),               // draw segment
            _y2 + ((l > 0) ? (x2 - _x2) * w / l : 0));
    _x1 = x2;           // save end-point
    _y1 = y2;
    
    // render lower bézier curve
    for (double s = 0, t = 1; s <= 1 && t >= 0; s += step, t -= step)   // for each segment
    {   // (swapped s and t, to follow curve backwards)
        
        // calculate next point
        // B(t) = (1-t)^3 * p1 + 3*(1-t)^2*t * c1 + 3*(1-t)*t^2 * c2 + t^3 * p2
        _x2 = s*s*(s*x1 + 3*t*cx1) + (3*s*cx2 + t*x2)*t*t;
        _y2 = s*s*(s*y1 + 3*t*cy1) + (3*s*cy2 + t*y2)*t*t;
        
        // calculate offset (w = linewidth / 2)
        w = 2 * (w1 - w0) * t * s + w0;
        
        // calculate segment length
        l = sqrt((_x2 - _x1) * (_x2 - _x1) + (_y2 - _y1) * (_y2 - _y1));
        
        // draw segment (add negative normal if the segment length is positive)
        line_to(_x2 + ((l > 0) ? (_y2 - _y1) * w / l : 0),
                _y2 + ((l > 0) ? (_x1 - _x2) * w / l : 0));
        
        // save point
        _x1 = _x2;
        _y1 = _y2;
    };
    
    // draw final segment
    l = sqrt((_x2 - x1) * (_x2 - x1) + (_y2 - y1) * (_y2 - y1));    // calculate segment length
    line_to(_x2 + ((l > 0) ? (_y2 - y1) * w / l : 0),               // draw segment
            _y2 + ((l > 0) ? (x1 - _x2) * w / l : 0));
    
    // finish rendering
    fill();         // fill the path
    stroke();       // render the path
}


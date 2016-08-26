
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

#ifndef SCOREPRESS_UI_HH
#define SCOREPRESS_UI_HH

#include "file_reader.hh"   // FileReader, DocumentReader
#include "file_writer.hh"   // FileWriter, DocumentWriter
#include "renderer.hh"      // Renderer
#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API UI;   // abstact user-interface base-class


//
//     class UI
//    ==========
//
// This is an abstract user-interface class to be used by external modules
// and implemented by the GUI-frontend.
//
class SCOREPRESS_API UI
{
 public:
    virtual void register_open_format(DocumentReader& reader) = 0;
    virtual void register_save_format(DocumentWriter& writer) = 0;
    virtual void register_import_format(DocumentReader& reader) = 0;
    virtual void register_export_format(DocumentWriter& writer) = 0;
    virtual void register_renderer(Renderer& renderer) = 0;
};

} // end namespace

#endif


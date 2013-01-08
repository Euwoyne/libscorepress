
#
#  ScorePress - Music Engraving Software  (libscorepress)
#  Copyright (C) 2012 Dominik Lehmann
#  
#  Licensed under the EUPL, Version 1.1 or - as soon they
#  will be approved by the European Commission - subsequent
#  versions of the EUPL (the "Licence");
#  You may not use this work except in compliance with the
#  Licence.
#  
#  Unless required by applicable law or agreed to in
#  writing, software distributed under the Licence is
#  distributed on an "AS IS" basis, WITHOUT WARRANTIES OR
#  CONDITIONS OF ANY KIND, either expressed or implied.
#  See the Licence for the specific language governing
#  permissions and limitations under the Licence.
#

#
# MAKEFILE
#

CXX      := g++
CMAKE    := make
AR       := ar rcs
LN       := ln -fs
LDCONFIG := ldconfig -n
INSTALL  := install -m 0755

SRCDIR   := src
BUILDDIR := obj
BINDIR   := .

SOFILE   := libscorepress.so.0.2.4
SONAME   := libscorepress.so.0
SOBASE   := libscorepress.so
AFILE    := libscorepress.a

MODE := DEBUG


INC_XML  := `pkg-config libxml-2.0 --cflags`
LIB_XML  := `pkg-config libxml-2.0 --libs`


WARNINGS := -W                     \
            -Wall                  \
            -Wextra                \
            -Wcast-qual            \
            -Wcast-align           \
            -Wconversion           \
            -Wfloat-equal          \
            -Winit-self            \
            -Wmissing-declarations \
            -Wold-style-cast       \
            -Wpointer-arith        \
            -Wredundant-decls      \
            -Wshadow               \
            -Wundef                \
            -Wunreachable-code     \
            -Wwrite-strings
            
CTRLFLAGS := -ansi                 \
             -pedantic             \
             -fno-nonansi-builtins
             
DEBUG   := -ggdb -O0
PROFILE := -pg -O2
RELEASE := -O2 -fomit-frame-pointer

FLAGS := $(WARNINGS) $(CTRLFLAGS) $($(MODE)) -I`pwd`/$(SRCDIR)
FRACT_SIZECHECK := -DSIZECHECK -Wno-long-long

SOFILES := $(BUILDDIR)/engine.s.o         \
           $(BUILDDIR)/press.s.o          \
           $(BUILDDIR)/engraver.s.o       \
           $(BUILDDIR)/engraver_state.s.o \
           $(BUILDDIR)/engrave_info.s.o   \
           $(BUILDDIR)/context.s.o        \
           $(BUILDDIR)/pageset.s.o        \
           $(BUILDDIR)/plate.s.o          \
           $(BUILDDIR)/pick.s.o           \
           $(BUILDDIR)/cursor.s.o         \
           $(BUILDDIR)/user_cursor.s.o    \
           $(BUILDDIR)/edit_cursor.s.o    \
           $(BUILDDIR)/renderer.s.o       \
           $(BUILDDIR)/sprites.s.o        \
           $(BUILDDIR)/sprite_id.s.o      \
           $(BUILDDIR)/score.s.o          \
           $(BUILDDIR)/classes.s.o        \
           $(BUILDDIR)/log.s.o            \
           $(BUILDDIR)/fraction.s.o

AFILES := $(BUILDDIR)/engine.o         \
          $(BUILDDIR)/press.o          \
          $(BUILDDIR)/engraver.o       \
          $(BUILDDIR)/engraver_state.o \
          $(BUILDDIR)/engrave_info.o   \
          $(BUILDDIR)/context.o        \
          $(BUILDDIR)/pageset.o        \
          $(BUILDDIR)/plate.o          \
          $(BUILDDIR)/pick.o           \
          $(BUILDDIR)/cursor.o         \
          $(BUILDDIR)/user_cursor.o    \
          $(BUILDDIR)/edit_cursor.o    \
          $(BUILDDIR)/renderer.o       \
          $(BUILDDIR)/sprites.o        \
          $(BUILDDIR)/sprite_id.o      \
          $(BUILDDIR)/score.o          \
          $(BUILDDIR)/classes.o        \
          $(BUILDDIR)/log.o            \
          $(BUILDDIR)/fraction.o


#
# MAIN TARGETS
#

build: shared static

$(BUILDDIR):
	-mkdir $(BUILDDIR)

shared: $(BUILDDIR) $(BUILDDIR)/$(SOFILE)

static: $(BUILDDIR) $(BUILDDIR)/$(AFILE)

doc:
	doxygen doxygen/Doxyfile


install: install-shared install-static

install-shared:
	$(INSTALL) $(BUILDDIR)/$(SOFILE) $(BINDIR)
	$(LDCONFIG) $(BINDIR)
	$(LN) $(BINDIR)/$(SONAME) $(BINDIR)/$(SOBASE)

install-static:
	cp $(BUILDDIR)/$(AFILE) $(BINDIR)/$(AFILE)


uninstall: uninstall-shared uninstall-static

uninstall-shared:
	-rm -f $(BINDIR)/$(SOBASE)
	-rm -f $(BINDIR)/$(SONAME)
	-rm -f $(BINDIR)/$(SOFILE)

uninstall-static:
	-rm -f $(BINDIR)/$(AFILE)


dist-clean:
	-rm -r $(BUILDDIR)

cleanshared:         dist-clean
cleanstatic:         dist-clean
cleaninstall-shared: uninstall-shared
cleaninstall-static: uninstall-static

clear:               dist-clean uninstall


#
# TARGET FILES
#

$(BUILDDIR)/$(SOFILE):	$(SOFILES)
						$(CXX) -shared -Wl,-soname,$(SONAME) -lc $(SOFILES) -o $(BUILDDIR)/$(SOFILE) $(LIB_XML) $(FLAGS)

$(BUILDDIR)/$(AFILE):	$(AFILES)
						$(AR) $(BUILDDIR)/$(AFILE) $(AFILES)

#
# OBJECT FILES
#

$(BUILDDIR)/engine.o:		$(SRCDIR)/engine.cpp
							$(CXX) -c $(SRCDIR)/engine.cpp -o $(BUILDDIR)/engine.o $(FLAGS)
$(BUILDDIR)/engine.s.o:		$(SRCDIR)/engine.cpp
							$(CXX) -fPIC -c $(SRCDIR)/engine.cpp -o $(BUILDDIR)/engine.s.o $(FLAGS)

$(BUILDDIR)/press.o:		$(SRCDIR)/press.cpp
							$(CXX) -c $(SRCDIR)/press.cpp -o $(BUILDDIR)/press.o $(FLAGS)
$(BUILDDIR)/press.s.o:		$(SRCDIR)/press.cpp
							$(CXX) -fPIC -c $(SRCDIR)/press.cpp -o $(BUILDDIR)/press.s.o $(FLAGS)

$(BUILDDIR)/engraver.o:		$(SRCDIR)/engraver.cpp
							$(CXX) -c $(SRCDIR)/engraver.cpp -o $(BUILDDIR)/engraver.o $(FLAGS)
$(BUILDDIR)/engraver.s.o:	$(SRCDIR)/engraver.cpp
							$(CXX) -fPIC -c $(SRCDIR)/engraver.cpp -o $(BUILDDIR)/engraver.s.o $(FLAGS)

$(BUILDDIR)/engraver_state.o:	$(SRCDIR)/engraver_state.cpp
								$(CXX) -c $(SRCDIR)/engraver_state.cpp -o $(BUILDDIR)/engraver_state.o $(FLAGS)
$(BUILDDIR)/engraver_state.s.o:	$(SRCDIR)/engraver_state.cpp
								$(CXX) -fPIC -c $(SRCDIR)/engraver_state.cpp -o $(BUILDDIR)/engraver_state.s.o $(FLAGS)

$(BUILDDIR)/engrave_info.o:		$(SRCDIR)/engrave_info.cpp
								$(CXX) -c $(SRCDIR)/engrave_info.cpp -o $(BUILDDIR)/engrave_info.o $(FLAGS)
$(BUILDDIR)/engrave_info.s.o:	$(SRCDIR)/engrave_info.cpp
								$(CXX) -fPIC -c $(SRCDIR)/engrave_info.cpp -o $(BUILDDIR)/engrave_info.s.o $(FLAGS)

$(BUILDDIR)/context.o:		$(SRCDIR)/context.cpp
							$(CXX) -c $(SRCDIR)/context.cpp -o $(BUILDDIR)/context.o $(FLAGS)
$(BUILDDIR)/context.s.o:	$(SRCDIR)/context.cpp
							$(CXX) -fPIC -c $(SRCDIR)/context.cpp -o $(BUILDDIR)/context.s.o $(FLAGS)

$(BUILDDIR)/pageset.o:		$(SRCDIR)/pageset.cpp
							$(CXX) -c $(SRCDIR)/pageset.cpp -o $(BUILDDIR)/pageset.o $(FLAGS)
$(BUILDDIR)/pageset.s.o:	$(SRCDIR)/pageset.cpp
							$(CXX) -fPIC -c $(SRCDIR)/pageset.cpp -o $(BUILDDIR)/pageset.s.o $(FLAGS)

$(BUILDDIR)/plate.o:		$(SRCDIR)/plate.cpp
							$(CXX) -c $(SRCDIR)/plate.cpp -o $(BUILDDIR)/plate.o $(FLAGS)
$(BUILDDIR)/plate.s.o:		$(SRCDIR)/plate.cpp
							$(CXX) -fPIC -c $(SRCDIR)/plate.cpp -o $(BUILDDIR)/plate.s.o $(FLAGS)

$(BUILDDIR)/cursor.o:		$(SRCDIR)/cursor.cpp
							$(CXX) -c $(SRCDIR)/cursor.cpp -o $(BUILDDIR)/cursor.o $(FLAGS)
$(BUILDDIR)/cursor.s.o:		$(SRCDIR)/cursor.cpp
							$(CXX) -fPIC -c $(SRCDIR)/cursor.cpp -o $(BUILDDIR)/cursor.s.o $(FLAGS)

$(BUILDDIR)/user_cursor.o:		$(SRCDIR)/user_cursor.cpp
								$(CXX) -c $(SRCDIR)/user_cursor.cpp -o $(BUILDDIR)/user_cursor.o $(FLAGS)
$(BUILDDIR)/user_cursor.s.o:	$(SRCDIR)/user_cursor.cpp
								$(CXX) -fPIC -c $(SRCDIR)/user_cursor.cpp -o $(BUILDDIR)/user_cursor.s.o $(FLAGS)

$(BUILDDIR)/edit_cursor.o:		$(SRCDIR)/edit_cursor.cpp
								$(CXX) -c $(SRCDIR)/edit_cursor.cpp -o $(BUILDDIR)/edit_cursor.o $(FLAGS)
$(BUILDDIR)/edit_cursor.s.o:	$(SRCDIR)/edit_cursor.cpp
								$(CXX) -fPIC -c $(SRCDIR)/edit_cursor.cpp -o $(BUILDDIR)/edit_cursor.s.o $(FLAGS)

$(BUILDDIR)/pick.o:			$(SRCDIR)/pick.cpp
							$(CXX) -c $(SRCDIR)/pick.cpp -o $(BUILDDIR)/pick.o $(FLAGS)
$(BUILDDIR)/pick.s.o:		$(SRCDIR)/pick.cpp
							$(CXX) -fPIC -c $(SRCDIR)/pick.cpp -o $(BUILDDIR)/pick.s.o $(FLAGS)

$(BUILDDIR)/renderer.o:		$(SRCDIR)/renderer.cpp
							$(CXX) -c $(SRCDIR)/renderer.cpp -o $(BUILDDIR)/renderer.o $(INC_XML) $(FLAGS)
$(BUILDDIR)/renderer.s.o:	$(SRCDIR)/renderer.cpp
							$(CXX) -fPIC -c $(SRCDIR)/renderer.cpp -o $(BUILDDIR)/renderer.s.o $(INC_XML) $(FLAGS)

$(BUILDDIR)/sprites.o:		$(SRCDIR)/sprites.cpp
							$(CXX) -c $(SRCDIR)/sprites.cpp -o $(BUILDDIR)/sprites.o $(FLAGS)
$(BUILDDIR)/sprites.s.o:	$(SRCDIR)/sprites.cpp
							$(CXX) -fPIC -c $(SRCDIR)/sprites.cpp -o $(BUILDDIR)/sprites.s.o $(FLAGS)

$(BUILDDIR)/sprite_id.o:	$(SRCDIR)/sprite_id.cpp
							$(CXX) -c $(SRCDIR)/sprite_id.cpp -o $(BUILDDIR)/sprite_id.o $(FLAGS)
$(BUILDDIR)/sprite_id.s.o:	$(SRCDIR)/sprite_id.cpp
							$(CXX) -fPIC -c $(SRCDIR)/sprite_id.cpp -o $(BUILDDIR)/sprite_id.s.o $(FLAGS)

$(BUILDDIR)/score.o:		$(SRCDIR)/score.cpp
							$(CXX) -c $(SRCDIR)/score.cpp -o $(BUILDDIR)/score.o $(FLAGS)
$(BUILDDIR)/score.s.o:		$(SRCDIR)/score.cpp
							$(CXX) -fPIC -c $(SRCDIR)/score.cpp -o $(BUILDDIR)/score.s.o $(FLAGS)

$(BUILDDIR)/classes.o:		$(SRCDIR)/classes.cpp
							$(CXX) -c $(SRCDIR)/classes.cpp -o $(BUILDDIR)/classes.o $(FLAGS)
$(BUILDDIR)/classes.s.o:	$(SRCDIR)/classes.cpp
							$(CXX) -fPIC -c $(SRCDIR)/classes.cpp -o $(BUILDDIR)/classes.s.o $(FLAGS)

$(BUILDDIR)/log.o:			$(SRCDIR)/log.cpp
							$(CXX) -c $(SRCDIR)/log.cpp -o $(BUILDDIR)/log.o $(FLAGS)
$(BUILDDIR)/log.s.o:		$(SRCDIR)/log.cpp
							$(CXX) -fPIC -c $(SRCDIR)/log.cpp -o $(BUILDDIR)/log.s.o $(FLAGS)

$(BUILDDIR)/fraction.o:		$(SRCDIR)/fraction.cpp
							$(CXX) -c $(SRCDIR)/fraction.cpp -o $(BUILDDIR)/fraction.o $(FLAGS) $(FRACT_SIZECHECK)
$(BUILDDIR)/fraction.s.o:	$(SRCDIR)/fraction.cpp
							$(CXX) -fPIC -c $(SRCDIR)/fraction.cpp -o $(BUILDDIR)/fraction.s.o $(FLAGS) $(FRACT_SIZECHECK)

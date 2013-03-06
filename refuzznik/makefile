# Copyright (c) 2007 Johan Sarge
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software, and to permit
# persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

# Version number variables.
# This number was incremented recently.
# One more test.
plugversion := 0.5.4

# Directory variables.
sdkdir := /c/code/c/vstsdk2.3/source/common
installdir := "/c/Program Files/Image-Line/FL Studio 6/Plugins/VST/"

builddirs := bin/debug bin/dist dist

ifndef debug
odir := bin/dist
else
odir := bin/debug
endif


# Filename variables.
noguizip := dist/RefuzznikNoGUI-$(plugversion).zip
noguitar := dist/RefuzznikNoGUI-$(plugversion).tar.bz2
srczip := dist/Refuzznik-src-$(plugversion).zip
srctar := dist/Refuzznik-src-$(plugversion).tar.bz2

noguiplug := RefuzznikNoGUI.dll

deffile := Refuzznik.def

noguidistfiles := LICENSE $(noguiplug) readme.txt
srcdistfiles := LICENSE makefile $(deffile) *.cpp *.h *.hpp readme.txt

noguiheader := Refuzznik.hpp refuzznikparams.h
noguiobj := $(odir)/RefuzznikMainNoGUI.o $(odir)/RefuzznikNoGUI.o

commonsdkobj := $(odir)/audioeffectx.o $(odir)/AudioEffect.o


# Command option variables.
idirs := -I$(sdkdir)

ifndef debug
CXXFLAGS := -D_MT -march=pentium2 -O3 $(idirs)
dllflags := -shared -Wl,--gc-sections -Wl,-s
else
CXXFLAGS := -D_MT $(idirs)
dllflags := -shared
endif


# Phony targets.
.PHONY : all clean nogui install noguidist srcdist

all : noguidist srcdist

clean :
	$(RM) $(noguiplug)
	$(RM) $(odir)/*.o

nogui : $(builddirs) $(noguiplug)

install : nogui
	cp $(noguiplug) $(installdir)

noguidist : $(builddirs) $(noguizip) $(noguitar)

srcdist : $(builddirs) $(srczip) $(srctar)


# File targets.
$(builddirs) :
	mkdir -p $@

$(noguizip) : $(noguidistfiles)
	wzzip -uP $@ $^

$(noguitar) : $(noguidistfiles)
	tar -jcf $@ $^

$(srczip) : $(srcdistfiles)
	wzzip -uP $@ $^

$(srctar) : $(srcdistfiles)
	tar -jcf $@ $^

$(noguiplug) : $(deffile) $(noguiobj) $(commonsdkobj)
	$(CXX) $(CXXFLAGS) $(dllflags) -o $@ $^

$(noguiobj) : $(odir)/%NoGUI.o : %NoGUI.cpp %.cpp $(noguiheader)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(commonsdkobj) : $(odir)/%.o : $(sdkdir)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<


vnum := 1
bnum := $(vnum).0.1

libname := graphdll

# Directory variables.
ifdef prof
odir := obj/prof
bdir := bin/prof
else
ifdef debug
odir := obj/debug
bdir := bin/debug
else
odir := obj/dist
bdir := bin/dist
endif
endif

builddirs := $(odir) $(bdir)

# Filename variables.
ifdef windows
dllfile := $(bdir)/$(libname).dll
testfile := $(bdir)/cgtest.exe
testfile2 := $(bdir)/cgtest2.exe
else
dllfile := $(bdir)/lib$(libname).so.$(bnum)
testfile := $(bdir)/cgtest
testfile2 := $(bdir)/cgtest2
endif

testfiles := $(testfile) $(testfile2)

headers := *.hpp
testcode := cgtest.cpp leveleq.cpp
testcode2 := cgtest2.cpp imgdiff.cpp
basecode := $(filter-out $(testcode) $(testcode2), $(wildcard *.cpp))
testobj := $(addprefix $(odir)/, $(addsuffix .o, $(basename $(testcode))))
testobj2 := $(addprefix $(odir)/, $(addsuffix .o, $(basename $(testcode2))))
baseobj := $(addprefix $(odir)/, $(addsuffix .o, $(basename $(basecode))))

# Command option variables.
idirs :=
libdirs :=

ifndef debug
cxxflags1 := -DNDEBUG -DCG_GRAPHDLL_DLL_BUILD -fno-exceptions -O9 $(idirs)
dllflags := -shared -Wl,--gc-sections -Wl,-S
else
cxxflags1 := -ggdb -DCG_GRAPHDLL_DLL_BUILD -fno-exceptions $(idirs)
dllflags := -shared
endif

ifdef prof
cxxflags1 += -pg
endif

ifdef windows
# NOTE: Linking the GCC and C++ libs dynamically seems to bother Scilab* on Windows,
# so they are set to static linking here. (* It gave a strange message about not
# being able to link 64-bit DLLs, even though everything was 32-bit.)
dllflags += -static-libgcc -static-libstdc++ -Wl,--out-implib=$(bdir)/lib$(libname).a
else
cxxflags1 += -fpic
dllflags += -Wl,-soname=lib$(libname).so.$(vnum)
endif

override CXXFLAGS += $(cxxflags1)

# Phony targets.
.PHONY : all capi clean test

all : capi test

capi : $(builddirs) $(dllfile)

clean :
	$(RM) $(odir)/*.o $(bdir)/*.dll $(bdir)/*.so* $(testfiles)

test : $(builddirs) $(testfiles)

# File targets.
$(builddirs) :
	mkdir -p $@

$(dllfile) : $(baseobj)
	$(CXX) $(CXXFLAGS) $(dllflags) $(libdirs) -o $@ $^

$(testfile) : $(testobj) $(baseobj)
	$(CXX) $(CXXFLAGS) $(libdirs) -o $@ $^

$(testfile2) : $(testobj2) $(baseobj)
	$(CXX) $(CXXFLAGS) $(libdirs) -o $@ $^

$(odir)/%.o : %.cpp $(headers)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

name := aresdl
build := debug
threaded := false
openmp := false
vulkan := false
local := true
lto := true
flags += -I. -I.. -I../ares -I../thirdparty -DMIA_LIBRARY

nall.path := ../nall
include $(nall.path)/GNUmakefile

ifneq ($(filter $(arch),x86 amd64),)
  ifeq ($(filter cl,$(compiler)),)
    ifeq ($(local),true)
      flags += -march=native
    else
      # For official builds, default to x86-64-v2 (Intel Nehalem, AMD Bulldozer) which supports up to SSE 4.2.
      flags += -march=x86-64-v2
    endif
  endif
endif

libco.path := ../libco
include $(libco.path)/GNUmakefile

thirdparty.path := ../thirdparty
sljit.path := $(thirdparty.path)/sljit/sljit_src
libchdr.path := $(thirdparty.path)/libchdr
tzxfile.path := $(thirdparty.path)/TZXFile
ymfm.path := $(thirdparty.path)/ymfm
include $(thirdparty.path)/GNUmakefile


profile := performance
cores := a26 fc sfc n64 sg ms md ps1 pce ng msx cv myvision gb gba ws ngp spec
#cores += saturn

ares.path := ../ares
include $(ares.path)/GNUmakefile

mia.path := ../mia

mia.objects := mia mia-resource
mia.objects := $(mia.objects:%=$(object.path)/%.o)

$(object.path)/mia.o: $(mia.path)/mia.cpp
$(object.path)/mia-resource.o: $(mia.path)/resource/resource.cpp

cmdsdl.path = ../cmdsdl

cmdsdl.objects += cmdsdl cmdsdl-input cmdsdl-emulator cmdsdl-settings cmdsdl-program
cmdsdl.objects := $(cmdsdl.objects:%=$(object.path)/%.o)
cmdsdl.options += -LSDL2 -lSDL2main -lSDL2

$(object.path)/cmdsdl.o: $(cmdsdl.path)/cmdsdl.cpp
$(object.path)/cmdsdl-input.o: $(cmdsdl.path)/input/input.cpp
$(object.path)/cmdsdl-emulator.o: $(cmdsdl.path)/emulator/emulator.cpp
$(object.path)/cmdsdl-program.o: $(cmdsdl.path)/program/program.cpp
$(object.path)/cmdsdl-settings.o: $(cmdsdl.path)/settings/settings.cpp


all.objects :=            $(libco.objects) $(sljit.objects) $(libchdr.objects) $(tzxfile.objects) $(ymfm.objects) $(nall.objects) $(ruby.objects) $(hiro.objects) $(ares.objects) $(mia.objects) $(cmdsdl.objects)
all.options := $(options) $(libco.options) $(sljit.options) $(libchdr.options) $(tzxfile.options) $(ymfm.options) $(nall.options) $(ruby.options) $(hiro.options) $(ares.options) $(mia.options) $(cmdsdl.options)

$(all.objects): | $(object.path)

all: $(all.objects) | $(output.path)
	$(info Linking $(output.path)/$(name)$(extension) ...)
	$(compiler) $(call exe,$(output.path)/$(name)$(extension)) $(all.objects) $(all.options)


verbose: nall.verbose ruby.verbose hiro.verbose all;

clean:
	$(call rdelete,$(object.path))
	$(call rdelete,$(output.path))

install: all
ifeq ($(platform),windows)
	$(call mkdir,$(prefix)/$(name)/)
else ifeq ($(shell id -un),root)
	$(error "make install should not be run as root")
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/hicolor/256x256/apps/
	mkdir -p $(prefix)/share/$(name)/Shaders/
	mkdir -p $(prefix)/share/$(name)/Database/
	cp $(output.path)/$(name) $(prefix)/bin/$(name)
	cp -R $(ares.path)/Shaders/* $(prefix)/share/$(name)/Shaders/
	cp -R $(mia.path)/Database/* $(prefix)/share/$(name)/Database/
	cp resource/$(name).desktop $(prefix)/share/applications/$(name).desktop
	cp resource/$(name).png $(prefix)/share/icons/hicolor/256x256/apps/$(name).png
endif

uninstall:
ifeq ($(platform),windows)
else ifeq ($(shell id -un),root)
	$(error "make uninstall should not be run as root")
else ifeq ($(platform),macos)
	rm -rf /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	rm -f $(prefix)/bin/$(name)
	rm -f $(prefix)/share/applications/$(name).desktop
	rm -f $(prefix)/share/icons/hicolor/256x256/apps/$(name).png
endif

-include $(object.path)/*.d

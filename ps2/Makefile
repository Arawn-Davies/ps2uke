# ps2uke -- PlayStation 2 build of JFDuke3D (jfbuild software renderer + SDL2).
#
# RENDERTYPE=SDL, USE_OPENGL=0, USE_POLYMOST=0: the classic 8-bit software
# engine driven by jfbuild's SDL2 baselayer (sdlayer2.c). PS2's SDL2 renders
# through gsKit and libSDL2main does the IOP/SIF bring-up + calls main().
#
#   ../build.sh        (from repo root)   or   make   (inside the image)

EE_BIN = ps2uke.elf

JFB    = ../jfbuild/src
JFBINC = ../jfbuild/include
GAME   = ../src
MACT   = ../jfmact
AUD    = ../jfaudiolib/src
AUDINC = ../jfaudiolib/include
PLAT   = platform
OPL    = opl
OBJDIR = build

# jfbuild engine (software-only) + SDL2 layer.
JFB_SRC  = a-c asmprot baselayer cache1d compat crc32 defs engine kplib \
           mmulti_null osd pragmas scriptfile textfont talltextfont \
           smalltextfont sdlayer2 startwin_stub
# JFDuke3D game (app_main is in game.c; skip editor/desktop-launcher TUs).
GAME_SRC = game actors config gamedef global grpscan menues osdcmds osdfuncs \
           player premap rts sector sounds version
# Input/control + math helpers.
MACT_SRC = animlib control file_lib keyboard mathutil mouse scriplib util_lib
# Audio: MultiVoc SFX out via PS2 audsrv (driver_audsrv). Music is the OPL synth
# (OPL_SRC below), mixed into the audsrv feed thread -- jfaudiolib's MIDI path is
# bypassed, so its midi/music TUs stay but their driver is unused on PS2.
AUD_SRC  = asssys cd drivers fx_man midi mix mixst multivoc music pitch \
           driver_nosound driver_audsrv
# OPL FM-synth music (lifted from ps2oom): DBOPL core + MIDI->OPL sequencer,
# rendered on demand by the audsrv feed thread. See ps2/opl/.
OPL_SRC  = dbopl opl_queue opl opl_ps2 midifile memio mus2mid i_oplmusic \
           demo_genmidi
# Our PS2 glue: cdfs/GRP file shim, build stubs, DualShock input, gsKit present,
# shared options (argv/memory card) and the quit->launcher reboot. The options
# *picker* lives in the separate launcher ELF (ps2/launcher/), not here.
PLAT_SRC = ps2_fileio ps2_stubs ps2_pad ps2_gs ps2_settings ps2_reboot

EE_OBJS = $(addprefix $(OBJDIR)/jfb_,  $(addsuffix .o,$(JFB_SRC)))  \
          $(addprefix $(OBJDIR)/game_, $(addsuffix .o,$(GAME_SRC))) \
          $(addprefix $(OBJDIR)/mact_, $(addsuffix .o,$(MACT_SRC))) \
          $(addprefix $(OBJDIR)/aud_,  $(addsuffix .o,$(AUD_SRC)))  \
          $(addprefix $(OBJDIR)/opl_,  $(addsuffix .o,$(OPL_SRC)))  \
          $(addprefix $(OBJDIR)/plat_, $(addsuffix .o,$(PLAT_SRC)))

EE_CFLAGS  += -std=gnu99 -D_EE -DUSE_POLYMOST=0 -DUSE_OPENGL=0 -fno-strict-aliasing -DHAVE_AUDSRV
EE_INCS    += -I$(GAME) -I$(JFBINC) -I$(MACT) -I$(AUDINC) -I$(PLAT) -I$(OPL) \
              -I$(PS2SDK)/ports/include/SDL2 -I$(PS2SDK)/ports/include \
              -I$(GSKIT)/include
EE_LDFLAGS += -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib

# SDL2 (+ main = PS2 entry/IOP init) over gsKit; ps2_drivers loads cdfs.irx.
EE_LIBS = -lSDL2main -lSDL2 -lps2_drivers \
          -lgskit -ldmakit -ldraw -lgraph -lmath3d -lpacket2 -ldma \
          -ldebug -lfileXio -lpatches -lpad -laudsrv -lmc \
          -lm -lc -lkernel

all: $(EE_BIN)

# sdlayer2's main() must be SDL_main so libSDL2main's PS2 entry (which does the
# IOP reset + sbv_patch_enable_lmb, needed for cdfs.irx) becomes the real main().
$(OBJDIR)/jfb_sdlayer2.o: $(JFB)/sdlayer2.c | $(OBJDIR)
	$(EE_C_COMPILE) -Dmain=SDL_main -c $< -o $@

# Hot software-renderer TUs at -O3 + loop unrolling (the trailing -O3 overrides the
# default -O2). a-c.c = the BUILD rasterizer inner loops (vline/hline/mvline);
# engine.c = drawrooms + the scan setup. Perf experiment; reversible.
$(OBJDIR)/jfb_a-c.o:    $(JFB)/a-c.c    | $(OBJDIR)
	$(EE_C_COMPILE) -O3 -funroll-loops -c $< -o $@
$(OBJDIR)/jfb_engine.o: $(JFB)/engine.c | $(OBJDIR)
	$(EE_C_COMPILE) -O3 -funroll-loops -c $< -o $@

$(OBJDIR)/jfb_%.o:  $(JFB)/%.c  | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@
$(OBJDIR)/game_%.o: $(GAME)/%.c | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@
$(OBJDIR)/mact_%.o: $(MACT)/%.c | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@
$(OBJDIR)/aud_%.o:  $(AUD)/%.c  | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@
$(OBJDIR)/opl_%.o:  $(OPL)/%.c  | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@
$(OBJDIR)/plat_%.o: $(PLAT)/%.c | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

# ps2uke -- PlayStation 2 build of Chocolate Duke3D.
#
# Builds the engine + game with our PS2 platform layer (gsKit video, cdfs file
# I/O, libpad input) replacing display.c/SDL. main() is Game/src/game.c.
#
#   ../build.sh         (from repo root)   or   make   (inside the image)

EE_BIN = ps2uke.elf

ENG    = ../Engine/src
GAME   = ../Game/src
PLAT   = platform
OBJDIR = build

# Engine: skip display.c (-> ps2_display.c) and the real multiplayer
# mmulti.c/multi.c (-> dummy_multi.c).
ENG_SRC  = $(filter-out display.c mmulti.c multi.c, $(notdir $(wildcard $(ENG)/*.c)))
# Game: all top-level TUs (audiolib/ and midi/ are replaced by dummy_audiolib.c).
GAME_SRC = $(notdir $(wildcard $(GAME)/*.c))
# Our PS2 platform layer.
PLAT_SRC = ps2_display.c ps2_input.c ps2_pad.c ps2_fileio.c ps2_audio_stub.c ps2_bootscr.c

EE_OBJS = $(addprefix $(OBJDIR)/, $(ENG_SRC:.c=.o) $(GAME_SRC:.c=.o) $(PLAT_SRC:.c=.o))

# -std=gnu99: GCC 15 defaults to C23 (reserves `bool` etc.). -fpermissive +
# -fcommon: this 1996 code puns int<->pointer and has tentative defs in headers.
EE_CFLAGS  += -DPLATFORM_PS2 -std=gnu99 -fpermissive -fcommon -fno-strict-aliasing
EE_INCS    += -I$(ENG) -I$(GAME) -I$(PLAT) -I$(PLAT)/sdl_stub \
              -I$(GSKIT)/include -I$(PS2SDK)/ports/include
EE_LDFLAGS += -L$(GSKIT)/lib -L$(PS2SDK)/ports/lib

EE_LIBS = -lgskit -ldmakit -lgraph -ldma \
          -lps2_drivers -lpatches -lfileXio -lpad -ldebug \
          -lc -lkernel

all: $(EE_BIN)

$(OBJDIR)/%.o: $(ENG)/%.c  | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@
$(OBJDIR)/%.o: $(GAME)/%.c | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@
$(OBJDIR)/%.o: $(PLAT)/%.c | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

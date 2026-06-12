# ps2uke -- PlayStation 2 build of Duke Nukem 3D (BUILD engine).
#
# Stage 2 milestone: compile the portable BUILD engine (source/buildengine/) +
# the PS2 driver and link a smoke-test ELF with the ps2dev toolchain. The Duke
# game objects (source/*.c) get added in a later stage -- see PORTING.md.
#
# Build inside the ps2dev toolchain:   ../build.sh        (from repo root)
#                              or:      make              (already in the image)

EE_BIN = ps2uke.elf

BE  = ../source/buildengine
OBJDIR = build

# Portable BUILD engine core (asm-free C) + the PS2 platform driver.
ENGINE_SRCS = engine.c a.c pragmas.c cache1d.c ps2_driver.c

EE_OBJS = $(addprefix $(OBJDIR)/, $(ENGINE_SRCS:.c=.o)) $(OBJDIR)/ps2_main.o

# PLATFORM_PS2 selects ps2_compat.h in platform.h and the PS2 paths everywhere.
# -fno-strict-aliasing: BUILD type-puns heavily (same caveat as the SDL build).
EE_CFLAGS  += -DPLATFORM_PS2 -fno-strict-aliasing
EE_INCS    += -I$(BE) -I$(GSKIT)/include -I$(PS2SDK)/ports/include
EE_LDFLAGS += -L$(GSKIT)/lib -L$(PS2SDK)/ports/lib

EE_LIBS = -lgskit -ldmakit -lgraph -ldma -lc

all: $(EE_BIN)

# Engine sources live one dir up; compile them into build/ with the sdk's
# finalized EE_C_COMPILE (defined by Makefile.eeglobal, included below).
$(OBJDIR)/%.o: $(BE)/%.c | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(EE_C_COMPILE) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

# ps2uke boot launcher (LAUNCH.ELF) -- the options picker that chain-loads the
# game ELF. Standalone: libdebug (text screen) + libpad (controller) + libmc
# (memory card). No SDL, no engine. Built by ../../build.sh alongside the game.
#
#   make            -> launcher.elf
#   make clean

EE_BIN  = launcher.elf
EE_OBJS = launcher_main.o ps2_settings.o

EE_CFLAGS += -D_EE -std=gnu99
EE_INCS   += -I../platform
EE_LIBS    = -ldebug -lpad -lmc

all: $(EE_BIN)

# ps2_settings.c is shared with the game ELF (lives in ../platform).
ps2_settings.o: ../platform/ps2_settings.c
	$(EE_C_COMPILE) -c $< -o $@
launcher_main.o: launcher_main.c
	$(EE_C_COMPILE) -c $< -o $@

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

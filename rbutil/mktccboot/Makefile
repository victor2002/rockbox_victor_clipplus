#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 31257 2011-12-14 21:59:37Z bluebrother $
#

# We use the Telechips code available in the Rockbox tools/ directory
TOOLSDIR = ../../tools/
CFLAGS := -O -g -W -Wall -Wshadow -pedantic -I$(TOOLSDIR)

OUTPUT = mktccboot


# inputs
LIBSOURCES := mktccboot.c $(TOOLSDIR)telechips.c
SOURCES := $(LIBSOURCES) main.c
EXTRADEPS :=

include ../libtools.make

# rule for sources from tools dir
# Rules go _after_ including mkboot.make to have OBJDIR set up correctly.
# Paths are assumed to end with a /
telechips.o: $(OBJDIR)telechips.o
$(OBJDIR)%.o: $(TOOLSDIR)%.c
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) -c -o $@ $<



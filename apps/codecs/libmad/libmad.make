#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: libmad.make 28913 2010-12-27 17:27:59Z kugel $
#

# we need to build two different mad libraries
# (one for codec, one for mpegplayer)
# so a little trickery is necessary

# Extract optimization level ('-O') from compile flags. Will be set later.
MADFLAGS = $(filter-out -O%,$(CODECFLAGS)) -I$(APPSDIR)/codecs/libmad
MADFLAGS += -UDEBUG -DNDEBUG -DHAVE_LIMITS_H

# libmad is faster on ARM-targets with -O1 than -O2
ifeq ($(CPU),arm)
   MADFLAGS += -O1
else
   MADFLAGS += -O2
endif

# MPEGplayer
MPEGMADFLAGS = $(MADFLAGS) -DMPEGPLAYER

# libmad
MADLIB := $(CODECDIR)/libmad.a
MADLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libmad/SOURCES)
MADLIB_OBJ := $(call c2obj, $(MADLIB_SRC))
OTHER_SRC += $(MADLIB_SRC)

$(MADLIB): $(MADLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

# libmad-mpeg
MPEGMADLIB := $(CODECDIR)/libmad-mpeg.a
MPEGMADLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libmad/SOURCES)
MPEGMADLIB_OBJ := $(addsuffix .o,$(basename $(subst $(ROOTDIR)/apps/codecs/libmad,$(BUILDDIR)/apps/codecs/libmad-mpeg,$(MPEGMADLIB_SRC))))

$(MPEGMADLIB): $(MPEGMADLIB_OBJ)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

# pattern rules

$(CODECDIR)/libmad-mpeg/%.o : $(ROOTDIR)/apps/codecs/libmad/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<)) \
		$(CC) $(MPEGMADFLAGS) -c $< -o $@

$(CODECDIR)/libmad-mpeg/%.o : $(ROOTDIR)/apps/codecs/libmad/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<)) \
		$(CC) $(MPEGMADFLAGS) $(ASMFLAGS) -c $< -o $@

$(CODECDIR)/libmad/%.o: $(ROOTDIR)/apps/codecs/libmad/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<)) \
		$(CC) $(MADFLAGS) -c $< -o $@

$(CODECDIR)/libmad/%.o: $(ROOTDIR)/apps/codecs/libmad/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<)) \
		$(CC) $(MADFLAGS) $(ASMFLAGS) -c $< -o $@

#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: libwavpack.make 30330 2011-08-18 16:33:41Z nls $
#

# libwavpack
WAVPACKLIB := $(CODECDIR)/libwavpack.a
WAVPACKLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libwavpack/SOURCES)
WAVPACKLIB_OBJ := $(call c2obj, $(WAVPACKLIB_SRC))
OTHER_SRC += $(WAVPACKLIB_SRC)

$(WAVPACKLIB): $(WAVPACKLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

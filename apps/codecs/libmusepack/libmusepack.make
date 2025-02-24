#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: libmusepack.make 30330 2011-08-18 16:33:41Z nls $
#

# libmusepack
MUSEPACKLIB := $(CODECDIR)/libmusepack.a
MUSEPACKLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libmusepack/SOURCES)
MUSEPACKLIB_OBJ := $(call c2obj, $(MUSEPACKLIB_SRC))
OTHER_SRC += $(MUSEPACKLIB_SRC)

$(MUSEPACKLIB): $(MUSEPACKLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: libspc.make 30330 2011-08-18 16:33:41Z nls $
#

# libspc
SPCLIB := $(CODECDIR)/libspc.a
SPCLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libspc/SOURCES)
SPCLIB_OBJ := $(call c2obj, $(SPCLIB_SRC))
OTHER_SRC += $(SPCLIB_SRC)

$(SPCLIB): $(SPCLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

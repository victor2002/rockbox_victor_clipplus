#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: libwmapro.make 30330 2011-08-18 16:33:41Z nls $
#

# libwmapro
WMAPROLIB := $(CODECDIR)/libwmapro.a
WMAPROLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libwmapro/SOURCES)
WMAPROLIB_OBJ := $(call c2obj, $(WMAPROLIB_SRC))
OTHER_SRC += $(WMAPROLIB_SRC)

$(WMAPROLIB): $(WMAPROLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: libtremor.make 30330 2011-08-18 16:33:41Z nls $
#

# libtremor
TREMORLIB := $(CODECDIR)/libtremor.a
TREMORLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libtremor/SOURCES)
TREMORLIB_OBJ := $(call c2obj, $(TREMORLIB_SRC))
OTHER_SRC += $(TREMORLIB_SRC)

$(TREMORLIB): $(TREMORLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

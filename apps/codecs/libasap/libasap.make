#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: libasap.make 30330 2011-08-18 16:33:41Z nls $
#

# libasap
ASAPLIB := $(CODECDIR)/libasap.a
ASAPLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libasap/SOURCES)
ASAPLIB_OBJ := $(call c2obj, $(ASAPLIB_SRC))
OTHER_SRC += $(ASAPLIB_SRC)

$(ASAPLIB): $(ASAPLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

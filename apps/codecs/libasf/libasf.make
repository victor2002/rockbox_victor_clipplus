#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: libasf.make 30330 2011-08-18 16:33:41Z nls $
#

# libasf
ASFLIB := $(CODECDIR)/libasf.a
ASFLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libasf/SOURCES)
ASFLIB_OBJ := $(call c2obj, $(ASFLIB_SRC))
OTHER_SRC += $(ASFLIB_SRC)

$(ASFLIB): $(ASFLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

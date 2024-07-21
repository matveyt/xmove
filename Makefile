BUILDTYPE ?= minsize
NOSTDLIB ?= off
UNICODE ?= on
include yama.mk

define NOSTDLIB.on
    LDFLAGS += -nostdlib
    LDLIBS += -lkernel32 -luser32 -lshell32
    $$(call yama.goalExe,xmove,../nocrt0/nocrt0c)
endef
define UNICODE.on
    CPPFLAGS += -D_UNICODE -DUNICODE
    LDFLAGS += -municode
endef
$(call yama.options,NOSTDLIB UNICODE)

all: xmove
$(call yama.goalExe,xmove,xmove)
$(call yama.rules)

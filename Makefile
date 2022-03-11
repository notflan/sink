# `sink` - sinks all input to /dev/null
# Usage: `sink [<program> [<args...>]]`
#
# Targets:
# `sink` (default): stripped `release` target. output: `sink`
# `release`: optimised build. output: `sink-release`
# `debug`: unoptimised build with debuginfo. output: `sink-debug`
# `clean`: remove all previous object and binary output files
#
# Environment variables:
# * `SHARED` - set to "yes" to prevent passing `--static` to cc
# * `TARGET_ARCH` - target arch (`-march=`), or set `TARGET_ARCH=` to force set to generic target. (default [release only]: `native`)
# * `CFLAGS`, `COMMON_FLAGS` - passthrough to cc
# * `LDFLAGS` - passthrough to ld
# * `DEBUG_CFLAGS`, `DEBUG_LDFLAGS` - passthrough to cc / ld on target `debug`
# * `RELEASE_CFLAGS`, `RELEASE_LDFLAGS` - passthrough to cc / ld on target `release`
#
# Make overridable only:
# * `STRIP` - Strip command for default output (stripped release) target (`make sink`). set `make STRIP=:` to prevent stripping entirely. (NOTE: When using `make install`, `STRIP=:` will not work, instead, paradoxically, set `STRIP=true`, they have the same effect for all targets)
PROJECT=sink
DESCRIPTION=sink all input and output of a program to /dev/null
VERSION=0.1.0
AUTHOR=Avril <avril@cumallover.me>
LICENSE=GPL3+

SRC=$(wildcard *.c)
OUTPUT=$(PROJECT)

ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

FEATURES?=

ifneq ($(FEATURES),)
	FEATURES:=$(addprefix -DFEATURE_,$(FEATURES))
endif

COMMON_FLAGS+= -W -Wall -Wextra -Wstrict-aliasing -fno-strict-aliasing "-D_AUTHOR=\"$(AUTHOR)\"" "-D_LICENSE=\"$(LICENSE)\"" "-D_VERSION=\"$(VERSION)\"" "-D_PROJECT=\"$(PROJECT)\"" "-D_DESCRIPTION=\"$(DESCRIPTION)\"" $(FEATURES)

TARGET_ARCH?=native
ifneq ($(TARGET_ARCH),)
	TARGET_ARCH:=$(addprefix -march=,$(TARGET_ARCH))
endif
RELEASE_CFLAGS+= $(TARGET_ARCH) -fgraphite \
	    -floop-interchange -ftree-loop-distribution -floop-strip-mine -floop-block \
	    -fno-stack-check

RELEASE_CFLAGS+= -O3 -flto -DRELEASE
RELEASE_LDFLAGS+= -Wl,-O3 -Wl,-flto

DEBUG_CFLAGS+=-Og -g3 -ggdb -gz -DDEBUG
DEBUG_LDFLASG+=-Wl,-g

CFLAGS   += $(COMMON_FLAGS) --std=gnu17
LDFLAGS  +=

ifneq ($(SHARED),yes)
	CFLAGS+=--static
endif

OBJ   = $(SRC:.c=.o)

STRIP=strip


# Phonies

.PHONY: $(OUTPUT)
$(OUTPUT): $(PROJECT)-release
	mv $< $@
	$(STRIP) $@	


.PHONY: release
release: $(PROJECT)-release

.PHONY: debug
debug: $(PROJECT)-debug

# Targets

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@ $(LDFLAGS)

$(PROJECT)-debug: CFLAGS+=$(DEBUG_CFLAGS)
$(PROJECT)-debug: LDFLAGS+=$(DEBUG_LDFLAGS)
$(PROJECT)-debug: $(OBJ)
	$(CC) $^ -fwhole-program $(CFLAGS) -o $@ $(LDFLAGS)


$(PROJECT)-release: CFLAGS+=$(RELEASE_CFLAGS)
$(PROJECT)-release: LDFLAGS+=$(RELEASE_LDFLAGS)
$(PROJECT)-release: $(OBJ)
	$(CC) $^ -fwhole-program $(CFLAGS) -o $@ $(LDFLAGS)

clean:
	rm -f *.o
	rm -f $(OUTPUT) $(PROJECT)-*


.PHONY: install
install: 
	install -s --strip-program=$(STRIP) -m 755 $(OUTPUT) $(DESTDIR)$(PREFIX)/bin

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(OUTPUT)

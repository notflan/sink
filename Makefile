# Sinks all input to /dev/null
PROJECT=sink

SRC=$(wildcard *.c)
OUTPUT=$(PROJECT)

COMMON_FLAGS+= -W -Wall -Wextra -Wstrict-aliasing -fno-strict-aliasing

TARGET_ARCH?=native
ifneq ($(TARGET_ARCH),)
	TARGET_ARCH:=$(addprefix -march=,$(TARGET_ARCH))
endif
RELEASE_CFLAGS+= $(TARGET_ARCH) -fgraphite -fopenmp -floop-parallelize-all -ftree-parallelize-loops=4 \
	    -floop-interchange -ftree-loop-distribution -floop-strip-mine -floop-block \
	    -fno-stack-check

RELEASE_CFLAGS+= -O3 -flto
RELEASE_LDFLAGS+= -Wl,-O3 -Wl,-flto

DEBUG_CFLAGS+=-Og -g3 -ggdb -gz
DEBUG_LDFLASG+=-Wl,-g

CFLAGS   += $(COMMON_FLAGS) --std=gnu17
LDFLAGS  +=

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


	

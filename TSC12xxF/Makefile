PCSC_CFLAGS := $(shell pkg-config --cflags libpcsclite)
LDFLAGS := $(shell pkg-config --libs libpcsclite)

CFLAGS += $(PCSC_CFLAGS)

pcsc_demo: pcsc_demo.c

clean:
	rm -f sample


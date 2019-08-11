CFLAGS := -Wall -Werror -Wextra -Wno-unused $(shell pkg-config --cflags libhandy-0.0 evince-view-3.0 libcurl json-glib-1.0)
LDFLAGS := $(shell pkg-config --libs libhandy-0.0 evince-view-3.0 libcurl json-glib-1.0) -lm

OBJ_FILES := librevfr.o librevfr-resources.o docs.o nav.o tools.o aircraft.o \
			 checklist.o flight.o utils.o provider.o provider-sia.o \
			 provider-basulm.o terrain.o

%o%c:
	$(CC) $(CFLAGS) -c $< -o $@

all: $(OBJ_FILES)
	$(CC) $(LDFLAGS) $(OBJ_FILES) -o ../librevfr

clean:
	@rm -f $(OBJ_FILES) librevfr-resources.* ../librevfr

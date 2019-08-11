all: librevfr-resources.c librevfr-resources.h
	cp librevfr-resources.c librevfr-resources.h ../src

librevfr-resources.c: librevfr.ui sample-aircraft.json sample-aircraft_sample-checklist.json
	glib-compile-resources --generate --target=librevfr-resources.c librevfr.gresources.xml
	glib-compile-resources --generate --target=librevfr-resources.h librevfr.gresources.xml

clean:
	@rm -f librevfr-resources.*

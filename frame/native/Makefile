CC=gcc -Wall

PAIRS=frame.xx
CFILES=$(PAIRS:.xx=.c)
HFILES=$(PAIRS:.xx=.h)
OFILES=$(PAIRS:.xx=.o)

JAVA=Test.java
CLASSFILES=$(JAVA:.java=.class)

LIB=/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers/

all: libframeintrospect.dylib


$(OFILES): %.o: %.c
	$(CC) -G -fPIC $< -o $@ -c -I$(LIB) -I /Users/qianpanwang/code/jvm-frame/frame/build/native

libframeintrospect.dylib: $(OFILES)
	$(CC) -dynamiclib -undefined suppress -flat_namespace $(OFILES) -o libframeintrospect.dylib


clean:
	rm -f $(OFILES) libframeintrospect.dylib

.PHONY: all clean run

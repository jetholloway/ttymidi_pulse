WARNING_FLAGS := -Wall -Wcast-align -Wcast-align -Wextra -Wfloat-equal -Winit-self -Winline -Wmissing-declarations -Wmissing-include-dirs -Wno-long-long -Wpointer-arith -Wredundant-decls -Wredundant-decls -Wshadow -Wswitch-default -Wswitch-enum -Wundef -Wuninitialized -Wunreachable-code -Wwrite-strings

OPTIMIZATION_FLAGS := -march=corei7 -fexpensive-optimizations -Os

CFLAGS  := $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) $(STD_FLAG)

all:
	$(CC) src/ttymidi.c -o ttymidi -lasound -pthread $(CFLAGS)

clean:
	rm ttymidi

install:
	mkdir -p $(DESTDIR)/usr/bin
	cp ttymidi $(DESTDIR)/usr/bin

uninstall:
	rm $(DESTDIR)/usr/bin/ttymidi

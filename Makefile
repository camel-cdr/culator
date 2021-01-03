.POSIX:

culator: culator.c functions.c arg.h
	${CC} culator.c -o $@  -Wall -Wextra -Os -lm

install: culator
	cp -f culator /usr/local/bin/culator
	chmod 755 /usr/local/bin/culator

uninstall:
	rm -rf /usr/local/bin/culator

clean:
	rm -f culator

.PHONY: install uninstall clean

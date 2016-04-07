
OS := $(shell uname -s)
include Makefiles/$(OS).mk

default: hindsight.exe socket.exe window.exe nttime.exe

CLEANS += hindsight.exe
hindsight.exe: hindsight.o command.o
	$(CC) $(CFLAGS) -o $@ $< -ltcl

CLEANS += socket.exe
socket.exe: socket.o
	$(CC) $(CFLAGS) -o socket.exe socket.o -lws2_32

CLEANS += window.exe
window.exe: window.o
	$(CC) $(CFLAGS) -o window.exe window.o

CLEANS += nttime.exe
nttime.exe: nttime.o
	$(CC) $(CFLAGS) -o nttime.exe nttime.o -lntdll

clean:
	rm -f $(CLEANS) *.o

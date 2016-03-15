
CC=/usr/local/gcc-4.8.0-qt-4.8.4-for-mingw32/win32-gcc/bin/i586-mingw32-gcc
# CFLAGS+=-O2

default: hindsight.exe socket.exe window.exe nttime.exe

CLEANS += hindsight.exe
hindsight.exe: hindsight.o
	$(CC) $(CFLAGS) -o hindsight.exe hindsight.o

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

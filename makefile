CC=gcc
CFLAGS=-Wall -Wextra -O2

all: giflib gif2bmp

giflib: 
	$(CC) $(CFLAGS) -c gif.c -o gif.o
	
gif2bmp: giflib
	$(CC) $(CFLAGS) gif2bmp.c gif.o -o gif2bmp

clean:
	rm *.o

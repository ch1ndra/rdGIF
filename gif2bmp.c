#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "gif.h"
#include "bmp.c"


int main(int argc, char *argv[])
{
gif_t *     gif;
uint32_t *  buf;

    if( argc != 2 ) {
      printf("Usage: gif2bmp <giffile>\n");
      return -1;
    }
    
    gif = gif_open(argv[1]);
    if( gif == NULL ) {
      printf("\nError: gif_open(%s)\n", argv[1]);
      return 1;
    }
    
    buf = malloc(gif->width * gif->height * 4);
    gif_read(buf, gif->width, gif->height, gif);
    bmp_write("output.bmp", buf, gif->width, gif->height);
    gif_close(gif);
    free(buf);
    
    return 0;
}

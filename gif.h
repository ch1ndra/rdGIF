#ifndef __GIF_H
#define __GIF_H

#include <stdint.h>
#include <stdio.h>

typedef struct
{
  char      signature[3];
  char      version[3];
} __attribute__((packed))
GIF_header;

typedef struct
{
  uint16_t  canvas_width;
  uint16_t  canvas_height;
  uint8_t   packed_field;
  uint8_t   background_color_index;
  uint8_t   pixel_aspect_ratio;
}  __attribute__((packed))
GIF_LSD;  /* Logical Screen Descriptor */


typedef struct
{
  /* uint8_t   extension_introducer; */
  /* uint8_t   graphic_control_label; */
  uint8_t   block_size;
  uint8_t   packed_field;
  uint16_t  delay_time;
  uint8_t   transparent_color_index;
  uint8_t   block_terminator;
} __attribute__((packed))
GIF_GCE;  /* Graphics Control Extension */


typedef struct
{
  /* uint8_t   extension_introducer; */
  /* uint8_t   application_extension_label; */
  uint8_t   block_size;
  char      id[11];
} __attribute__((packed))
GIF_APPEXT; /* Application Extension */


typedef struct
{
  /* uint8_t   image_separator; */
  uint16_t  image_left;
  uint16_t  image_top;
  uint16_t  image_width;
  uint16_t  image_height;
  uint8_t   packed_field;
} __attribute__((packed))
GIF_IMGDSC;  /* Image Descriptor */


typedef struct
{
    FILE   *   fp;
    uint8_t *  GCT;
    uint8_t    background_color_index;
    uint16_t   delay;                /* Duration to wait (ms) before rendering the next image */
    uint16_t   width;                /* Width of the GIF canvas */
    uint16_t   height;              /* Height of the GIF canvas */
} 
gif_t; 


gif_t *   gif_open(const char * GIFfile);
int8_t    gif_read( uint32_t * surface, uint16_t surface_width, uint16_t surface_height, gif_t * gif);
void      gif_close(gif_t * gif);


#endif

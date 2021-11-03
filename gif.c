#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include "gif.h"
#include "codetable.c"

#define pow2( x) ( (long)1 << (x) )


static void * memset32(void *__dest, int32_t __c, size_t __n)
{
int32_t * dp = (int32_t *)__dest;

	for( ; __n != 0; __n--) 
        *dp++ = __c;

return __dest;
}


uint16_t read_code(uint16_t * codestream, uint16_t codesize, uint32_t bitindex) 
{
uint32_t  byteindex;
uint32_t  codebytes;
uint16_t  code;

  /* Let's assume the following bit stream:
   * 1[0]100010 10011001 10010001 ...
   * The square bracket indicates where to start reading from. Assume the specified codesize is 12
   * (which is the maximum allowed codesize). In this case, the correponding code spans three bytes
   * (2 most significant bits of the first byte, all bits of the second byte and two least significant
   * bits of the third byte */

  byteindex = bitindex/8;
  codebytes = *(uint32_t *)((uint8_t *)codestream + byteindex);
  /* Shift and mask */
  code = (codebytes >> (bitindex % 8)) & ((1 << codesize) - 1);

  return code;
}


void read_image_data(FILE * infp, STREAM * outstream)
{
uint8_t     min_code_size, smallest_code_size;
uint8_t     subblock_size;
uint16_t  * codestream;
uint8_t   * ptr;
CODETABLE * codetable;
uint16_t    code;
uint16_t    lastcode = CC;      /* initialized to get around the compiler warning: "'lastcode' may be used uninitialized in this function" */
uint8_t     lastcode_firstbyte;
uint32_t    bitindex;   
int         status;

    fread(&min_code_size, 1, 1, infp);
    // fprintf(stdout, "Minimum code size: %u\n", min_code_size);
    /* Actual smallest code size = minimum code size + 1 */
    smallest_code_size = min_code_size + 1;

    codetable = codetable_init(min_code_size);
    codestream = calloc(1<<16, 1);   /* = 64 Kilobytes (To do: allocate only as much memory as required) */
    bitindex = 0;

    /* Read all subblocks into the codestream */
    for(ptr = (uint8_t *)codestream; ; ptr += subblock_size)
    {
      fread(&subblock_size, 1, 1, infp);
      // fprintf(stdout, "Subblock size: %u\n", subblock_size);
      
      if(subblock_size == 0)
        break;

      fread(ptr, 1, subblock_size, infp);
    }

    while(1)
    {
        code = read_code(codestream, smallest_code_size, bitindex);
        bitindex += smallest_code_size;
        // fprintf(stdout, "code: %d\n", code);
        status = codetable_decode(codetable, code, outstream);
        if(status == 0)
        {
          // fprintf(stdout, "Code [%d] not found\n", code);
          lastcode_firstbyte = codetable_code_firstbyte(codetable, lastcode);
          codetable_decode(codetable, codetable_add(codetable, lastcode_firstbyte, lastcode), outstream);
        }
        else if(status == CC)
        {
          // fprintf(stdout, "Clear-Code! [offset: %u]\n", (uint16_t)(outstream->tail - outstream->head));
          /* Reset the smallest_code_size */
          smallest_code_size = min_code_size + 1;

          /* Reset the codetable */
          codetable_free(codetable);
          codetable = codetable_init(min_code_size);

          /* Read the immediate next code */
          code = read_code(codestream, smallest_code_size, bitindex);
          bitindex += smallest_code_size;
          // fprintf(stdout, "code: %d\n", code);
          codetable_decode(codetable, code, outstream); 
        }
        else if(status == EOI)
        {
          // fprintf(stdout, "End-Of-Information! [Image data reading complete!]\n");
          // codetable_dump(codetable);
          free(codestream);
          codetable_free(codetable);
          break;
        }
        else
        {
          codetable_add(codetable, codetable_code_firstbyte(codetable, code), lastcode);
        }

        if( (codetable->tail > pow2(smallest_code_size) - 1) && (smallest_code_size < 12) )
          smallest_code_size++;
        
        lastcode = code;
        // codetable_dump(codetable);
    }
    
}


int8_t gif_read( uint32_t * surface, uint16_t surface_width, uint16_t surface_height, gif_t * gif)
{
uint8_t  * LCT = NULL;
uint8_t  * color_table;    /* Pointer to the color table (GCT or LCT) */
uint8_t     LCT_size;
uint16_t    LCT_length;
uint8_t     type;
GIF_GCE     gce;
GIF_APPEXT  appext;
GIF_IMGDSC  imgdsc;
uint8_t     label;
uint8_t     subblock_size;
STREAM      outstream;
uint8_t   * ptr;
uint32_t    offset;
uint16_t    w, h;
int8_t      error = 0;

  /* Set the default color table */
  color_table = gif->GCT; 
  
  do
  {
    fread(&type, 1, 1, gif->fp);
    switch(type)
    {
      case 0x2C:   /* Image Descriptor */
        // fprintf(stdout, "Reading Image Descriptor ...\n");
        fread(&imgdsc, 1, sizeof(GIF_IMGDSC), gif->fp);
        
        /* Check for the the Local Color Table */
        if(imgdsc.packed_field>>7 & 0x01)
        {
          LCT_size = imgdsc.packed_field & 0x07;      
          LCT_length = 3*pow2(LCT_size+1);

          // fprintf(stdout, "Local Color Table found! [LCT length: %u]\n", LCT_length);
        
          LCT = malloc(LCT_length);
          fread(LCT, 1, LCT_length, gif->fp);
          color_table = LCT;
        }

        // fprintf(stdout, "Now reading image data ...\n");
        // fprintf(stdout, "Image width: %d | Image height: %d\n", imgdsc.image_width, imgdsc.image_height);
        
        outstream.head = outstream.tail = calloc(imgdsc.image_width*imgdsc.image_height, 1);
        read_image_data(gif->fp, &outstream);
        
        /* Handle appropriate disposal method before drawing the image */
        switch(gce.packed_field >> 2 & 0x7)
        {
            case 1:
              /* Draw the next image on the top of the current image */
              /* No action needed */
            break;
            
            case 2:
              /* Restore the canvas to the background color */
              memset32(surface, (uint32_t)(color_table[gif->background_color_index*3] << 16) | (color_table[gif->background_color_index*3+1] << 8 ) | color_table[gif->background_color_index*3+2], surface_width*surface_height);
            break;
            
            default:
              /* Do nothing */
            break;
        }
        if( (imgdsc.image_width <= surface_width) && (imgdsc.image_height <= surface_height) ) 
        {
            for(h = 1, ptr = outstream.head, offset = imgdsc.image_top*surface_width + imgdsc.image_left; h <= imgdsc.image_height; h++, offset += (surface_width - imgdsc.image_width) )
            {
                for(w = 1; w <= imgdsc.image_width; w++, ptr++, offset++ )
                {
                    /* Check if a transparent color is defined */
                    if( gce.packed_field & 1)
                    {
                        if(*ptr == gce.transparent_color_index)
                          continue;
                    }
                    surface[offset] =  (uint32_t)(color_table[*ptr*3] << 16) | (color_table[*ptr*3+1] << 8 ) | color_table[*ptr*3+2] ;    /* ARGB */ 
                }
            }
        }
        
        /* To do: scale the image to the dimensions of the surface */

        free(outstream.head);
        
        if(LCT) {
          free(LCT);
        }
        
        gif->delay = gce.delay_time*10;
        return 0;

      case 0x21:
        fread(&label, 1, 1, gif->fp);
        switch(label)
        {
          case 0xF9:
            // fprintf(stdout, "Reading Graphics Control Extension ...\n");
            fread(&gce, 1, sizeof(GIF_GCE), gif->fp);
          break;

          case 0xFF:
            // fprintf(stdout, "Reading Application Extension ...");
            fread(&appext, 1, sizeof(GIF_APPEXT), gif->fp);
            if(strncmp(appext.id, "NETSCAPE2.0", sizeof(appext.id)) == 0)
            {
              // fprintf(stdout, "[NETSCAPE2.0]\n");
              fseek(gif->fp, 5, SEEK_CUR);   /* To do: Parse fields */
            }
            else
            {
              // fprintf(stdout, "[Unknown Application]\n");
              fseek(gif->fp, -sizeof(GIF_APPEXT), SEEK_CUR);
              /* Skip */
              do
              {
                fread(&subblock_size, 1, 1, gif->fp);
                // fprintf(stdout, "Subblock size: %u\n", subblock_size);
                fseek(gif->fp, subblock_size, SEEK_CUR);
              } while(subblock_size != 0);
            }
          break;

          case 0xFE:
            // fprintf(stdout, "Skipping Comment Extension ...\n");
            /* Skip */
            do
            {
              fread(&subblock_size, 1, 1, gif->fp);
              fseek(gif->fp, subblock_size, SEEK_CUR);
            } while(subblock_size != 0);
          break;

          default:
            fprintf(stdout, "gif_read(): Error! [Extension not supported! [label: %x]]\n", label);
            error = -1;
          break;
        }
      break;

      case 0x3B:
        // fprintf(stdout, "End Marker!\n");
        error = 1;
      break;

      default:
        fprintf(stdout, "gif_read(): Error! [Unknown block type!]\n");
        error = -2;
      break;
    }
  } while(type != 0x3B && error == 0);

  return error;
}


gif_t * gif_open(const char * GIFfile)
{
gif_t     *  gif;
GIF_header  header;
GIF_LSD     lsd;
uint8_t     GCT_res __attribute__((unused));
uint8_t     GCT_size;
uint16_t    GCT_length;

  gif = malloc(sizeof(gif_t));
  if(!gif)
    return NULL;
    
  gif->fp = fopen(GIFfile,"rb");
  if(!gif->fp)
  {
    fprintf(stdout, "\nGIF image file '%s' does not exist!", GIFfile);
    free(gif);
    return NULL;
  }

  fread(&header, 1, sizeof(GIF_header), gif->fp);
  if( strncmp(header.signature, "GIF", 3) != 0 )
  {
    fprintf(stdout, "Not a valid GIF file!");
    fclose(gif->fp);
    free(gif);
    return NULL;
  }
  
  fread(&lsd, 1, sizeof(GIF_LSD), gif->fp);
  gif->width = lsd.canvas_width;
  gif->height = lsd.canvas_height;

 /* Check for the the Global Color Table */
  if(lsd.packed_field>>7 & 0x01)
  {
    GCT_res = (lsd.packed_field>>4) & 0x07;
    GCT_size = lsd.packed_field & 0x07;      
    GCT_length = 3*pow2(GCT_size+1);

    //fprintf(stdout, "Global Color Table found! [Color Resolution: %u | GCT length: %u]\n", GCT_res, GCT_length);

    gif->GCT = calloc(GCT_length, 1);
    fread(gif->GCT, 1, GCT_length, gif->fp);
  }
   
  return gif;
}


void gif_close(gif_t * gif)
{
    fclose(gif->fp);
    if(gif->GCT) {
      free(gif->GCT);
    }
    free(gif);
}


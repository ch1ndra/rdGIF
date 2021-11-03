#include <stdint.h>
#include <math.h>
#include "codetable.h"

#define pow2( x) ( (long)1 << (x) )

#define CC  0xFFFE
#define EOI 0xFFFF


CODETABLE * codetable_init(uint8_t min_code_size)
{
CODETABLE * codetable;
uint16_t    i;

  codetable = calloc(1, sizeof(CODETABLE));
  
  for(i=0; i<pow2(min_code_size); i++)
  {
    codetable->codeentry[i].byte = i;
    codetable->codeentry[i].len = 1;
  }

  codetable->codeentry[i].byte = CC;
  codetable->codeentry[i].len = 1;
  codetable->codeentry[++i].byte = EOI;
  codetable->codeentry[i].len = 1;

  codetable->tail = ++i;
  return codetable;
}


int codetable_decode(CODETABLE * codetable, uint16_t code, STREAM * outstream)
{
uint8_t * stack;
uint16_t  len;
uint16_t  i;

  if(codetable->codeentry[code].len == 0)
  {
    return 0;
  }
  
  else if(codetable->codeentry[code].len == 1)
  {
    switch(codetable->codeentry[code].byte)
    {
      case CC:
        return CC;

      case EOI:
        return EOI;

      default:
        *outstream->tail++ = codetable->codeentry[code].byte;
        // fprintf(stdout, "Output: [%d]\n", *(outstream->tail-1));
      break;
    }
    return 1;
  }
  
  else
  {
    len = codetable->codeentry[code].len;
    stack = malloc(len);
    do
    {
      i = codetable->codeentry[code].len - 1;
      stack[i] = codetable->codeentry[code].byte;
      code = codetable->codeentry[code].prev;
    } while(i > 0);
    /* Note: prev = 0 doesnot indicate that we have reached the end of the linked list.
     * This is because 0 corresponds to the first color in the GCT/LCT */
    
    memcpy(outstream->tail, stack, len);
    /* fprintf(stdout, "Output: [");
    for(i = 0; i < len; i++)
      fprintf(stdout, "%d ", outstream->tail[i]);
    fprintf(stdout, "]\n");
    */
    outstream->tail += len;
    free(stack);
    
    return len;
  }
}


uint16_t codetable_add(CODETABLE * codetable, uint16_t byte, uint16_t prev)
{
  if( codetable->tail < sizeof(codetable->codeentry)/sizeof(codetable->codeentry[0]) )
  {
    codetable->codeentry[codetable->tail].byte = byte;
    codetable->codeentry[codetable->tail].prev = prev;
    codetable->codeentry[codetable->tail].len = codetable->codeentry[prev].len + 1;

    return codetable->tail++;
  }

  fprintf(stdout, "codetable_add(): Error! [Codetable overflow!]\n");
  return codetable->tail;
}


uint16_t codetable_code_firstbyte(CODETABLE * codetable, uint16_t code)
{
  while(1)
  {
    if(codetable->codeentry[code].len == 1)
      break;
      
    code = codetable->codeentry[code].prev;
  }
  
  return(codetable->codeentry[code].byte);
}


void codetable_free(CODETABLE * codetable)
{
  free(codetable);
}


void codetable_dump(CODETABLE * codetable)
{
uint16_t i;

  for(i = 0; i < codetable->tail; i++)
    fprintf(stdout, "[index = %d] | byte: %d | prev: %d | len: %d\n", i, codetable->codeentry[i].byte, codetable->codeentry[i].prev, codetable->codeentry[i].len);
}

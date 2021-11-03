#ifndef __CODETABLE_H
#define __CODETABLE_H

typedef struct
{
  uint16_t  byte;
  uint16_t  prev;
  uint16_t  len;
}
CODEENTRY;


typedef struct
{
  CODEENTRY codeentry[4096];
  uint16_t  tail;   /* Index to the first vacant slot */
}
CODETABLE;


typedef struct
{
  uint8_t * head;
  uint8_t * tail;
}
STREAM;


CODETABLE * codetable_init(uint8_t min_code_size);
int         codetable_decode(CODETABLE * codetable, uint16_t code, STREAM * outstream);
uint16_t    codetable_add(CODETABLE * codetable, uint16_t color, uint16_t prev);
uint16_t    codetable_code_firstbyte(CODETABLE * codetable, uint16_t code);
void        codetable_free(CODETABLE * codetable);
void        codetable_dump(CODETABLE * codetable);

#endif

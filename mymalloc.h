#ifndef MYMALLOC_H
#define MYMALLOC_H

#define malloc(x) mymalloc(x, __FILE__, __LINE__)
#define free(x) myfree(x, __FILE__, __LINE__)

struct _meta { // Since this struct has only one member,
	       // an alternative would be to do something like
	       // typedef short meta;
  unsigned short info;
  //
  // layout of info:
  // 
  //          Byte 2        |        Byte 1
  // __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __
  // ^  ^
  // |  \_______
  // in_use    Size begins here
};


/*
 * To read in_use:
 * (info >> 15) & 1
 *
 * To set in_use to true:
 * info |= 1 << 15
 *
 * To set in_use to false:
 * info &= ~(1 << 15)
 *
 * To read size:
 * We need to AND info with 0111 1111 1111 1111
 * info & 0x7FFF
 *
 * To write to size:
 * WRONG:
 * First mask the new size to be sure we don't
 * overwrite the in_use bit
 * info = newsize & 0x7FFF
 * 
 * RIGHT:
 * We only want to change the lower 15 bits. We want
 * to leave the upper bit alone.
 *
 * Proceed as follows: First zero out the lower 15 bits.
 * To do this, AND info with 1000 0000 0000 0000
 * info = info & 0x8000
 *
 * Now take that result, and OR it with the (masked) new size.
 * Note we don't need to shift it, since it's the LOWER 15 bits.
 *
 * info = (info & 0x8000) | (new_size & 0x7FFF)
 */



enum _boolean {FALSE, TRUE};

typedef struct _meta meta;

typedef enum _boolean boolean;

int read_in_use(meta *m);

void set_in_use(meta *m);

void clear_in_use(meta *m);

unsigned short read_size(meta *m);

void write_size(meta *m, unsigned short new_size);

void bootstrap();

void *mymalloc(size_t size, char *filename, int line);

void myfree(void *p, char *filename, int line);

void info();

#endif

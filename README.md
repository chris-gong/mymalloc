# My Malloc
CS214 Rutgers Systems Programming Assignment 1: Making a better version of free and malloc by simulating virtual memory.

Runs on Rutgers iLab machines (Linux x86_64 bit architecture).

To run the program, simple call the make command
```
make
```
This will produce not only an object file of ```mymalloc``` but also a ```memgrind``` executable. To excute the executable, simply call
```
./memgrind
```
Note that ```memgrind.c``` just tests the malloc and free macros defined in ```mymalloc```. If you wish to implement our malloc and free in your program then replace all instances of ```memgrind``` in the Makefile with the name of your program. Also, make sure to include ```mymalloc.h``` in your program
```
#include "mymalloc.h"
```
Currently, our virtual memory is 5000 bytes but this can be modified by adjusting this line in `mymalloc.c`.
```
#define HEAP_SIZE 5000
```
Right now, the size of the meta blocks are 2 bytes each so this may limit the amount of memory that the simulated heap can hold. If the current struct cannot handle the requested HEAP_SIZE, then consider modifying our struct and bit-masking techniques that allowed us to hold both a size and in_use variable into one short in `mymalloc.c`
```
int read_in_use(meta *m) {
  return (m->info >> 15) & 1;
}

void set_in_use(meta *m) {
  m->info |= 1 << 15;
}

void clear_in_use(meta *m) {
  m->info &= ~(1 << 15);
}

unsigned short read_size(meta *m) {
  return m->info & 0x7FFF;
}

void write_size(meta *m, unsigned short new_size) {
  if (new_size > 0x7FFF) return; // TOO BIG
  m->info = (m->info & 0x8000) | (new_size & 0x7FFF);
}
```
and in `mymalloc.h`
```
struct _meta { 
  unsigned short info;
};
```
For more information on the implementation, check out the [readme.pdf](https://github.com/chris-gong/mymalloc/blob/master/readme.pdf) file. For more information on some of the test cases in `memgrind.c`, check out the [testcases.txt](https://github.com/chris-gong/mymalloc/blob/master/testcases.txt) file.

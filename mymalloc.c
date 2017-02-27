#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <errno.h>
#include "mymalloc.h"

#define HEAP_SIZE 5000
int errorInFree = 0;
int firstMalloc = 1;


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



static meta *global_meta = NULL;
static meta *last = NULL; // NOTE: could also use length-based calculation to find end

// Note: global_meta is not really necessary, since we know the first
// meta block is ALWAYS at the beginning of myblock

static char myblock[HEAP_SIZE];

void bootstrap() {
  // create the first meta block, managing the (5000-sizeof meta) bytes
  
  global_meta = (meta*) myblock;
  write_size(global_meta, HEAP_SIZE - sizeof(meta));
  clear_in_use(global_meta);
  last = (meta*) myblock;
}


void *mymalloc(size_t size, char *filename, int line) {

  // loop over list looking for free space such that:
  // -- size + sizeof(meta) fits
  // If necessary, we may need to give them back slightly more than
  // they asked for if we cannot fit both the requested size
  // and another meta block
 
  if (size == 0) {
    return NULL; // if the user mallocs 0, return NULL
  }

  if (firstMalloc) {
    //printf("First malloc!\n");
    bootstrap();
    firstMalloc = 0;
  }
 
  meta *ptr = global_meta, *smallest = NULL;
  boolean in_use = read_in_use(ptr);
  unsigned short block_size = read_size(ptr);
  boolean found_block = (in_use == FALSE) && (size + sizeof(meta) <= block_size);
  if (found_block) {
    smallest = ptr;
  }
  // on first malloc, loop will never execute but it's okay,
  // because loop is only needed to traverse multiple blocks
  while (ptr!=last) {
    // ptr is updated by first incrementing by 1 to get to the user data area,
    // then by casting that address to a char * and incrementing by the size
    // of the user data to get to the next meta block
    ptr = (meta*) ((char*) (ptr + 1) + block_size); // TODO make sure this works!!!!!!
    in_use = read_in_use(ptr);
    block_size = read_size(ptr);
    found_block = (in_use == FALSE) && (size + sizeof(meta) <= block_size);
    if (found_block && smallest == NULL) {
       smallest = ptr; // We've yet to find any block big enough to hold the resuest (until now)
		       // Since this block is the only one big enough, it is certainly
		       // the *smallest* block big enough!
    }
    else if (found_block && block_size < read_size(smallest)) {
       smallest = ptr; // we found a smaller block that can fit the requested size
    } 
    
  }
  if (smallest == NULL) { 
    printf("ERROR: No space found to hold a block of the requested size.\n"
    "Filename: %s, Line: %d\n\n", filename, line);
    return NULL;
  }

  /*printf("The smallest size that can accommodate the request is %u\n", 
    read_size(smallest));*/

  set_in_use(smallest);
 
  // Now cut up the remainder ONLY IF there's room for it
  // Only split if the new piece is bigger than sizeof(meta) <- replaced with 16
  size_t old_size = read_size(smallest); // the size of the chunk before the splitting
  if (old_size - size - sizeof(meta) < 16) {
     // don't split
     // smallest->size remains what it was b/c we don't split
     
     // NOTE: The following discussion assumes a meta block size of 16 bytes.
     // What is maximum extra space we can give them in the case of non-split: 15 + 16 = 31 (old_size - size)
     // 16 is size of meta block that would have resulted from split, 15 is 36 - 5 - 16
     // We use 5 bytes of the 36, for a total of 31 unused. Had we split, we'd have:
     //    meta   data     meta   data
     //  | [16] |   5   |  [16]  | 15 |
     //   
     // Versus:
     //
     //   5 + 31 = 36 (notice the BONUS of 31, making for a total of 36 user data bytes!)
     //
     // Old size is 36, requested size is 4:
     // 36 - 4 - 16 = 16. 16 not less than 16, so go to else and SPLIT
     // into a new chunk of size 16 >= 16
     // Assume requested size is instead 5:
     // 36 - 5 - 16 = 15. 15 IS less than 16, so do NOT split.
     // Assume requested size is instead 3:
     // 36 - 3 - 16 = 17. 17 not less than 16, so go to else and SPLIT
     // into a new chunk of size 17 >= 16
     
     /*printf("NOTICE: The remaining size %d - %d - %d is < %d. Thus we will NOT split,"
       "and you are getting a BONUS %d bytes\n", old_size, size, sizeof(meta),
        16, old_size - size);*/
  } else {
    write_size(smallest, size);
    meta *nextmeta = (meta*) ((char*) (smallest + 1) + size);
    
    write_size(nextmeta, old_size - size - sizeof(meta));
    clear_in_use(nextmeta);
  
    // case where we split the tail:
    if (smallest == last) {
      //printf("***The tail has changed!!!***\n");
      last = nextmeta;
    }
  }
  
  // Return smallest + 1 because the end of the meta block is the start of the user data
  return smallest + 1;
}

// What happens if we free the bootstrapped meta? Can we? <--If there's data in it, then yes.
void myfree(void *p, char *filename, int line) {
  
  // loop through linked list until we find a meta struct
  // such that the address of the struct + sizeof(meta)
  // EXACTLY equals the passed in address
  errorInFree = 0;
  if (p == NULL) {
    errorInFree = -1;
    //printf("WARNING: Argument is NULL. Cannot free a NULL pointer.\n"
    //"Filename: %s, Line: %d\n\n", filename, line);
    return;
  }
  
  // inputted address is outside the bounds of the vritual memory array
  if ((char*) p < myblock || (char*) p >=  myblock + HEAP_SIZE) {
    errorInFree = -1;
    printf("ERROR: Address contained in pointer is out of bounds.\n"
    "Filename: %s, Line: %d\n\n", filename, line);
    return;
  }
  // calling free when you haven't malloced once yet
  if (global_meta == NULL) {
    errorInFree = -1;
    printf("ERROR: Nothing has been malloced yet, so nothing can be freed.\n"
    "Filename: %s, Line: %d\n\n", filename, line);
    return;
  }


  // search through linked list of meta structs until we find
  // one such that (address of meta instance + sizeof(meta)) == p

  meta *ptr = global_meta, *prev = NULL;
  // Conditions to **leave** the loop:
  // -- ptr is equal to last address,
  //         OR
  // -- We find the meta data for the block whose address
  //    was passed in
  boolean found_block = (ptr + 1) == p; //end of meta block is beginning of user data
  while (! (ptr==last || found_block) ) {
  // Note that by design, this loop doesn't execute when there's only one meta block
    prev = ptr;
    ptr = (meta*) ((char*) (ptr + 1) + read_size(ptr)); // TODO make sure this works!!!!!!
    found_block = (ptr + 1) == p;
  }
  // inputted address was never returned by malloc
  if (!found_block) { // **** WARNING **** Do NOT say if ptr==last. What if
		      // ptr DOES EQUAL last BUT we
		      // DID find a block (namely, the last one)!!!!
    errorInFree = -1;
    printf("ERROR: The address contained in pointer (%p) is not the address of a block on the heap.\n"
    "Filename: %s, Line: %d\n\n", p, filename, line);
    return;
  }


  // inputted address is valid but it's already been freed
  if (read_in_use(ptr) == FALSE) {
    
    errorInFree = -1;
    printf("ERROR: This block is already free.\n"
    "Filename: %s, Line: %d\n\n", filename, line);
    //fprintf( stderr, "-1", strerror( errno ));
    return;
  }
  clear_in_use(ptr);
 
 
  // Look ahead and behind to see if we can coalesce
  if (ptr  == global_meta && ptr == last) {
    // do nothing if there's only one meta block
  }
  else if (ptr == global_meta) {
    // only check the right
    meta *right = (meta *) ((char *)(ptr + 1) + read_size(ptr));
    if (read_in_use(right) == FALSE) {
      // if ptr is a block before the last it will become the new last
      if (right == last) {
        last = ptr;
      }
      // destroy right meta block
      // and combine the sizes
      write_size(ptr, read_size(ptr) + sizeof(meta) + read_size(right));
      short zero = 0x0000;
      memcpy(right, &zero, sizeof(meta));
    }
    
  }
  else if (ptr == last) {
    // prev is left
    if (read_in_use(prev) == FALSE) {
      // since ptr is last, and we're coalescing to the left, prev will be the new last no matter what
      last = prev;
      // destroy left meta block
      // and combine the sizes
      write_size(prev, read_size(ptr) + sizeof(meta) + read_size(prev));
      short zero = 0x0000;
      memcpy(ptr, &zero, sizeof(meta));
    }
  }
  else {
    meta *right = (meta *) ((char *)(ptr + 1) + read_size(ptr));
     if (read_in_use(right) == FALSE) {
      // if ptr is a block before the last it will become the new last
      if (right == last) {
        last = ptr;
      }
      // destroy right meta block
      // and combine the sizes
      write_size(ptr, read_size(ptr) + sizeof(meta) + read_size(right));
      short zero = 0x0000;
      memcpy(right, &zero, sizeof(meta));
    }
    //prev is left
    if (read_in_use(prev) == FALSE) {
      // in the rare case where ptr is now the last after freeing what's right of ptr, make prev tail
      /*
         not in use -> not in use (ptr) -> not in use
         not in use -> not in use (ptr + right) 
         not in use (prev + ptr + right)
         remaining is the tail
      */
      if (ptr == last) {
        last = prev;
      }
      // destroy left meta block
      // and combine the sizes
      write_size(prev, read_size(ptr) + sizeof(meta) + read_size(prev));
      short zero = 0x0000;
      memcpy(ptr, &zero, sizeof(meta));
    }


  }
}

void info() {
  meta *ptr = global_meta;
  unsigned short block_size = read_size(global_meta);
  boolean in_use = read_in_use(global_meta);
  printf("INFO: Address: %p\t\tSize: %05u\tIn use: %s\n",
    ptr, block_size, (read_in_use(ptr) ? "true" : "false"));
  int totalSize = 0; // used to make sure that the virtual memory array is 5000 bytes total
  totalSize = sizeof(meta) + block_size;
 
  int num_blocks = 1;
  // do-while would only work for a circular linked list
  while (! (ptr==last) ) { 
    num_blocks++;
    ptr = (meta*) ((char*) (ptr + 1) + block_size);
    block_size = read_size(ptr);
    in_use = read_in_use(ptr);
    printf("INFO: Address: %p\t\tSize: %05u\tIn use: %s\n",
      ptr, block_size, (in_use ? "true" : "false"));
    totalSize = totalSize + sizeof(meta) + block_size;
    //printf("totalSize so far %i\n", totalSize);
  }
  printf("The total size is %i\n", totalSize);
  printf("The number of blocks is %d\n", num_blocks);
  /* Won't work!
  do {
       // ptr == last
       // advance ptr here
       // ptr ?
  } while (ptr != last)
  */
  
}


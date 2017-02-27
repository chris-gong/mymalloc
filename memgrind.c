#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include "mymalloc.h"

extern int errorInFree; //this variable is used to tell whether free was successful or not

int main() {
  
  int *pointers[1000];
  int i;
  int j;
  int workLoadATotalTime = 0;
  int workLoadBTotalTime = 0;
  int workLoadCTotalTime = 0;
  int workLoadDTotalTime = 0;
  int workLoadETotalTime = 0;
  int workLoadFTotalTime = 0;
  struct timeval start;
  struct timeval end;
  
  // Repeat the workload 100 times
  for(j = 0; j < 100; j++) {
    gettimeofday(&start, 0);
    // CASE A: Mallocing 1 byte 1000 times then freeing each pointer one by one 1000 times
    for(i = 0; i < 1000; i++) {
      pointers[i] = (int *) malloc(1);
    }
    for(i = 0; i < 1000; i++) {
      free(pointers[i]);
      pointers[i] = NULL;
    }
    gettimeofday(&end, 0);
    workLoadATotalTime = workLoadATotalTime + ((end.tv_sec-start.tv_sec)*1000000 + (end.tv_usec-start.tv_usec));
  }
  
  
  for(j = 0; j < 100; j++) {
    //info();
    gettimeofday(&start, 0);
    // CASE B: Malloc 1 byte then free it right after - do this 1000 times
    for(i = 0; i < 1000; i++) {
      pointers[i] = (int *) malloc(1);
      free(pointers[i]);
      pointers[i] = NULL;
    }
    gettimeofday(&end, 0);
    workLoadBTotalTime = workLoadBTotalTime + ((end.tv_sec-start.tv_sec)*1000000 + (end.tv_usec-start.tv_usec));
  }


  gettimeofday(&start, 0);
  for(j = 0; j < 100; j++) {
    gettimeofday(&start, 0);
    // CASE C: Randomly choose between a 1 byte malloc and freeing a pointer
    int timesMalloced = 0;
    //int availableNodesLeft = 0; //not including globalmeta
    int currentMallocIndex = 0; //rightmost position in pointers array that can be malloced
    int currentFreeIndex = 0; //leftmost position in pointers array that can be freed
    srand(time(NULL)); // ONLY CALL THIS ONCE OR ELSE RANDOM VALUES WILL NOT BE RANDOM
    int random = rand() % 2; //returns a random int between 0 and 1 
    //in case C and D, you only increment currentMallocIndex when you malloc and you only increment currentFreeIndex when you free
    while (timesMalloced < 1000) {
      if (random == 0) { //malloc
        pointers[currentMallocIndex] = (int *)malloc(1);
        if (!(pointers[currentMallocIndex] == NULL)) { // if no more room to malloc
          currentMallocIndex++;
          timesMalloced++;
        }
        else {
          free(pointers[currentFreeIndex]);
          pointers[currentFreeIndex] = NULL;
          currentFreeIndex++;
        }
      }
      else if (random == 1) { //free
        free(pointers[currentFreeIndex]);
        if (errorInFree < 0) { //can't free
          pointers[currentMallocIndex] = (int *)malloc(1);
          currentMallocIndex++;
          timesMalloced++; // <- if malloc is a result of a failed free, it doesn't count towards the 1000 mallocs
        }
        else {
          pointers[currentFreeIndex] = NULL;
          currentFreeIndex++;
        }
      }
      random = rand() % 2;
    }
    for(i = currentFreeIndex; i < 1000; i++) {
      free(pointers[i]);
      pointers[i] = NULL;
    }
    gettimeofday(&end, 0);
    workLoadCTotalTime = workLoadCTotalTime + ((end.tv_sec-start.tv_sec)*1000000 + (end.tv_usec-start.tv_usec));
  }
  
  for(j = 0; j < 100; j++) {
    gettimeofday(&start, 0);
    // CASE D: Randomly choose between a 1-64 byte malloc and freeing a pointer
    int timesMalloced = 0;
    //int availableNodesLeft = 0; //not including globalmeta
    int currentMallocIndex = 0; //rightmost position in pointers array that can be malloced
    int currentFreeIndex = 0; //leftmost position in pointers array that can be freed
    int random = rand() % 2; //returns a random int between 0 and 1 
    //in case D, you only increment currentMallocIndex when you malloc and you only increment currentFreeIndex when you free
    while (timesMalloced < 1000) {
      int randomSize = (rand() % 63) + 1; //returns a random int between 1 and 64
      if (random == 0) { //malloc
        pointers[currentMallocIndex] = (int *)malloc(randomSize);
        if (!(pointers[currentMallocIndex] == NULL)) { // if no more room to malloc
          currentMallocIndex++;
          timesMalloced++;
        }
        else {
          free(pointers[currentFreeIndex]);
          pointers[currentFreeIndex] = NULL;
          currentFreeIndex++;
        }
      }
      else if (random == 1) { //free
        free(pointers[currentFreeIndex]);
        if (errorInFree < 0) { //can't free
          pointers[currentMallocIndex] = (int *)malloc(randomSize);
          currentMallocIndex++;
          timesMalloced++; // <- if malloc is a result of a failed free, it doesn't count towards the 1000 mallocs
        }
        else {
          pointers[currentFreeIndex] = NULL;
          currentFreeIndex++;
        }
      }
      random = rand() % 2;
    }
    
    for(i = currentFreeIndex; i < 1000; i++) {
      free(pointers[i]);
      pointers[i] = NULL;
    }
    gettimeofday(&end, 0);
    workLoadDTotalTime = workLoadDTotalTime + ((end.tv_sec-start.tv_sec)*1000000 + (end.tv_usec-start.tv_usec));
  } // end 100 workload loop D

  
  for (i = 0; i < 100; i++) {
    gettimeofday(&start, 0);
    
    // CASE E
    int numberOfBlocks = 1;
    int currentMallocIndex = 0;
    int currentFreeIndex = 0;

    // malloc until we reach capacity
    while ((pointers[currentMallocIndex] = malloc(50)) != NULL ) {
      numberOfBlocks++;
      currentMallocIndex++;
    }
    // Now free every other block
    for (currentFreeIndex = 0; currentFreeIndex < currentMallocIndex; currentFreeIndex += 2) {
      free(pointers[currentFreeIndex]);
      pointers[currentFreeIndex] = NULL;
    }
    // Now re-malloc
    for (currentMallocIndex = 0; currentMallocIndex < numberOfBlocks-1; currentMallocIndex += 2) {
      pointers[currentMallocIndex] = malloc(35);
    }
    // Now free everything
    for (currentFreeIndex = 0; currentFreeIndex < numberOfBlocks-1; currentFreeIndex += 1) {
      free(pointers[currentFreeIndex]);
      pointers[currentFreeIndex] = NULL;
    }
    gettimeofday(&end, 0);
    workLoadETotalTime = workLoadETotalTime + ((end.tv_sec-start.tv_sec)*1000000 + (end.tv_usec-start.tv_usec));
  }

  

  for (i = 0; i < 100; i++) {
    gettimeofday(&start, 0);
    // CASE F
    int numberOfBlocks = 1;
    int currentMallocIndex = 0;
    int currentFreeIndex = 0;
    // malloc until we reach capacity
    while ((pointers[currentMallocIndex] = malloc(50)) != NULL ) {
      numberOfBlocks++;
      currentMallocIndex++;
    }
    // Now free every other block
    for (currentFreeIndex = 0; currentFreeIndex < currentMallocIndex; currentFreeIndex += 2) {
      free(pointers[currentFreeIndex]);
      pointers[currentFreeIndex] = NULL;
    }
    // Now re-malloc
    for (currentMallocIndex = 0; currentMallocIndex < numberOfBlocks-1; currentMallocIndex += 2) {
      pointers[currentMallocIndex] = malloc(25);
    }
    // Now free everything
    for (currentFreeIndex = 0; currentFreeIndex < numberOfBlocks-1; currentFreeIndex += 1) {
      free(pointers[currentFreeIndex]);
      pointers[currentFreeIndex] = NULL;
    }
    //info();
    gettimeofday(&end, 0);
    workLoadFTotalTime = workLoadFTotalTime + ((end.tv_sec-start.tv_sec)*1000000 + (end.tv_usec-start.tv_usec));
  }


  printf("Mean time to execute workload A %d milliseconds\n", workLoadATotalTime/100);
  printf("Mean time to execute workload B %d milliseconds\n", workLoadBTotalTime/100);
  printf("Mean time to execute workload C %d milliseconds\n", workLoadCTotalTime/100);
  printf("Mean time to execute workload D %d milliseconds\n", workLoadDTotalTime/100);
  printf("Mean time to execute workload E %d milliseconds\n", workLoadETotalTime/100);
  printf("Mean time to execute workload F %d milliseconds\n", workLoadFTotalTime/100);


  return 0;
}

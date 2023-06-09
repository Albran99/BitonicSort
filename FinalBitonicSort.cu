#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <chrono>
#include <cmath>
using namespace std;

/* 
  arrayInit fill the array with random values 
  it takes as input the array to fill and its lenght  
*/
void arrayInit(int *arr, unsigned long length) {
    unsigned long i;
    for (i = 0; i < length; ++i) {
        arr[i] = (rand() % 100);
    }
}
/* 
  checkArray check if the array after the bitonic sort is properly sorted 
  it takes as input the array to fill and its lenght
  return true if the array is sorted, false otherwise
  */
bool checkArray(int *arr, unsigned long long length) {
    for (unsigned long long i = 1; i < length; ++i) {
        if(arr[i-1]>arr[i]){
            printf("Array not sorted");
            return false;
        }
    }
    return true;
}

/* 
  swap_values use a temp variable in order to swap two values
  it is performed in GPU and it is used by bitonic_sort_kernel
  it takes as input the two values to swap 
  */
__device__ void swap_values(int *a, int *b){
  int temp = *a;
  *a = *b;
  *b = temp;
}

/* 
 bitonic_sort_kernel performs a single pass of sorting in the GPU 
 it takes as input the array on which to operate, the distance between the element to
 swap in case the check is passed and the size of the sequence to sort
*/
__global__ void bitonic_sort_kernel(int *arr, unsigned long long distance, unsigned long long subSequence_size){
  //get the thread id and the check value using the xor operands
  unsigned long long i, xorCheck; 
  i = threadIdx.x + blockDim.x * blockIdx.x;
  
  //get the element in the array to sort
  xorCheck = i^distance;

  //Sort only the element that are distant enough
  if ((xorCheck)>i) {
    
    //if the operator produce 0 we are in the ascending part
    //of the bitonic sequence
    if ((i & subSequence_size)==0) {
      if (arr[i]>arr[xorCheck]) {
        swap_values(&arr[i],&arr[xorCheck]);
      }
    }

    //otherwise we are in the decending part
    else {
      if (arr[i]<arr[xorCheck]) {
        swap_values(&arr[i],&arr[xorCheck]);
      }
    }
  }
}

/* 
  bitonic_sort performs the operations to sort the array 
  it takes as input the array to sort and its lenght
  */
void bitonic_sort(int *arr, unsigned long long array_size){
    int *cuda_arr; // device array
    size_t size = array_size * sizeof(int); // size * 4 byte
    
    //allocate memory on device
    cudaMalloc((void**) &cuda_arr, size); 
    //copy the original array to the device one
    cudaMemcpy(cuda_arr, arr, size, cudaMemcpyHostToDevice); 

    //set the number of threads per blocks and calculate the number of blocks
    int thread_dim=128;
    int block_dim=(array_size + thread_dim - 1) / thread_dim;

    
    unsigned long long distance, subSequence_size; 

    //initialize the time recording without cudamemcpy
    cudaEvent_t start, stop;
    float elapsed;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start, 0);

    //iterate through the array sorting sequence of ascending dimention over distances
    //of decending dimention
    for (subSequence_size = 2; subSequence_size <= array_size; subSequence_size <<= 1) 
      for (distance = subSequence_size >> 1; distance > 0; distance = distance >> 1)        
        bitonic_sort_kernel<<<block_dim, thread_dim>>>(cuda_arr, distance, subSequence_size);
        
    //calculate elapsed time withouth the cudaMemcpy
    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&elapsed, start, stop);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    printf("The elapsedtime in GPU without memcopy was: %.3f ms\n", elapsed);
    
    //copy back to the host array
    cudaMemcpy(arr, cuda_arr, size, cudaMemcpyDeviceToHost); 
    //free device memory
    cudaFree(cuda_arr); 
   
}

int main(int argc, char** argv){
  //counter for the sum of the elapsed time for the numbers of iteration  
  double tot=0;
  double min = 99999999999; // initialization value 
  int power = atoi(argv[1]); 
  //number of elements must be a power of 2 so the dimension is 2^power
  unsigned long long array_size = 1<<power; 
  printf("array_size %llu --> 2^%i\n", array_size, power);
  //number of iterations
  int iter=30; 

  for(int h=0; h<iter; h++){
    //allocate host array
    int *arr = (int*) malloc( array_size * sizeof(int)); 
    //initialize the host array with random values
    arrayInit(arr, array_size); 

    cudaEvent_t startExt, outExt;
    //contains the elapsed time on the bitonic sort with bitonic sort
    float elapsed; 
    cudaEventCreate(&startExt);
    cudaEventCreate(&outExt);
    cudaEventRecord(startExt, 0);
    //=========================================//
    bitonic_sort(arr, array_size);
    //=========================================//
    cudaEventRecord(outExt, 0);
    cudaEventSynchronize(outExt);
    cudaEventElapsedTime(&elapsed, startExt, outExt);
    cudaEventDestroy(startExt);
    cudaEventDestroy(outExt);
    printf("The elapsedtime in GPU with memcopy was: %.3f ms\n", elapsed);

    //check if sort has been done correctly
    if(checkArray(arr,array_size)){ 
        printf("Array sorted correctly\n");
    }
    //free host memory
    free(arr); 
    //add the current iteration to the total
    tot +=elapsed; 
    
    //calculate the minimum over the various iteration
    if(elapsed<min){
      min = elapsed;
    }
  }
  printf("Average elapsed time: %.3f\n",tot/iter);
  printf("Minimum time: %.3f\n",min);
}
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#define ARRAY_SIZE 4096 //65536 //

double calc_time(struct timespec start, struct timespec end) { 
  double start_sec = (double)start.tv_sec*1000000000.0 + (double)start.tv_nsec; 
  double end_sec = (double)end.tv_sec*1000000000.0 + (double)end.tv_nsec; 
  if (end_sec < start_sec) {  
    return 0;   
  } else {   
    return end_sec - start_sec;
  } 
}

void init_array(double *arr){
  for (int i=0; i<ARRAY_SIZE; ++i){
    arr[i] = i;
  }
}

double test_bandwidth_mod1(int ITERATIONS){
    struct timespec start_time, end_time;
    double array[ARRAY_SIZE];
    init_array(array);
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (int i=0; i<ITERATIONS; ++i){
        for (int j=0; j<ARRAY_SIZE; ++j){
          array[j] = 1;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double res_ns = calc_time(start_time, end_time);
    return res_ns;
}

double test_bandwidth_mod2(int ITERATIONS){
    struct timespec start_time, end_time;
    double array[ARRAY_SIZE];
    init_array(array);
    int cur;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (int i=0; i<ITERATIONS; ++i){
        for (int j=0; j<ARRAY_SIZE; ++j){
          array[j] = 1;
          cur = array[j];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double res_ns = calc_time(start_time, end_time);
    return res_ns;
}

double test_bandwidth_mod3(int ITERATIONS){
    struct timespec start_time, end_time;
    double array[ARRAY_SIZE];
    init_array(array);
    int cur;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (int i=0; i<ITERATIONS; ++i){
        for (int j=0; j<ARRAY_SIZE; ++j){
          array[j] = 1;
          cur = array[j];
          cur = array[j];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double res_ns = calc_time(start_time, end_time);
    return res_ns;
}

int main(int argc, char* argv[]) {
    if (argc < 2){
        printf("Usage: ./bandwidth_test <iterations>\n");
        return -1;
    }
    int N = atoi(argv[1]);
    double runtime[3];
    double bandwidth[3];
    runtime[0] = test_bandwidth_mod1(N);
    bandwidth[0] = (double)ARRAY_SIZE*N*8/runtime[0];
    printf("****** Array Size 32KB ******\n");
    printf("Write Only:\n");
    printf("Total runtime for %d iterations: %f\n", N, runtime[0]/1000000000.0);
    printf("The calculated bandwidth is %f GB/s.\n", bandwidth[0]);
    runtime[1] = test_bandwidth_mod2(N);
    bandwidth[1] = (double)ARRAY_SIZE*N*8*2/runtime[1];
    printf("1 Write 1 Read:\n");
    printf("Total runtime for %d iterations: %f\n", N, runtime[1]/1000000000.0);
    printf("The calculated bandwidth is %f GB/s.\n", bandwidth[1]);
    runtime[2] = test_bandwidth_mod3(N);
    bandwidth[2] = (double)ARRAY_SIZE*N*8*3/runtime[2];
    printf("1 Write 2 Read:\n");
    printf("Total runtime for %d iterations: %f\n", N, runtime[2]/1000000000.0);
    printf("The calculated bandwidth is %f GB/s.\n", bandwidth[2]);
    return 0;
}
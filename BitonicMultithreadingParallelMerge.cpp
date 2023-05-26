#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
using namespace std;

void print_array(int arr[], int length) {
    for (int i = 0; i < length; ++i) {
        cout << arr[i]<<' ';
    }
    cout << endl;
}
/* check_array check if the array after the bitonic sort is properly sorted */
void check_array(int arr[], int length) {
    for (int i = 1; i < length; ++i) {
        if(arr[i-1]>arr[i]){
            exit(-1); // aborting the program if the array is not sorted
        }
    }
}

void bitonic_merge(int arr[], int start, int length, bool direction) { 
    //if (length == 1) return;
    int mid = length / 2;
    for (int i = start; i < start + mid; ++i) {
        if ((arr[i] > arr[i + mid]) == direction) { // perform the swap in order to construct the sequence
            int temp = arr[i];
            arr[i]= arr[i+mid];
            arr[i+mid] = temp;
        }
    }
    if(mid == 1) // optimization: avoid a recursive call
        return;
    bitonic_merge(arr, start, mid, direction);
    bitonic_merge(arr, start + mid, mid, direction);
}

void bitonic_sort(int arr[], int start, int length, bool direction) { 
    if (length == 1) return;
    int mid = length / 2;
    bitonic_sort(arr, start, mid, true); // ascending sequence 
    bitonic_sort(arr, start + mid, mid, false); // discending sequence 
    bitonic_merge(arr, start, length, direction); // create the bitonic sequence
}

// optimized version with multithreading in the merge phase
void parallel_bitonic_merge(int arr[], int start, int length, bool direction, int num_threads) {
    
    if (length == 1) {
        return;
    }
    int mid = length / 2;
    for (int i = start; i < start + mid; ++i) {
        if ((arr[i] > arr[i + mid]) == direction) {
            int temp = arr[i];
            arr[i] = arr[i + mid];
            arr[i + mid] = temp;
        }
    }

    if (num_threads > 1) { 
        int new_threads = num_threads / 2;
        thread t1(parallel_bitonic_merge, arr, start, mid, direction, new_threads); // optimized version with multithreading in the merge phase, same three structure of recursive call
        thread t2(parallel_bitonic_merge, arr, start + mid, mid, direction, num_threads - new_threads);
        t1.join();
        t2.join();
    } else {
        bitonic_merge(arr, start, length, direction);
    }
}
// entry point function for the bitonic_sort
void parallel_bitonic_sort(int arr[], int start, int length, bool direction, int num_threads) {
    if (num_threads == 1) { // call the bitonic_sort and create the bitonic sequence 
        bitonic_sort(arr, start, length, direction);
    } else {
        int mid = length / 2; // halve the array size
        thread t1(parallel_bitonic_sort, arr, start, mid, true, num_threads / 2); // the first thread call recursively the function with first half of the array
        thread t2(parallel_bitonic_sort, arr, start + mid, mid, false, num_threads - num_threads / 2); // the second thread call recursively the function with second half of array
        t1.join(); // wait for thread to finish, three structure of recursive call
        t2.join();
        parallel_bitonic_merge(arr, start, length, direction, num_threads); // perform the bitonic sequence merge and finalize the sort
    }
}

int main(int argc, char** argv) {
    int pot; // the exponent of 2
    int MAX_TH; //number of max thread for the run
    sscanf(argv[1], "%i", &pot);
    int N = pow(2,pot); // number of elements of the array 
    double previous_avg_time = 0;  // time elapsed for the previous run 
    int policy; // 0 for random (avg case), 1 for all elements having the same value, 2 for sorted array, 3 for worst case
    sscanf(argv[2], "%i", &MAX_TH); 
    sscanf(argv[3], "%i", &policy);
    double time_mono=0; // time elapsed for the mono_thread case
    for(int i = 1; i<=MAX_TH; ++i){ // perform the tests until MAX_TH
        double previous_time = 0;
        double speedup_mono = 0;
        double sum_time = 0;
        double sum_speedup=0;
        double speedup = 0;
        double avg_speedup = 0;
        double time_min=0; 
        int iter=30; // numbers of iteration for the current thread number
        for(int j = 0; j<iter; ++j){
            int * a = new int[N];
            if(policy==0){
                srand(3);
                for(int i=0; i<N; i++){
                    a[i]=(rand()%100);
                    //cout<<a[i];
                }
            }
            if(policy==1){
                for(int i=0; i<N; i++){
                    a[i]=0;
                    //cout<<a[i];
                }
            }
            if(policy==2){
                for(int i=0; i<N; i++){
                    a[i]=i;
                    //cout<<a[i];
                }
            }
            if(policy==3){
                for(int i=0; i<N; i++){
                    a[i]=(N-i);
                    //cout<<a[i];
                }
            }
            //=========================//
            auto start = chrono::high_resolution_clock::now();
            parallel_bitonic_sort(a, 0, N, true, i);
            auto end = chrono::high_resolution_clock::now();
            //=========================//
            //print_array(a, N);
            check_array(a, N); // check if the array is properly sorted 
            auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
            if(previous_time == 0){ // first iteration, initialize statistics 
                previous_time = duration.count();
                time_min=duration.count();
            }
            else{ // not the first iteration, update the statistics
                speedup = previous_time/duration.count();
                previous_time = duration.count();
                time_min=min(time_min,(double)duration.count()); 
            }
            sum_time +=duration.count();
            sum_speedup +=speedup;
            delete[] a;
        }
        double avg_time = sum_time/iter; // calculate avg time
        
        if(i != 1){ // if thred_number is >1 
            avg_speedup = previous_avg_time/avg_time;
            speedup_mono = time_mono/avg_time;
        }
        else{ // mono thread case 
            time_mono = avg_time;
            avg_speedup = 1;
            speedup_mono = 1;
        }
        previous_avg_time = avg_time;
        
        cout<<pot<<"\t"<<i<<"\t"<<avg_time<<"\t"<<time_min<<"\t"<<avg_speedup<<"\t"<<speedup_mono<<endl;
    }
    return 0;
}

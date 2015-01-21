#include <immintrin.h>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <thread>
#include <pthread.h>
#include <unistd.h>

//using namespace std;

#define MAX_THREAD 8
#define NUM_REPEAT 4
#define ARRAY_SIZE 20


int arr[ARRAY_SIZE];

void target(int tid) {

//    long tid = (long) threadid;

 for (int repeat = 0; repeat < NUM_REPEAT; repeat++) {
	int status;
	status = _xbegin();
	if (status == _XBEGIN_STARTED) {
		for (int i = 0; i <ARRAY_SIZE; i++) {
			arr[i]=i;
			}

		_xend();
       		}

        else
             __transaction_atomic {
                for (int i = 0; i <ARRAY_SIZE; i++) {
			arr[i] = i;
	                }
		}
 }
}

int main() {

    std::vector<std::thread> threads;

    for (int i = 0; i < ARRAY_SIZE; i++)
             arr[i] = 0;


    for(int i = 0; i <MAX_THREAD; i++){
        threads.push_back(std::thread(target, i));
    }

        for(auto& thread : threads){
                thread.join();
    }

 return 0;

}

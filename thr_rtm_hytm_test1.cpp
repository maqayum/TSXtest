// written by Roman Dementiev

//#include "stdafx.h"
//#include <windows.h>
//#include <process.h>
#include <immintrin.h>
#include <iostream>
#include <thread>

#define NUM_THREAD 16
#define NUM_REPEAT 16
#define ARRAY_SIZE 20


int arr[ARRAY_SIZE];

void thread_worker() {

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
             __transaction_relaxed {
                for (int i = 0; i <ARRAY_SIZE; i++) {
                        arr[i] = i;
                        }
                }
 }
}


int main(int argc, char* argv[])

{

//    std::vector<std::thread> threads;

//    for(int i = 0; i < 2; ++i){
//        threads.push_back(std::thread(thread_worker, i+1));
//    }

//        for (int i = 0; i < 2; ++i)
//                std::thread (thread_worker, i+1);

//        for(auto& thread : threads){
//                thread.join();
//    }
        std::thread thr[NUM_THREAD];



        for (int i = 0; i < NUM_THREAD; ++i)
                thr[i] = std::thread(thread_worker);

        for (auto &t : thr)
                t.join();



	return 0;
}



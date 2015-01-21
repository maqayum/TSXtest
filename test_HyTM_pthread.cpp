#include <iostream>
#include <immintrin.h>
#include <pthread.h>

using namespace std;

#define MAX_THREAD 8
#define NUM_REPEAT 4
#define ARRAY_SIZE 20


int arr[ARRAY_SIZE];

void *target(void *threadid) {

    long tid = (long) threadid;

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
        pthread_t threads[MAX_THREAD];

        for (int i = 0; i < ARRAY_SIZE; i++)
                arr[i] = 0;

        for (long t = 0; t < MAX_THREAD; t++) {
                if (pthread_create(&threads[t], NULL, &target, (void *)t)) {
                        cout << "create thread " << t << " failed\n";
                        return -1;
                }
        }

        for (int i = 0; i <  MAX_THREAD; i++) {
                if (pthread_join(threads[i], NULL)) {
                        cout << "join thread " << i << " failed\n";
                        return -1;
                }
                cout << "Thread " << i << " joined.\n";
        }

        return 0;

}

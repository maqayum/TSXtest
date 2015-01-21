#include <iostream>
//#include <mutex>
#include <sys/syscall.h>
#include <atomic>
#include <omp.h>
#include <immintrin.h>

#define N 1000
using namespace std;

class SimpleSpinLock
{
	volatile unsigned int state;

	enum { Free = 0, Busy = 1};

public:
	SimpleSpinLock() : state(Free) {}

	void lock()
	{
//		while(InterlockedCompareExchange(&state,Busy,Free) != Free)
		while(__sync_lock_test_and_set(&state,Busy,Free) != Free)
		{
			do { _mm_pause(); } while(state == Busy);
		}
	}

	void unlock() { __sync_lock_release(&state, Free);  }

	bool isLocked() const { return state == Busy; }

};

int main() {

//mutex fb_mutex; // fallback mutex
SimpleSpinLock fallBackLock;

int i, group[N], data[N], sums[N];

for(i = 0; i < N; i++){
    group[i]=i;
    data[i]=i;
}


#pragma omp parallel for num_threads(100)
for(i = 0; i < N; i++){
   int mygroup = group[i];
   if(_xbegin()) {  // First attempt
       if( !fallBackLock.isLocked() ) {
           sums[mygroup] += data[i];
       }
       else{
           _xabort(1);
       }
       _xend();
   } else { // Fallback path: get mutex
       //fb_mutex.acquire();
       fallBackLock.lock();
       sums[mygroup] += data[i];
       //fb_mutex.release();
       fallBackLock.unlock();
   }
}
cout<<"I am here"<<endl;
cout<<"sums[20]="<<sums[20]<<endl;
}

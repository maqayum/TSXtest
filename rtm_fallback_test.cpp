// written by Roman Dementiev
//Edited by M A Qayum for unix system
//#include "stdafx.h"
//#include <windows.h>
//#include <process.h>
#include <immintrin.h>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <tr1/random>
#include <thread>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <atomic>

#define atomic_inc(P) __sync_add_and_fetch((P), 1)

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

} globalFallBackLock;

#if 0
class TransactionScope // implementation using a legacy lock
{
	SimpleSpinLock & lock;
        TransactionScope(); // forbidden
public:
	TransactionScope(SimpleSpinLock & lock_): lock(lock_) { lock.lock(); }

	~TransactionScope() { lock.unlock(); }
};
#endif


int64_t naborted = 0;


class TransactionScope
{
	SimpleSpinLock & fallBackLock;

	TransactionScope(); // forbidden
public:
	TransactionScope(SimpleSpinLock & fallBackLock_, int max_retries = 3) : fallBackLock(fallBackLock_)
	{
#if 1
		int nretries = 0;

		while(1)
		{
			++nretries;
			unsigned status = _xbegin();

			if(status == _XBEGIN_STARTED)
			{
				if(!fallBackLock.isLocked()) return; //successfully started transaction
				// started transaction but someone executes the transaction section
				// non-speculatively (acquired the fall-back lock)
				_xabort(0xff); // abort with code 0xff
			}
			// abort handler

			atomic_inc(&naborted); // do abort statistics

			std::cout << "DEBUG: Transaction aborted "<< nretries <<" time(s) with the status "<< status << std::endl;

			// handle _xabort(0xff) from above
			if((status & _XABORT_EXPLICIT) && _XABORT_CODE(status)==0xff && !(status & _XABORT_NESTED))
			{
				while(fallBackLock.isLocked())
					_mm_pause(); // wait until lock is free

			} else if(!(status & _XABORT_RETRY)) break; //take the fall-back lock if the retry abort flag is not set

			if(nretries >= max_retries) break; // too many retries, take the fall-back lock
		}

		fallBackLock.lock();

#else // the naive version goes here
		int nretries = 0;
		while(1)
		{
			++nretries;
			unsigned status = _xbegin();

			if(status == _XBEGIN_STARTED) return;

			std::cout << "DEBUG: Transaction aborted "<< nretries <<" time(s) with the status "<< status << std::endl;
		}
#endif
	}

	~TransactionScope()
	{
		if(fallBackLock.isLocked())
			fallBackLock.unlock();
		else
			_xend();
	}
};

std::vector<int> Accounts;

int global = 0;

//unsigned __stdcall thread_worker(void * arg)
void thread_worker(int arg)
{
	int thread_nr = (int) arg;
	std::cout << "Thread "<< thread_nr<< " started." << std::endl;
	std::tr1::minstd_rand myRand(thread_nr);
	long int loops = 10000;

	while(--loops)
	{
		{
			TransactionScope guard(globalFallBackLock); //protect everything in this scope with RTM
			// put 100 units into a random account atomically
			Accounts[myRand() % Accounts.size()] += 100;
		}

		{
			TransactionScope guard(globalFallBackLock); // protect everything in this scope with RTM
			// transfer 100 units between random accounts (if there is enough money) atomically
			int a = myRand() % Accounts.size(), b = myRand() % Accounts.size();
			if(Accounts[a] >= 100)
			{
				Accounts[a] -= 100;
				Accounts[b] += 100;
			}
		}
	}

	std::cout << "Thread "<< thread_nr<< " finished." << std::endl;

}

//int _tmain(int argc, _TCHAR* argv[])
int main(int argc, char* argv[])

{

	{
		std::cout << "open new account" << std::endl; //account 0
		TransactionScope guard(globalFallBackLock); // protect everything in this scope with RTM
		Accounts.push_back(0);
	}
	{
		std::cout << "open new account" << std::endl; //account 1
		TransactionScope guard(globalFallBackLock); // protect everything in this scope with RTM
		Accounts.push_back(0);
	}
	{
		std::cout << "put 100 units into account 0" <<std::endl;
		TransactionScope guard(globalFallBackLock); // protect everything in this scope with RTM
		Accounts[0] += 100; // atomic update due to RTM
	}
	{
		std::cout << "transfer 10 units from account 0 to account 1 atomically!" << std::endl;
		TransactionScope guard(globalFallBackLock); // protect everything in this scope with RTM
		Accounts[0] -= 10;
		Accounts[1] += 10;
	}
	{
		std::cout << "atomically draw 10 units from account 0 if there is enough money"<< std::endl;
		TransactionScope guard(globalFallBackLock); // protect everything in this scope with RTM
		if(Accounts[0] >= 10) Accounts[0] -= 10;
	}
	{
		std::cout << "add 1000 empty accounts atomically"<<std::endl;
		TransactionScope guard(globalFallBackLock); // protect everything in this scope with RTM
		Accounts.resize(Accounts.size() + 1000, 0);

	}

    std::vector<std::thread> threads;

    for(int i = 0; i < 2; ++i){
        threads.push_back(std::thread(thread_worker, i+1));
    }

//        for (int i = 0; i < 2; ++i)
//                std::thread (thread_worker, i+1);

        for(auto& thread : threads){
                thread.join();
    }


//	HANDLE thread1 = (HANDLE) _beginthreadex(NULL, 0, &thread_worker, (void *)1, 0, NULL);
//	HANDLE thread2 = (HANDLE) _beginthreadex(NULL, 0, &thread_worker, (void *)2, 0, NULL);
//	WaitForSingleObject( thread1, INFINITE );
//	WaitForSingleObject( thread2, INFINITE );

	std::cout << "Total number of transaction aborts: "<<naborted<<std::endl;

	return 0;
}



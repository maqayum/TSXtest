//Coded by MA Qayum

#include <immintrin.h>
#include <iostream>
#include <pthread.h>
#include <atomic>
#include <vector>
#include <tr1/random>

#define MAX_THREAD 2

#define atomic_inc(P) __sync_add_and_fetch((P), 1)

class SimpleSpinLock
{
	volatile unsigned int state;

	enum { Free = 0, Busy = 1};

public:
	SimpleSpinLock() : state(Free) {}

	void lock()
	{
		while(__sync_lock_test_and_set(&state,Busy,Free) != Free)
		{
			do { _mm_pause(); } while(state == Busy);
		}
	}

	void unlock() { __sync_lock_release(&state, Free);  }

	bool isLocked() const { return state == Busy; }

} globalFallBackLock;


int64_t naborted = 0;


class TransactionScope
{
	SimpleSpinLock & fallBackLock;

	TransactionScope(); // forbidden
public:
	TransactionScope(SimpleSpinLock & fallBackLock_, int max_retries = 3) : fallBackLock(fallBackLock_)
	{
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

        //        __transaction_atomic {
	//			if(!fallBackLock.isLocked()) return; //successfully started transaction
        //        }

		fallBackLock.lock();

	}

	~TransactionScope()
	{
		if(fallBackLock.isLocked())
			fallBackLock.unlock();
	//	else if(status == _XBEGIN_STARTED)
	//		_xend();
                else
			_xend();
        //		;
	}
};

std::vector<int> Accounts;

int global = 0;


void *thread_worker(void *arg)
{
        long thread_nr = (long) arg;

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

        pthread_t threads[MAX_THREAD];


        for (long t = 0; t < MAX_THREAD; t++) {
                if (pthread_create(&threads[t], NULL, thread_worker, (void *)t)) {
                        std::cout << "create thread " << t << " failed\n";
                        return -1;
                }
        }

        for (int i = 0; i < MAX_THREAD; i++) {
                if (pthread_join(threads[i], NULL)) {
                        std::cout << "join thread " << i << " failed\n";
                        return -1;
                }
                std::cout << "Thread " << i << " joined.\n";
        }


	std::cout << "Total number of transaction aborts: "<<naborted<<std::endl;

	return 0;
}



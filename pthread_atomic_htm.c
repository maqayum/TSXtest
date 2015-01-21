//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>
//#include <sys/unistd.h>
//#include <unistd.h>
//#include <sys/types.h>
#include <pthread.h>
//#include <omp.h>

float *x,*y,*work1,*work2;
int *index;

#define N 20
#define MAX_THREAD 8

void *target(void *threadid) {
           int i;
	   for( i=0;i< N;i++) {
		   if (_xbegin() == _XBEGIN_STARTED) {
                      x[index[i]] += work1[i];
                      _xend();
                       }
                   else
                      __transaction_atomic {
                      x[index[i]] += work1[i];
                        }
               y[i] += work2[i];
           }
	}

int main() {
    pthread_t threads[MAX_THREAD];
    x=(float*)malloc(N*sizeof(float));
    y=(float*)malloc(N*sizeof(float));
    work1=(float*)malloc(N*sizeof(float));
    work2=(float*)malloc(N*sizeof(float));
    index=(int*)malloc(N*sizeof(float));

    for( int i=0;i <N;i++) {
        index[i]=(N-i)-1;
        x[i]=0.0;
        y[i]=0.0;
        work1[i]=i;
        work2[i]=i*i;
    }

        for (long t = 0; t < MAX_THREAD; t++) {
                if (pthread_create(&threads[t], NULL, &target, (void *)t)) {
                        printf("Creation of thread %l failed\n", t);
                        return -1;
                }
        }

        for (long u = 0; u <  MAX_THREAD; u++) {
                if (pthread_join(threads[u], NULL)) {
                        printf("Joining of thread %l failed\n", u);
                        return -1;
                }
                printf("Thread %l joined.\n", u);
        }

for( int i=0;i <N;i++)
                printf("%d %g %g\n",i,x[i],y[i]);

        return 0;

}

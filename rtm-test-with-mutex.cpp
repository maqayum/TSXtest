#include <iostream>
#include <mutex>
#include <omp.h>
#include <immintrin.h>

#define N 1000
using namespace std;

int main() {
mutex fb_mutex; // fallback mutex
int i, group[N], data[N], sums[N];


#pragma omp parallel for num_threads(8)

for(i = 0; i < N; i++){
   int mygroup = group[i];
   if(_xbegin()) {  // First attempt
       if( !fb_mutex.is_acquired() ) {
           sums[mygroup] += data[i];
       }
       else{
           _xabort(1);
       }
       _xend();
   } else { // Fallback path: get mutex
       fb_mutex.acquire();
       sums[mygroup] += data[i];
       fb_mutex.release();
   }
}
}

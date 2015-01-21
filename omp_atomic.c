#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main() {
    float *x,*y,*work1,*work2;
    int *index;
    int n,i;
    n=100;
    x=(float*)malloc(n*sizeof(float));
    y=(float*)malloc(n*sizeof(float));
    work1=(float*)malloc(n*sizeof(float));
    work2=(float*)malloc(n*sizeof(float));
    index=(int*)malloc(10*sizeof(float));
//    omp_set_num_threads(8);
    for( i=0;i < n;i++) {
        index[i]=(n-i)-1;
        x[i]=0.0;
        y[i]=0.0;
        work1[i]=i;
        work2[i]=i*i;
    }
#pragma omp parallel for  shared(x,y,index,n)
    for( i=0;i< n;i++) {

#pragma omp atomic
        x[index[i]] += work1[i];
        y[i] += work2[i];
    }
for( i=0;i < n;i++)
                printf("%d %g %g\n",i,x[i],y[i]);
return 0;
}

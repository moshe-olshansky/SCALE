#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

struct th2 {
	unsigned int *i;
	unsigned int *j;
	float *x;
	float *v;
	float *res;
	long m;
};

void *Mul(void *threadid) {
	struct th2 *a = (struct th2 *) threadid;
	unsigned int *i = a->i;
	unsigned int *j = a->j;
	float *x = a->x;
	float *v = a->v;
	float *res = a->res;
	long m = a->m;
	long p;
	for (p=0;p<m;p++) {
               res[i[p]] += x[p]*v[j[p]];
               res[j[p]] += x[p]*v[i[p]];
	}
}

void utmvMul(unsigned int *i,unsigned int *j,float *x,long m,float *v,unsigned int k,float *res, int nth, float **rs) {
	long *mm = (long *) malloc(nth*sizeof(long));
	int t;
	long n = (long) (m/nth);
	for (t=0;t<nth;t++) mm[t] = n;
	mm[nth-1] = m - n*(nth-1);
	long p;
	struct th2 a[nth];
//	float *rs[nth];
	for (t=0;t<nth;t++) {
//		rs[t] = (float *) malloc(k*sizeof(float));
		for (p=0;p<k;p++) rs[t][p] = 0;
		a[t].m = mm[t];
		a[t].i = i + t*n;
		a[t].j = j + t*n;
		a[t].x = x + t*n;
		a[t].v = v;
		a[t].res = rs[t];
	}

	pthread_t threads[nth];
	int rc[nth];
	for (t=0;t<nth;t++) rc[t] = pthread_create(&threads[t], NULL, Mul, (void *) &a[t]);
	for (t=0;t<nth;t++) pthread_join(threads[t], NULL);
	for (p=0;p<k;p++) {
		res[p] = 0;
		for (t=0;t<nth;t++) res[p] += rs[t][p];
	}
}


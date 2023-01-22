#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/timeb.h>
#include <time.h>
#include <real.h>

int scale(long m,int *i,int *j,real *x, real *b, real *report,int verb, double tol,double *pppp, int maxiter, int zerodiag, double del);


static void usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s [-m memarray][-p percent][-d del][-v verbose][-s silent][-t tol][-I max_iterations][-z remove_zero_diag] <infile> <outfile>\n", argv0);
  fprintf(stderr, "  <infile>: matrix file in sparse upperc triangular notation\n");
  fprintf(stderr, "  <outfile>: normalization vector output  file\n");
}

int main(int argc, char *argv[]) {
  long m0,m1,m,k,p;
  int q,n,ibad,iter;
  int *i, *j;
  real *x;
  real *z, *z0;
  struct timeb t0,t1,start,end;
  int opt;
  
  // parameters
  double tol=5.0e-4;
  double del=2.0e-2;
  m1 = (long) 7e8; 
  double perc = 1.0e-2;
  int verb=0;
  int silent=1;
  int maxiter=300;
  int zerodiag=0;
  
  while ((opt = getopt(argc, argv, "m:p:d:v:s:t:I:z:h")) != -1) {
    switch (opt) {
    case 'm':
      m1 = (long) atof(optarg);
      break;
    case 'p':
      perc = atof(optarg);
      break;
    case 'd':
      del = atof(optarg);
      break;
    case 't':
      tol = atof(optarg);
      break;
    case 'v':
      verb = atoi(optarg);
      break;
    case 's':
      silent = atoi(optarg);
      break;
    case 'I':
      maxiter=atoi(optarg);
      break;
    case 'z':
      zerodiag=atoi(optarg);
      break;
    case 'h':
      usage(argv[0]);
      exit(EXIT_SUCCESS);
    default:
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  
  if (argc - optind < 2) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  
  FILE *fin = fopen(argv[optind++],"r");
  if (fin==NULL) {
    fprintf(stderr, "Error! File %s cannot be opened for reading\n", argv[optind-1]);
    exit(EXIT_FAILURE);
  }
  else if (verb) printf("Reading from %s", argv[optind-1]);

  FILE *fout = fopen(argv[optind++],"w");
  if (fout==NULL) {
    fprintf(stderr, "Error! File %s cannot be opened for writing\n", argv[optind-1]);
    exit(EXIT_FAILURE);
  }
  else if (verb) printf(" and writing to %s\n", argv[optind-1]);
  
  ftime(&start);

  i = (int *) malloc(m1*sizeof(int));
  if (i == NULL) {
    fprintf(stderr,"Failed to allocate %ld bytes. Exiting\n",m1*sizeof(int));
    exit(EXIT_FAILURE);
  }
  j = (int *) malloc(m1*sizeof(int));
  if (j == NULL) {
    fprintf(stderr,"Failed to allocate %ld bytes. Exiting\n",m1*sizeof(int));
    exit(EXIT_FAILURE);
  }
  x = (real *) malloc(m1*sizeof(real));
  if (x == NULL) {
    fprintf(stderr,"Failed to allocate %ld bytes. Exiting\n",m1*sizeof(real));
    exit(EXIT_FAILURE);
  }
  m = 0; 
  m0 = m1;
  ftime(&t0);
#ifdef fl  
  while(fscanf(fin,"%d %d %f",&i[m],&j[m],&x[m]) == 3) {
#endif  
#ifdef doub  
  while(fscanf(fin,"%d %d %lf",&i[m],&j[m],&x[m]) == 3) {
#endif  
    m++;
    if (m == m1) {
      m1 += m0;
      i = (int *) realloc(i,m1*sizeof(int));
      if (i == NULL) {
        fprintf(stderr,"Failed to allocate additional %ld bytes. Exiting\n",m0*sizeof(int));
        exit(EXIT_FAILURE);
      }
      j = (int *) realloc(j,m1*sizeof(int));
      if (j == NULL) {
        fprintf(stderr,"Failed to allocate additional %ld bytes. Exiting\n",m0*sizeof(int));
        exit(EXIT_FAILURE);
      }
      x = (real *) realloc(x,m1*sizeof(real));
      if (x == NULL) {
        fprintf(stderr,"Failed to allocate additional %ld bytes. Exiting\n",m0*sizeof(real));
        exit(EXIT_FAILURE);
      }
    }
  }
   
  for (p=0;p<m;p++) {
    i[p]--;
    j[p]--;
  }
  ftime(&t1);
  fclose(fin);
  printf("finished reading\n");
  printf("took %ld seconds to read %ld records\n",(long) (t1.time - start.time),m);

  k = 0;
  for (p=0; p<m;p++) if (j[p] > k) k=j[p];
  k++;

  for (p=0; p<m;p++) if (j[p] == i[p]) x[p] *= 0.5;

  real *b = (real *) malloc((k+1)*sizeof(real));
  real *report = (real *) malloc((maxiter+3+100)*sizeof(real));
  
  ftime(&t1);
  iter = scale(m,i,j,x,b,report,1-silent,tol,&perc,maxiter,zerodiag,del);

  ftime(&end);
  printf("took %ld seconds\n",(long) (end.time - start.time));
  printf("iterations took %15.10lf seconds\n",((real) (end.time - t1.time)) + 0.001*(end.millitm - t1.millitm) );
  if (silent) for (p=0;p<abs(iter);p++) printf("%d: %30.15lf\n",(int) (p+1),report[p]);
  printf("final p is %g; final error in scaling vector is %g and in row sums is %g\n",perc,report[maxiter+1],report[maxiter+2]);
  for (p=0;p<k;p++) fprintf(fout,"%30.15lf\n",b[p]);
  fclose(fout);

  return(iter);
}


#include <cstring>
#include <iostream>
#include <fstream>
#include <straw.h>
#include <sys/timeb.h>
#include <time.h>
#include <cmath>
#include <unistd.h>
using namespace std;

int scale(long m,int *i,int *j,float *x, double *z,double *b, double *report,int *allIters, double tol,double *pppp,double *pppp1, int maxiter, int zerodiag, double del, double dp, double dp1,int *totIter, int threads, int k);

int getMatrix(string fname, int binsize, string norm, int **i, int **j, float **x, long *m, string ob, string chr);

double ppNormVector(long m,int *ii,int *jj,float *xx, double *b,int k, int threads, double **space);

static void usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s [-o oe][-p percent][-P delta_p][-q percent1][-Q delta_q][-v verbose][-t tol][-I max_iterations][-z remove_zero_diag][-d delta][-A All_iterations][-T threads] <hicfile> <chromosome> <resolution> <outfile> <[vector_file]>\n", argv0);
  fprintf(stderr, "  <hicfile>: hic file\n");
  fprintf(stderr, "  <chr>: chromosome\n");
  fprintf(stderr, "  <resolution>: resolution in bp\n");
  fprintf(stderr, "  <outfile>: normalization vector output  file\n");
  fprintf(stderr, "  [optional] <vecor_file>: vecor to scale to (otherwise balancing)\n");
}

int main(int argc, char *argv[]) {
	string norm("NONE");
	struct timeb t0,t1;

	string ob("observed");
	double tol=5.0e-4;
	double del=2.0e-2;
	double perc = 1.0e-2;
	double dp = 5.0e-3;
	double perc1 = 0.25e-2;
	double dp1 = 1.0e-3;
	int maxiter=100;
	int zerodiag=0;
	int All_iter = 200;
	int threads = 1;
	int verb = 1;
	int opt;

	while ((opt = getopt(argc, argv, "o:p:P:q:Q:v:t:d:I:z:A:T:h")) != -1) {
		switch (opt) {
				case 'o':
                                        if (strcmp(optarg,"oe") == 0) ob = "oe";
                                        else {
                                                usage(argv[0]);
                                                exit(EXIT_FAILURE);
                                        }
                                        break;
				case 'p':
					perc = atof(optarg);
					break;
				case 'P':
					dp = atof(optarg);
					break;
				case 'q':
					perc1 = atof(optarg);
					break;
				case 'Q':
					dp1 = atof(optarg);
					break;
				case 't':
					tol = atof(optarg);
					break;
				case 'd':
					del = atof(optarg);
					break;
				case 'v':
					verb = atoi(optarg);
					break;
				case 'A':
				      All_iter=atoi(optarg);
					break;
				case 'I':
					maxiter=atoi(optarg);
					break;
				case 'z':
					zerodiag=atoi(optarg);
					break;
				case 'T':
					threads=atoi(optarg);
					break;
				case 'h':
					usage(argv[0]);
					exit(EXIT_SUCCESS);
				default:
					usage(argv[0]);
					exit(EXIT_FAILURE);
		}
	}

	if (argc - optind < 4) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	string fname = argv[optind++];
	if (verb) printf("Reading hic file from %s\n", argv[optind-1]);
	string chr = argv[optind++];
	int binsize = atoi(argv[optind++]);
	char *out_name = argv[optind++];

	FILE *fout = fopen(out_name,"w");
	if (fout==NULL) {
		fprintf(stderr, "Error! File %s cannot be opened for writing\n", argv[optind-1]);
		exit(EXIT_FAILURE);
	  }
	  else if (verb) printf("Writing norm vector to %s\n", argv[optind-1]);

	FILE *fscal;
	bool scaling = false;
	if (argc - optind > 0) scaling = true;
	if (scaling) {
		char *scal_name = argv[optind++];
		fscal = fopen(scal_name,"r");
		if (fscal==NULL) {
			fprintf(stderr, "Error! File %s cannot be opened for reading\n", argv[optind-1]);
			exit(EXIT_FAILURE);
	  	}
	  	else if (verb) printf("Reading target vector from %s\n", argv[optind-1]);
	}
	printf("\n");

	int *ii;
	int *jj;
	float *xx;
	long p;
	long m;
	int k;

	ftime(&t0);
	k = getMatrix(fname, binsize, norm, &ii, &jj, &xx, &m, ob, chr);
	ftime(&t1);
	if (k < 0) {
		cerr << "Error! File " << fname << " cannot be opened for reading" << endl;
		exit(EXIT_FAILURE);
	}
	else if (k == 0) {
		cerr << "Error! chromosome " << chr << " is not found in " << fname << endl;
		exit(EXIT_FAILURE);
	}
	if (verb) printf("took %ld seconds for %ld records\n",(long) (t1.time - t0.time),m);

	for (p=0; p<m;p++) if (jj[p] == ii[p]) xx[p] *= 0.5;

	double *z = (double *) malloc(k*sizeof(double));
	string norm_name;
	if (scaling) {
		norm_name = "SCAL";
		int n = 0;
		while(fscanf(fscal,"%lf",&z[n]) == 1 && n < k) n++;
		if (n < k) for (p=n;p<k;p++) z[p] = NAN;
		fclose(fscal);
	}
	else {
		norm_name = "BAL";
		for (p=0;p<k;p++) z[p] = 1.0;
		perc1 = 0;
		dp1 = 0;	
	}

	double *b = (double *) malloc(k*sizeof(double));
	for (p=0;p<k;p++) b[p] = NAN;
	int totalIt = All_iter;
	double *report = (double *) malloc((All_iter+3+100)*sizeof(double));
	int *allIters = (int *) malloc((All_iter+3+100)*sizeof(int));

	int iter;
	ftime(&t0);
	iter = scale(m,ii,jj,xx, z,b, report,allIters, tol,&perc,&perc1, maxiter, zerodiag, del, dp, dp1, &totalIt,threads,k);
	ftime(&t1);

	if (verb) printf("iterations took %15.10lf seconds\n",((double) (t1.time - t0.time)) + 0.001*(t1.millitm - t0.millitm));
	if (verb > 1) for (p=0;p<totalIt;p++) printf("%d: %30.15lf\n",allIters[p],report[p]);
	if (verb) printf("total %d iterations; final perc = %g and perc1 = %g\n",totalIt,perc,perc1);
	if (verb) printf("final error in scaling vector is %g and in row sums is %g\n",report[totalIt+1],report[totalIt+2]);

	double **space = (double **) malloc(threads*sizeof(double *));
	for (p=0;p<threads; p++) space[p] = (double *) malloc(k*sizeof(double));
	double sum = ppNormVector(m,ii,jj,xx,b,k,threads,space);

	fprintf(fout,"vector %s %s %d BP\n",const_cast<char*> (norm_name.c_str()),const_cast<char*> (chr.c_str()),binsize);
	for (int p=0;p<k;p++) {
		if (!isnan(b[p])) fprintf(fout,"%20.10f\n",b[p]);
		else fprintf(fout,"%20s\n","NaN");
	}
	fclose(fout);
	return(iter);
}


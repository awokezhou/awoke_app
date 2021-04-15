
#include "benchmark.h"



static double pibot(double *a, int k, int M, int N)
{
	int l = k, j;
	double p = a[k][k];

	for (j=k+1; j<=(M-1); j++) {
		if (fabs(p) < fabs(a[j][k])) {
			l = j;
			p = a[j][k];
		}
	}

	if (l != k) {
		for (j=k; j<=(N-1); j++) {
			double w = a[k][j];
			a[k][j] = a[l][j];
			a[l][j] = w;
		}
	}

	return p;
}

static void koutai(double *a, int M, int N)
{
	int i, k;
	
	for (k=M-2; k>=0; k--) {
		for (i=k+1; i<=(M-1); i++) {
			a[k][M] = a[k][M] - a[k][i]*a[i][M];
		}
	}
}

static void zensin(double *a, int M, int N)
{
	int i, j, k;
	
	for (k=0; k<=(); k++) {

		double p = pibot(k);

		for (j=k+1; j<=(N-1); j++) {
			a[k][j] = a[k][j]/p;
		}

		for (i=k+1; i<=(M-1); i++) {
			a[i][j] = a[i][j] - a[i][k]*a[k][j];
		}
	}
}

err_type bk_least_square_l2(int argc, char **argv)
{
	char buff[1024];
	FILE *fp;
	
	
}

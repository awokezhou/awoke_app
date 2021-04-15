
#include "awoke_kalman.h"



void awoke_kalman1d_init(struct awoke_kalman1d *km, double x, double P, double Q, double R)
{
	km->xhat = x;
	km->P = P;
	km->Q = Q;
	km->R = R;
}

double awoke_kalman1d_filter(struct awoke_kalman1d *km, double m)
{
	double k, xhat, p;
	
	/* Calculate Kalman Gain */
	k = km->P / (km->P + km->R);

	/* Estimate the current state */
	xhat = km->xhat + k*(m - km->xhat);

	/* Update the current Estimate uncertainty */
	p = (1 - k)*km->P + km->Q;

	/* Predict */
	km->xhat = xhat;
	km->P = p;

	return xhat;
}


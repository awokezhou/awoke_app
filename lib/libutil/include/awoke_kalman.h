
#ifndef __AWOKE_KALMAN_H__
#define __AWOKE_KALMAN_H__



struct awoke_kalman1d {
	double xhat;	/* estimate */
	double P;		/* estimate uncertainty */
	double Q;		/* process noice */
	double R;		/* measurements uncertainty */
	int count;
};


void awoke_kalman1d_init(struct awoke_kalman1d *km, double x, double P, double Q, double R);
double awoke_kalman1d_filter(struct awoke_kalman1d *km, double m);

#endif /* __AWOKE_KALMAN_H__ */
#include <stdio.h>
#include <math.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_multiroots.h>

#define CP_DFM CP_AIR /* specific heat of dry mixture at constant pressure at 273K [kJ/(kg*K)] --- CP_DFM=CP_AIR assumes no fuel */
#define CP_AIR 1.006 /* specific heat of dry air at constant pressure at 273K [kJ/(kg*K)] */
#define CP_VAP 1.805 /* specific heat of water vapor at constant pressure at 273K [kJ/(kg*K)] */

#define L_W 2501.0 /* enthalpy of vaporisation of water at T=273K [kJ/kg] */

#define A_W (M_W/M_AIR)
#define M_W 18.0153 /* molar mass of water [g/mol] */
#define M_AIR 28.9645 /* molar mass of air [g/mol] */

#define ABS_ERR_BOUND 1e-7 /* absolute error bound for root solver */
#define MAXITER 1000 /* maximum iterations for root solver */

#define T_AMBIENT 298.0 /* ambient temperature [K] */

struct theta_wb_params {
	double h1;
	double p2;
};

double mixture_specific_enthalpy(double t, double w);
double wet_bulb_temp(double h1, double p2);
double eq_vapor_pressure(double t);
int theta_wb(const gsl_vector *x, void *params, gsl_vector *f);
double eq_specific_water_content(double p, double t);

int
main(int argc, char *argv[]) {
	double t1 = 170.0 + 273.15; /* K */
	double w1 = 0.0; /* dry air */
	double h1 = mixture_specific_enthalpy(t1, w1);
	double p2 = 2e5; /* Pa */
	printf("t1 = %f *C\n", t1-273.15);
	printf("p2 = %f\n", p2);
	double t_wb = wet_bulb_temp(h1, p2);
	printf("h1: %f\nwet bulb temp: %f *C\n", h1, t_wb-273.15);
	double w_eq = eq_specific_water_content(p2, t_wb);
	printf("w_eq = %f\n", w_eq);
}

/* specific enthalpy of mixture h [kJ/kg] at temperature t [K] and specific water content w */
double
mixture_specific_enthalpy(double t, double w) {
	return (CP_DFM + w*CP_VAP)*t + w*L_W;
}

double
wet_bulb_temp(double h1, double p2) {
	struct theta_wb_params params = {h1, p2};
	gsl_multiroot_function f = {&theta_wb, 1, &params};

	double x_init = T_AMBIENT;
	gsl_vector *x = gsl_vector_alloc(1);
	gsl_vector_set(x, 0, x_init);

	const gsl_multiroot_fsolver_type *t = gsl_multiroot_fsolver_dnewton;
	gsl_multiroot_fsolver *s = gsl_multiroot_fsolver_alloc(t, 1);
	gsl_multiroot_fsolver_set(s, &f, x);

	int status;
	size_t iter = 0;
	do {
		iter++;
		status = gsl_multiroot_fsolver_iterate(s);
		if (status) {
			break;
		}
		status = gsl_multiroot_test_residual(s->f, ABS_ERR_BOUND);
	} while (status == GSL_CONTINUE && iter < MAXITER);

	double res =  gsl_vector_get(s->x, 0);
	gsl_multiroot_fsolver_free(s);
	gsl_vector_free(x);
	return res;
}

int
theta_wb(const gsl_vector *x, void *params, gsl_vector *f) {
	double h1 = ((struct theta_wb_params *) params)->h1;
	double p2 = ((struct theta_wb_params *) params)->p2;
	double t = gsl_vector_get(x, 0);

	double w_eq = eq_specific_water_content(p2, t);

	double y = CP_DFM*t + w_eq*(CP_VAP*t + L_W) - h1;

	gsl_vector_set(f, 0, y);

	return GSL_SUCCESS;
}

/* equilibrium specific water content at temperature t [K] and pressure p [Pa] */
double
eq_specific_water_content(double p, double t) {
	double p_eq = eq_vapor_pressure(t);
	return A_W * p_eq/(p - p_eq);
}

/* equilibrium vapor pressure [Pa] at temperature t [K] according to Wexler 1976 */
double
eq_vapor_pressure(double t) {
	return 1.0
		/ exp(2.9912729e3 / pow(t, 2))
		/ exp(6.0170128e3 / t)
		* exp(1.887643845e1)
		/ exp(2.8354721e-2 * t)
		* exp(1.7838301e-5 * pow(t, 2))
		/ exp(8.4150417e-10 * pow(t, 3))
		* exp(4.4412543e-13 * pow(t, 4))
		* exp(2.858487 * log(t));
}


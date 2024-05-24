#include <stdio.h>
#include <math.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_multiroots.h>

#define CP_DFM CP_AIR /* specific heat of dry mixture at constant pressure at 273K [kJ/(kg*K)]
						 --- CP_DFM=CP_AIR assumes no fuel */
#define CP_AIR 1.006 /* specific heat of dry air at constant pressure at 273K [kJ/(kg*K)] */
#define CP_VAP 1.805 /* specific heat of water vapor at constant pressure at 273K [kJ/(kg*K)] */

#define L_W 2501.0 /* enthalpy of vaporisation of water at T=273K [kJ/kg] */

#define A_W (M_W/M_AIR)
#define M_W 18.0153 /* molar mass of water [g/mol] */
#define M_AIR 28.9645 /* molar mass of air [g/mol] */

#define ABS_ERR_BOUND 1e-7 /* absolute error bound for root solver */
#define MAXITER 1000 /* maximum iterations for root solver */

#define T_AMBIENT 298.0 /* ambient temperature [K] */

#define N_P_BRPOINTS 11
#define N_RPM_BRPOINTS 8
#define V_DISP 2000e-6 /* displaced volume [m^3] */
#define T_REF 323.15 /* reference air temperature [K] */
#define ROH_W_REF  997.0 /* reference water density at T=25*C [kg/m^3] */
#define P_W_REF 689475.7 /* reference water pressure [Pa] */
#define V_RATE_MAX_W_REF (340e-6/60) /* maximum water flow rate at P_W_REF [m^3/s] */

/* types */

/* web bulb temperature function parameters */
struct t_wb_params {
	double h1; /* specific enthalpy at point 1 */
	double p2; /* absolute pressure at point 2 */
};

/* function declarations */
unsigned int duty_cycle(double p, double t, unsigned int s);
double m_rate_w(double p, double t, unsigned int s);
double m_rate_air(double p, double t, unsigned int s);
double ve(double p, unsigned int s);
double mixture_specific_enthalpy(double t, double w);
double wet_bulb_temp(double h1, double p2);
double eq_vapor_pressure(double t);
int t_wb(const gsl_vector *x, void *params, gsl_vector *f);
double eq_specific_water_content(double p, double t);

/* global constants */
const double p_brpoints[N_P_BRPOINTS] = {500e2, 750e2, 1000e2, 1250e2, 1500e2, 1750e2, 2000e2, 2250e2, 2500e2, 2750e2, 3000e2};
const unsigned int rpm_brpoints[N_RPM_BRPOINTS] = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000};
const unsigned int ve_tbl[N_P_BRPOINTS][N_RPM_BRPOINTS] = {
	{75, 80, 85, 90, 95, 95, 93, 90},
	{75, 80, 85, 90, 95, 95, 93, 90},
	{75, 80, 85, 90, 95, 95, 93, 90},
	{75, 80, 85, 90, 95, 95, 93, 90},
	{75, 80, 85, 90, 95, 95, 93, 90}, /*  -  */
	{75, 80, 85, 90, 95, 95, 93, 90}, /*  P  */
	{75, 80, 85, 90, 95, 95, 93, 90}, /*  +  */
	{75, 80, 85, 90, 95, 95, 93, 90},
	{75, 80, 85, 90, 95, 95, 93, 90},
	{75, 80, 85, 90, 95, 95, 93, 90},
	{75, 80, 85, 90, 95, 95, 93, 90}
	/*           - rpm +          */
};

/* function definitions */

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


	printf("\nDuty cycle table:\n");
	double p;
	unsigned int i, j, s;
	for (i = 0; i < N_P_BRPOINTS; i++) {
		printf("%4.0f ", p_brpoints[i]*1e-2);
		for (j = 0; j < N_RPM_BRPOINTS; j++) {
			p = p_brpoints[i];
			s = rpm_brpoints[j];
			printf("%4d ", duty_cycle(p, T_REF, s));
		}
		putchar('\n');
	}
	printf("%4s ", "");
	for (j = 0; j < N_RPM_BRPOINTS; j++) {
		printf("%4d ", rpm_brpoints[j]);
	}
	putchar('\n');
}

/* duty cycle (0-100) at air pressure p [Pa], air temperature t [K], and engine speed s [rpm] */
unsigned int
duty_cycle(double p, double t, unsigned int s) {
	return 100.0 * m_rate_w(p, t, s) / ROH_W_REF / V_RATE_MAX_W_REF;
}

/* mass flow rate of water at air pressure p [Pa], air temperature t [K], and engine speed s [rpm] */
double
m_rate_w(double p, double t, unsigned int s) {
	double h1, t_wb, w_eq;

	h1 = mixture_specific_enthalpy(t, 0.0);
	t_wb = wet_bulb_temp(h1, p);
	w_eq = eq_specific_water_content(p, t_wb);
	return w_eq * m_rate_air(p, t, s);
}

/* mass flow rate of air at pressure p [Pa], temperature t[K], and engine speed s [rpm] */
double
m_rate_air(double p, double t, unsigned int s) {
	return 3.483e-3 * p * V_DISP * ve(p, s) * (double) s / t / 120.0;
}

/* volumetric efficiency at air pressure p [Pa] and engine speed s [rpm] */
double
ve(double p, unsigned int s) {
	/* TODO */
	return 1.0;
}

/* specific enthalpy of mixture h [kJ/kg] at temperature t [K] and specific water content w */
double
mixture_specific_enthalpy(double t, double w) {
	return (CP_DFM + w*CP_VAP)*t + w*L_W;
}

/* wet bulb temperature [K] given specific mixture enthalpy h1 [kJ/kg] and air pressure p2 [Pa] */
double
wet_bulb_temp(double h1, double p2) {
	struct t_wb_params params = {h1, p2};
	gsl_multiroot_function f = {&t_wb, 1, &params};

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

/* wet bulb temperature function to be solved by GSL */
int
t_wb(const gsl_vector *x, void *params, gsl_vector *f) {
	double h1 = ((struct t_wb_params *) params)->h1;
	double p2 = ((struct t_wb_params *) params)->p2;
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


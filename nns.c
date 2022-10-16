// You can only use the statements #include<***.h>, NOT #include " * *.h"
#include <math.h>

#define l3 0.1
#define l4 0.5
#define l5 15.0
#define l6 4.0
#define l7 0.5

// Upper and lower bounds of the search space for system parameters
const double LMAX[] = {0.8910, 0.740};
const double LMIN[] = {0.8905, 0.705};

// for state variables
const double XMAX[] = {40.0, 40.0, 40.0, 40.0, 1.0};
const double XMIN[] = {-40.0, -40.0, -40.0, -40.0, -1.0};

// Evolution functions of the system
// Updates the values of the state variables array x with the parameters array
// l.
double g(double u, double t) {
    return 1.0 / (1.0 + exp(-(u - t) / l3));
}

void next_x(double *x, double *l) {
    double l1 = l[0];
    double l2 = l[1];

    double ex[5];

    ex[0] = l4 * x[0] + 2.0 + l5 * (g(x[0] + x[1], 0.0) - g(x[4], 0.5));
    ex[1] = l1 * x[1] - l6 * g(x[0] + x[1], 0) + l7;
    ex[2] = l4 * x[2] + 2.0 + l5 * (g(x[2] + x[3], 0.0) - g(x[4], 0.5));
    ex[3] = l1 * x[3] - l6 * g(x[2] + x[3], 0) + l7;
    ex[4] =
        l2 * (g(g(x[0] + x[1], l5) + g(x[2] + x[3], l5), 0.0) - x[4]);

    // memcpy(x, ex, N*sizeof(double));
    for (int p = 0; p < 5; p++) x[p] = ex[p];
    return;
}

// You can only use the statements #include<***.h>, NOT #include " * *.h"
#include <math.h>

// Upper and lower bounds of the search space for system parameters
const double LMAX[] = {0.65, 2.0};
const double LMIN[] = {0.35, 0.5};

// for state variables
const double XMAX[] = {1.0};
const double XMIN[] = {0.0};

// Evolution functions of the system
// Updates the values of the state variables array x with the parameters array l.
void next_x(double *x, double *l) {
    x[0] = x[0] + l[0] - l[1] / 2.0 / M_PI * sin(2.0 * M_PI * x[0]);
    x[0] -= (int)x[0];
    return;
}
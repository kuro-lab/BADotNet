// You can only use the statements #include<***.h>, NOT #include " * *.h"
#include <math.h>

// Upper and lower bounds of the search space for system parameters
const double LMAX[] = {1.6, 0.28};
const double LMIN[] = {1.4, 0.08};

// for state variables
const double XMAX[] = {2.0, 2.0};
const double XMIN[] = {-2.0, -2.0};

// Evolution functions of the system
// Updates the values of the state variables array x with the parameters array l.
void next_x(double *x, double *l) {
    double px = x[0];
    double py = x[1];
    x[0] = 1 - l[0] * px * px + py;
    x[1] = l[1] * px;
    return;
}
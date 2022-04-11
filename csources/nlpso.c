#include "common.h"
#include "config.h"

#define W 0.729
#define CP 1.49445
#define CG 1.49445

#ifndef _OPENMP
int omp_get_thread_num(){
    return 0;
}

void omp_set_num_threads(int n){
    return;
}
#endif

extern const double LMAX[];
extern const double LMIN[];
extern const double XMAX[];
extern const double XMIN[];
void next_x(double *x, double *l);

// rand.c
extern rand_t gen[];
void rand_seed(unsigned int seed);
double rand_next_double(rand_t *gen, double min, double max);

// watch.c
double get_cputime(void);
double get_realtime(void);
char *elapsed_span(double elapsed);

double zbif[PARALLELISM][Dbif];
double zpp[PARALLELISM][Dpp];

void write_stream(int fileIdx, char *str, bool append) {
    char fn[50];
    switch (fileIdx) {
        case 0:
            sprintf(fn, "out/%s%d.csv", MU == -1 ? "I" : "G", PERIOD);
            break;
        case 1:
            sprintf(fn, "out/timeSpan_%s%d.csv", MU == -1 ? "I" : "G", PERIOD);
            break;
    }

    FILE *file = fopen(fn, append ? "a" : "w");
    fprintf(file, "%s", str);
    fclose(file);
    return;
}

double f_pp(double* x){
    int threadIdx = omp_get_thread_num();
    for(int d=0; d<Dpp; d++){
        zpp[threadIdx][d] = x[d];
    }

    double xk[Dpp];
    for(int d=0; d<Dpp; d++){
        xk[d] = x[d];
    }

    for(int k=0; k<PERIOD; k++){
        next_x(xk, zbif[threadIdx]);
    }

    double mag = 0.0;
    for (int d = 0; d < Dpp; d++) {
        xk[d] -= x[d];
        mag += xk[d] * xk[d];
    }

    // Squared Euclidian distance
    return mag;

    // Euclidian distance
    //  return sqrt(d);
}

void mul_mat(double l[Dpp][Dpp], double r[Dpp][Dpp]) {
    double n[Dpp][Dpp];
    for (int i = 0; i < Dpp; i++) {
        for (int k = 0; k < Dpp; k++) {
            n[i][k] = 0.0;
        }
    }
    for (int i = 0; i < Dpp; i++) {
        for (int k = 0; k < Dpp; k++) {
            for (int j = 0; j < Dpp; j++) {
                n[i][j] += l[i][k] * r[k][j];
            }
        }
    }
    for (int i = 0; i < Dpp; i++) {
        for (int k = 0; k < Dpp; k++) {
            l[i][k] = n[i][k];
        }
    }
    return;
}

double determinant(double a[Dpp][Dpp]) {
    if (Dpp == 1) {
        return a[0][0];
    }

    if (Dpp == 2) {
        return a[0][0] * a[1][1] - a[0][1] * a[1][0];
    }

    // LU decomposition
    double l[Dpp][Dpp];
    double u[Dpp][Dpp];

    for (int i = 0; i < Dpp; i++) {
        l[i][i] = 1.0;
    }

    for (int i = 0; i < Dpp; i++) {
        for (int j = 0; j < Dpp; j++) {
            if (i <= j) {
                double sum1 = 0.0;
                for (int k = 0; k <= i - 1; k++) {
                    sum1 += l[i][k] * u[k][j];
                }
                u[i][j] = a[i][j] - sum1;
            } else if (i > j) {
                double sum2 = 0.0;
                for (int k = 0; k <= j - 1; k++) {
                    sum2 += l[i][k] * u[k][j];
                }
                l[i][j] = (a[i][j] - sum2) / u[j][j];
            }
        }
    }

    double det = 1.0;
    for (int i = 0; i < Dpp; i++) {
        det *= u[i][i];
    }
    return det;
}

double particle_swarm_optimize(int dim, int population, int tmax, double (*f)(double*), double criterion, const double* min, const double* max, int* out_t){
    double z[population][dim];
    double v[population][dim];
    double err[population];
    double pbest[population][dim];
    double pberr[population];
    double gbest[dim];
    double gberr = DBL_MAX;

    int threadIdx = omp_get_thread_num();

    // init
    for (int i = 0; i < population; i++) {
        for (int d = 0; d < dim; d++) {
            z[i][d] = rand_next_double(&gen[threadIdx], min[d], max[d]);
            v[i][d] = 0.0;
        }
        err[i] = DBL_MAX;
        pberr[i] = DBL_MAX;
    }

    // main loop
    for(int t=0; t<tmax; t++){
        for(int i=0; i<population; i++){
            err[i] = f(z[i]);

            // update pbest
            if(err[i] < pberr[i]){
                for(int d=0; d<dim; d++){
                    pbest[i][d] = z[i][d];
                }
                pberr[i] = err[i];
            }

            // update gbest
            if(err[i] < gberr){
                for(int d=0; d<dim; d++){
                    gbest[d] = z[i][d];
                }
                gberr = err[i];
            }

            // judgement
            if(gberr < criterion){
                *out_t = t;
                for(int d=0; d<Dbif; d++){
                    zbif[threadIdx][d] = z[i][d];
                }
                return gberr;
            }
        }

        for(int i=0; i<population; i++){
            for (int d = 0; d < dim; d++) {
                double r1 = rand_next_double(&gen[threadIdx], 0.0, CP);
                double r2 = rand_next_double(&gen[threadIdx], 0.0, CG);
                v[i][d] = W * v[i][d] + r1 * (pbest[i][d] - z[i][d]) +
                          r2 * (gbest[d] - z[i][d]);
                double nx = z[i][d] + v[i][d];
                //
                if (nx < min[d] || max[d] < nx) {
                    z[i][d] = rand_next_double(&gen[threadIdx], min[d], max[d]);
                    v[i][d] = 0.0;
                }
                z[i][d] += v[i][d];
            }
        }
    }

    *out_t =  -1;
}

double f_bif(double* l){
    int threadIdx = omp_get_thread_num();
    for(int d=0; d<Dbif; d++){
        zbif[threadIdx][d] = l[d];
    }

    int tpp = -1;
    double pperr = particle_swarm_optimize(Dpp, Mpp, Tpp, f_pp, Cpp, XMIN, XMAX, &tpp);

    if(tpp < 0){
        return pperr / Cpp / Cpp;
    }

    // numerical differentiation
    const double h = 1e-5;
    double m[PERIOD][Dpp][Dpp];

    double xk[Dpp];
    double prevx[Dpp];
    double qx[Dpp];

    for (int d = 0; d < Dpp; d++) {
        xk[d] = zpp[threadIdx][d];
    }

    for (int k = 0; k < PERIOD; k++) {
        for (int d = 0; d < Dpp; d++) {
            prevx[d] = xk[d];
        }

        next_x(xk, l);

        for (int i = 0; i < Dpp; i++) {
            for (int d = 0; d < Dpp; d++) {
                qx[d] = prevx[d];
            }
            qx[i] += h;
            next_x(qx, l);
            for (int j = 0; j < Dpp; j++) {
                m[k][j][i] = (qx[j] - xk[j]) / h;
            }
        }
    }

    double jacobi[Dpp][Dpp];
    for (int i = 0; i < Dpp; i++) {
        for (int k = 0; k < Dpp; k++) {
            jacobi[i][k] = m[PERIOD - 1][i][k];
        }
    }

    for (int k = 1; k < PERIOD; k++) {
        mul_mat(jacobi, m[PERIOD - 1 - k]);
    }

    for (int i = 0; i < Dpp; i++) {
        jacobi[i][i] -= MU;
    }

    double d = determinant(jacobi);

    return d > 0 ? d : -d;
}

int main(){
    rand_seed((unsigned int)time(NULL));
    omp_set_num_threads(PARALLELISM);

    char str0[200];
    sprintf(str0, "t,");
    for (int i = 1; i <= Dbif; i++) {
        char s[10];
        sprintf(s, "l%d,", i);
        strcat(str0, s);
    }
    for (int i = 1; i <= Dpp; i++) {
        char s[10];
        sprintf(s, "x%d,", i);
        strcat(str0, s);
    }
    strcat(str0, "Fbif\n");
    write_stream(0, str0, false);

    // char str1[] = "trial,suc,time,[s],tbif\n";
    // write_stream(1, str1, false);

    double bif[N][Dbif];
    double pp[N][Dpp];
    int tend[N];
    double err[N];

    double t0 = get_realtime();
    #pragma omp parallel for
    for(int k=0; k<N; k++){
        int threadIdx = omp_get_thread_num();

        err[k] = particle_swarm_optimize(Dbif, Mbif, Tbif, f_bif, Cbif, LMIN, LMAX, &tend[k]);

        for(int d=0; d<Dbif; d++){
            bif[k][d] = zbif[threadIdx][d];
        }
        for(int d=0; d<Dpp; d++){
            pp[k][d] = zpp[threadIdx][d];
        }

        printf("%s %3d %.4e  \n", tend[k] >= 0 ? "I" : "F", tend[k], err[k]);
    }
    double t1 = get_realtime();

    int suc = 0;
    for(int k=0; k<N; k++){
        char str_result[1000];
        sprintf(str_result, "%d,", tend[k]);
        for (int d = 0; d < Dbif; d++) {
            char res[32];
            sprintf(res, "%.10f,", bif[k][d]);
            strcat(str_result, res);
        }
        for (int d = 0; d < Dpp; d++) {
            char res[32];
            sprintf(res, "%.10f,", pp[k][d]);
            strcat(str_result, res);
        }
        char res_err[32];
        sprintf(res_err, "%.4e\n", err[k]);
        strcat(str_result, res_err);
        write_stream(0, str_result, true);

        // suc
        if(tend[k] >= 0) suc++;
    }

    printf("\n");
    printf("%.2fs elapsed.  ", t1 - t0);
    printf("suc: %.2f%%", suc / (double)N * 100);
    return 0;
}
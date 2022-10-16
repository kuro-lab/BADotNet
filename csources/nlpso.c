#include "common.h"
#include "config.h"

#define W 0.729
#define CP 1.49445
#define CG 1.49445

#ifndef _OPENMP
int omp_get_thread_num() { return 0; }

void omp_set_num_threads(int n) { return; }
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

void write_stream(int fileIdx, char *str, bool append) {
    char fn[256];
    switch (fileIdx) {
        case 0:
            sprintf(fn, "out/%d/%s%d.csv", PARALLELISM, MU == -1 ? "I" : "G",
                    PERIOD);
            break;
        case 1:
            sprintf(fn, "out/%d/timeSpan_%s%d.csv", PARALLELISM,
                    MU == -1 ? "I" : "G", PERIOD);
            break;
    }

    FILE *file = fopen(fn, append ? "a" : "w");
    fprintf(file, "%s", str);
    fclose(file);
    return;
}

double f_pp(double *x, double *l, int _) {
    double xk[Dpp];
    for (int d = 0; d < Dpp; d++) {
        xk[d] = x[d];
    }

    for (int k = 0; k < PERIOD; k++) {
        next_x(xk, l);
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

double solve(int dim, int population, int tmax,
             double (*f)(double *, double *, int), double criterion,
             const double *min, const double *max, double *param, bool omp,
             int idx, double *out_z, int *out_t) {
    *out_t = -1;

    double z[population][dim];
    double v[population][dim];
    double err[population];
    double pbest[population][dim];
    double pberr[population];
    double gbest[population][dim];
    double gberr[population];
    double _min[population][dim];
    double _max[population][dim];

    // init
    for (int i = 0; i < population; i++) {
        for (int d = 0; d < dim; d++) {
            z[i][d] = rand_next_double(&gen[idx], min[d], max[d]);
            v[i][d] = 0.0;
            _min[i][d] = min[d];
            _max[i][d] = max[d];
        }
        err[i] = DBL_MAX;
        pberr[i] = DBL_MAX;
        gberr[i] = DBL_MAX;
    }

    // main loop
#ifdef _OPENMP
    if (omp) {
        bool finish = false;
#pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < population; i++) {
            for (int t = 0; t < tmax; t++) {
                err[i] = f(z[i], param, 10 * i + 1);

                // update pbest
                if (err[i] < pberr[i]) {
                    for (int d = 0; d < dim; d++) {
                        pbest[i][d] = z[i][d];
                    }
                    pberr[i] = err[i];
                }

                if (finish) break;

                // update gbest
                if (err[i] < gberr[i]) {
#pragma omp critical
                    {
                        if (err[i] < gberr[i]) {
                            for (int p = 0; p < population; p++) {
                                for (int d = 0; d < dim; d++) {
                                    gbest[p][d] = z[i][d];
                                }
                                gberr[p] = err[i];
                            }

                            // judgement
                            if (gberr[i] < criterion) {
                                *out_t = t;
                                for (int d = 0; d < dim; d++) {
                                    out_z[d] = z[i][d];
                                }
                                finish = true;
                            }
                        }
                    }
                }

                for (int d = 0; d < dim; d++) {
                    double r1 = rand_next_double(&gen[10 * i + 1], 0.0, CP);
                    double r2 = rand_next_double(&gen[10 * i + 1], 0.0, CG);
                    v[i][d] = W * v[i][d] + r1 * (pbest[i][d] - z[i][d]) +
                              r2 * (gbest[i][d] - z[i][d]);
                    double nx = z[i][d] + v[i][d];
                    //
                    if (nx < _min[i][d] || _max[i][d] < nx) {
                        z[i][d] = rand_next_double(&gen[10 * i + 1], _min[i][d],
                                                   _max[i][d]);
                        v[i][d] = 0.0;
                    }
                    z[i][d] += v[i][d];
                }
            }
        }
    } else {
#endif
        for (int t = 0; t < tmax; t++) {
            for (int i = 0; i < population; i++) {
                err[i] = f(z[i], param, 0);

                // update pbest
                if (err[i] < pberr[i]) {
                    for (int d = 0; d < dim; d++) {
                        pbest[i][d] = z[i][d];
                    }
                    pberr[i] = err[i];
                }

                // update gbest
                if (err[i] < gberr[i]) {
                    for (int p = 0; p < population; p++) {
                        for (int d = 0; d < dim; d++) {
                            gbest[p][d] = z[i][d];
                        }
                        gberr[p] = err[i];
                    }
                }

                // judgement
                if (gberr[i] < criterion) {
                    *out_t = t;
                    for (int d = 0; d < dim; d++) {
                        out_z[d] = z[i][d];
                    }
                    return gberr[i];
                }
            }

            for (int i = 0; i < population; i++) {
                for (int d = 0; d < dim; d++) {
                    double r1 = rand_next_double(&gen[idx], 0.0, CP);
                    double r2 = rand_next_double(&gen[idx], 0.0, CG);
                    v[i][d] = W * v[i][d] + r1 * (pbest[i][d] - z[i][d]) +
                              r2 * (gbest[i][d] - z[i][d]);
                    double nx = z[i][d] + v[i][d];
                    //
                    if (nx < min[d] || max[d] < nx) {
                        z[i][d] = rand_next_double(&gen[idx], min[d], max[d]);
                        v[i][d] = 0.0;
                    }
                    z[i][d] += v[i][d];
                }
            }
        }
#ifdef _OPENMP
    }
#endif
    for (int d = 0; d < dim; d++) {
        out_z[d] = gbest[0][d];
    }
    return gberr[0];
}

double f_bif(double *l, double *out_pp, int idx) {
    int tpp = -1;
    double pp[Dpp];
    double pperr =
        solve(Dpp, Mpp, Tpp, f_pp, Cpp, XMIN, XMAX, l, false, idx, pp, &tpp);

    if (tpp < 0) {
        return pperr / Cpp / Cpp;
    }

    for (int d = 0; d < Dpp; d++) {
        out_pp[d] = pp[d];
    }

    // numerical differentiation
    const double h = 1e-5;
    double m[PERIOD][Dpp][Dpp];

    double xk[Dpp];
    double prevx[Dpp];
    double qx[Dpp];

    for (int d = 0; d < Dpp; d++) {
        xk[d] = pp[d];
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

int main() {
    rand_seed((unsigned int)time(NULL));
    omp_set_num_threads(PARALLELISM);

    double bif[N][Dbif];
    double pp[N][Dpp];
    int tend[N];
    double err[N];

    double t0 = get_realtime();
    for (int k = 0; k < N; k++) {
        err[k] = solve(Dbif, Mbif, Tbif, f_bif, Cbif, LMIN, LMAX, pp[k], true,
                       0, bif[k], &tend[k]);

        printf("%s %3d %.4e  \n", tend[k] >= 0 ? "I" : "F", tend[k], err[k]);
    }
    double t1 = get_realtime();

    printf("I: search process finished. Exportiong result...\n");

    int suc = 0;
    int tbif = 0;
    for (int k = 0; k < N; k++) {
        char str_result[2000];
        memset(str_result, '\0', sizeof(str_result));
        snprintf(str_result, 2000, "%d,", tend[k]);

        // bifurcation parameter
        for (int d = 0; d < Dbif; d++) {
            char res[100];
            memset(res, '\0', sizeof(res));
            snprintf(res, 100, "%.10f,", bif[k][d]);
            strcat(str_result, res);
        }

        // periodic point
        for (int d = 0; d < Dpp; d++) {
            char res[100];
            memset(res, '\0', sizeof(res));
            snprintf(res, 100, "%.10f,", pp[k][d]);
            strcat(str_result, res);
        }

        // error
        char res_err[100];
        memset(res_err, '\0', sizeof(res_err));
        snprintf(res_err, 100, "%.4e\n", err[k]);
        strcat(str_result, res_err);
        write_stream(0, str_result, true);

        // suc
        if (tend[k] >= 0) {
            tbif += tend[k];
            suc++;
        }
    }

    printf("I: Exportiong timespan...\n");

    char *elapsedTotal = NULL;
    elapsedTotal = elapsed_span(t1 - t0);

    printf("\n");
    printf("%s elapsed.  ", elapsedTotal);
    printf("suc: %.2f%%\n", suc / (double)N * 100);
    printf("mean t_end: %.2f\n", tbif / (double)suc);

    char str_timespan[1000];
    sprintf(str_timespan, "%.2f%%,%s,%.3f,%.3f\n", suc / (double)N * 100,
            elapsedTotal, t1 - t0, tbif / (double)suc);
    write_stream(1, str_timespan, true);
    return 0;
}
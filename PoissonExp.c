#include <stdio.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <time.h>
char * get_time(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    //printf("%s", asctime(tm));
    return asctime(tm);
}

int poisson(int media)
{
    gsl_rng *rng = gsl_rng_alloc(gsl_rng_taus2);
    gsl_rng_set(rng, time(0)); // Seed with time
    int result = gsl_ran_poisson(rng, media);
    gsl_rng_free(rng);
    return result;
}
double exponencial(double media)
{
    gsl_rng *rng = gsl_rng_alloc(gsl_rng_taus2);
    gsl_rng_set(rng, time(0)); // Seed with time
    double result = gsl_ran_exponential(rng, media);
    gsl_rng_free(rng);

    return result;
}
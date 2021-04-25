#include <stdio.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>


int poisson(int media)
{
    const gsl_rng_type * T;
    gsl_rng * r;

    gsl_rng_env_setup();
    T = gsl_rng_default;
    r = gsl_rng_alloc (T);
    int i = 5;
    while (i>0){
    	printf("Poisson: %d\n", gsl_ran_poisson(r, media));
    	--i;
    }
    gsl_rng_free(r);
}
int exponencial(double media)
{
    const gsl_rng_type * T;
    gsl_rng * r;

    gsl_rng_env_setup();
    T = gsl_rng_default;
    r = gsl_rng_alloc (T);
    printf("Exponencial: %f\n", gsl_ran_exponential(r, media));
    printf("%f\n", gsl_ran_exponential(r, media));
    printf("%f\n", gsl_ran_exponential(r, media));
    printf("%f\n", gsl_ran_exponential(r, media));
    printf("%f\n", gsl_ran_exponential(r, media));
    printf("%f\n", gsl_ran_exponential(r, media));

    gsl_rng_free(r);
}
int main(){
    poisson(5);
    exponencial(5);
}

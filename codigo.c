#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <pmmintrin.h>

#define LINESIZE 64
#define L1dSIZE 32768
#define L2SIZE 262144
#define L3SIZE 12582912

#define N 10

void start_counter();

double get_counter();

double mhz();


/* Initialize the cycle counter */


static unsigned cyc_hi = 0;
static unsigned cyc_lo = 0;


/* Set *hi and *lo to the high and low order bits of the cycle counter.
Implementation requires assembly code to use the rdtsc instruction. */
void access_counter(unsigned *hi, unsigned *lo) {
    asm("rdtsc; movl %%edx,%0; movl %%eax,%1" /* Read cycle counter */
    : "=r" (*hi), "=r" (*lo) /* and move results to */
    : /* No input */ /* the two outputs */
    : "%edx", "%eax");
}

/* Record the current value of the cycle counter. */
void start_counter() {
    access_counter(&cyc_hi, &cyc_lo);
}

/* Return the number of cycles since the last call to start_counter. */
double get_counter() {
    unsigned ncyc_hi, ncyc_lo;
    unsigned hi, lo, borrow;
    double result;

    /* Get cycle counter */
    access_counter(&ncyc_hi, &ncyc_lo);

    /* Do double precision subtraction */
    lo = ncyc_lo - cyc_lo;
    borrow = lo > ncyc_lo;
    hi = ncyc_hi - cyc_hi - borrow;
    result = (double) hi * (1 << 30) * 4 + lo;
    if (result < 0) {
        fprintf(stderr, "Error: counter returns neg value: %.0f\n", result);
    }
    return result;
}

double mhz(int verbose, int sleeptime) {
    double rate;

    start_counter();
    sleep(sleeptime);
    rate = get_counter() / (1e6 * sleeptime);
    if (verbose)
        printf("\n Processor clock rate = %.1f MHz\n", rate);
    return rate;
}

void barajar(int *v, int tam) {
    int aux, ran;

    srand(getpid());

    for (int i = 0; i < tam; i++) {
        ran = rand() % tam;
        aux = v[i];
        v[i] = v[ran];
        v[ran] = aux;
    }
}

void quicksort(double *array, int primerInd, int ultimoInd) {
    int i, j, pivote;
    double temp;

    if (primerInd < ultimoInd) {
        pivote = primerInd;
        i = primerInd;
        j = ultimoInd;

        while (i < j) {
            while (array[i] <= array[pivote] && i < ultimoInd)
                i++;
            while (array[j] > array[pivote])
                j--;
            if (i < j) {
                temp = array[i];
                array[i] = array[j];
                array[j] = temp;
            }
        }

        temp = array[pivote];
        array[pivote] = array[j];
        array[j] = temp;
        quicksort(array, primerInd, j - 1);
        quicksort(array, j + 1, ultimoInd);

    }
}


int main() {
    int S1 = ceil((double) L1dSIZE / LINESIZE);
    int S2 = ceil((double) L2SIZE / LINESIZE);
    int S3 = ceil((double) L3SIZE / LINESIZE);

    // Se eligen las columnas de la matriz dependiendo del estudio a realizar
    int C = 8;

    int L = ceil(4 * S3);

    // Se eligen las filas de la matriz dependiendo del estudio a realizar
    int F = ceil((LINESIZE * L) / (C * sizeof(double)));
    //int F = L;
    //int F = ceil((double) L / 3);
    //int F = ceil((double) L / 5);

    double ck, sum;
    int ind[F];
    double red[N], clocks[N];
    int i, j, k;

    srand(getpid());

    // Reserva dinámica de memoria de la matriz
    double **M = _mm_malloc(F * sizeof(double *), LINESIZE);
    for (i = 0; i < F; i++) {
        M[i] = malloc(C * sizeof(double));
    }

    // Incialización y barajada de los índices
    for (i = 0; i < F; i++)
        ind[i] = i;
    barajar(ind, F);

    // Generación aleatoria de los datos de la matriz
    for (i = 0; i < F; i++)
        for (j = 0; j < C; j++)
            M[i][j] = (double) (rand() % 50) / 53;


    printf("\n");

    // Bucle para realizar las N mediciones
    for (k = 0; k < N + 1; k++) {
        sum = 0;
        /*
         * Si k == 0, se realiza la precarga de datos. Este dato será atípico.
         * Por esta razón no se mide la primera iteración.
         */
        if (k != 0)
            start_counter();


        // ESTUDIO 1
        /*for (j = 0; j < F; j++) {
            sum += M[ind[j]][0];
        }*/


        // ESTUDIO 2
        for (i = 0; i < F; i++) {
            for (j = 0; j < C; j += LINESIZE / sizeof(double))
                sum += M[i][j];
        }


        if (k != 0) {
            ck = get_counter();
            red[k - 1] = sum;
            printf("[Rep. %d] Ciclos: %1.10lf\n", k - 1, ck);
            clocks[k - 1] = ck;
        }
    }

    // Impresión de los resultados para evitar optimizaciones del compilador
    printf("\n");
    for (i = 0; i < N; i++)
        printf("red[%d] = %lf\n", i, red[i]);
    printf("\n");

    /* Esta rutina imprime a frecuencia de reloxo estimada coas rutinas start_counter/get_counter */
    mhz(1, 1);

    // Cálculo de la mediana
    quicksort(clocks, 0, N - 1);
    printf("\nMediana de ciclos: %lf\n\n", (clocks[4] + clocks[5]) / 2);

    /*
    // Cálculo de la media
    sum = 0;
    for (i = 0; i < N; i++) {
        sum+=clocks[i];
    }
    printf("\nMedia de ciclos: %lf\n\n", sum/N);
    */

    // Liberación de la memoria de la matriz
    for (i = 0; i < F; i++) {
        free(M[i]);
    }
    _mm_free(M);

    exit(EXIT_SUCCESS);
}

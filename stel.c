#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <errno.h>
#include <fenv.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "lista_ligada.c"

#define lambda 8.0

double delta = (double)(1.0/lambda)/5;

void printhistogram(int *histogram, int histogram_size, int max_count);

int main(int argc, char const *argv[]) {

    lista  * lista_eventos;
	int tipo_ev, i; double tempo_ev;
    double current_time = 0, last_event_time = 0, c, u;

    //  ---------------------------------------------------
    //  ---- Calculate number of bins to the histogram ----
    //  ---------------------------------------------------
    double tmp_n_bins = (1.0/lambda) * 10 / delta;

    if (tmp_n_bins - (int) tmp_n_bins > 0) {
        tmp_n_bins += 1;
    }
    int n_bins = (int)tmp_n_bins;

    int count_bins[n_bins], max_count = 0;
    for (i=0; i < n_bins; i++) count_bins[i] = 0;

    //  ---- Initialize events list ----
	lista_eventos = NULL;
    lista_eventos = adicionar(lista_eventos, 0, 0);

    /* Intializes random number generator */
    time_t t;
    srand((unsigned) time(&t));

    //  ---- Ask and set number of simulations ----
    int n_simulations = 0;
    printf("N simulations (thousands): ");
    scanf("%d", &n_simulations);
    printf("\n");
    n_simulations *= 1000;

    //  ---- Starting simulations ---
    double max = 0, min = -1, avg=0;
    double call_time;

    for (i = 0; i < n_simulations; i++) {

        //  ---- Update times ---
        last_event_time = current_time;
        current_time = lista_eventos->tempo;

        //  ---- Consume event on the list ----
        lista_eventos = remover(lista_eventos);

        //  ---- Random generate time for the next call ----
        int tmp;
        do {
            u = (double)((unsigned)rand() + 1U)/((unsigned)RAND_MAX + 1U);
            c = -log(u) / lambda;
        } while((int)(c / delta) > n_bins);

        //  ---- Calculate max, min and average ----
        // min
        if (min = -1)
            min = c;
        else if (c < min)
            min = c;
        // max
        if (c > max)
            max = c;
        // avg
        avg = ((avg*i) + call_time) / (i+1);

        //  ---- Calculate time betwen calls ----
        call_time = current_time - last_event_time;

        //  ---- Increment bins on histogram array ----
        tmp = (int)(call_time / delta);
        count_bins[tmp]++;
        if (count_bins[tmp] > max_count) max_count = count_bins[tmp];

        // ---- Add new event to the list of events ----
        lista_eventos = adicionar(lista_eventos, 0, current_time + c);
    }

    printf("Max: %lf, Min: %lf, Avg: %lf\n", max, min, avg);

    printhistogram(count_bins, n_bins, max_count);

    return 0;
}

/**
 * Function to print an horizontal bar of some size
 */
void printhbar(int size) {
    int i;
    for (i = 0; i < size; i++) {
        printf("_");
    }
    printf("\n");
}

/**
 * Function to print histogram
 */
void printhistogram(int *histogram, int histogram_size, int max_count){
	int	i, j;
    double val;

    struct winsize size;
    ioctl(STDOUT_FILENO,TIOCGWINSZ,&size);

    float bar_size = (float)(size.ws_col - 35);
    if (bar_size < 50) {
        bar_size = 50.0;
    }

    printhbar(size.ws_col);
	for (i=0; i < histogram_size; i++){
        printf(" %.5lf - %.5lf ( %6d )|", i * delta, (i+1) * delta, histogram[i]);
		for (j=0; j < (int)(histogram[i]*(bar_size/max_count)); j++){
			printf("*");
		}

        printf("\n");
	}
    printhbar(size.ws_col);
}

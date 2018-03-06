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

#define lambda 0.0000025
#define K 20000
#define dm 120.0

double delta = (double)(dm)/5;

void printhistogram(int *histogram, int histogram_size, int max_count);

int main(int argc, char const *argv[]) {

    lista  * lista_eventos;
	int tipo_ev, i; double tempo_ev;
    int n_channels = 0, free_channels = 0, n_simulations = 0, event_type;
    double current_time = 0, last_event_time = 0, c, u, d;

    //  ---------------------------------------------------
    //  ---- Calculate number of bins to the histogram ----
    //  ---------------------------------------------------
    double tmp_n_bins = dm * 10 / delta;

    if (tmp_n_bins - (int) tmp_n_bins > 0) {
        tmp_n_bins += 1;
    }
    int n_bins = (int)tmp_n_bins;

    printf("%d\n", n_bins);

    int count_bins[n_bins], max_count = 0;
    for (i=0; i < n_bins; i++) count_bins[i] = 0;

    //  ---- Initialize events list ----
	lista_eventos = NULL;
    lista_eventos = adicionar(lista_eventos, 0, 0);

    /* Intializes random number generator */
    time_t t;
    srand((unsigned) time(&t));

    //  ---- Ask and set number of simulations ----
    printf("N simulations (thousands): ");
    scanf("%d", &n_simulations);
    printf("\n");
    n_simulations *= 1000;

    //  ---- Ask and set number of channels ----
    printf("N channels: ");
    scanf("%d", &n_channels);
    printf("\n");
    free_channels = n_channels;

    //  ---- Starting simulations ---
    double avg_d=0;

    for (i = 0; i < n_simulations; i++) {

        //  ---- Update times ---
        current_time = lista_eventos->tempo;
        event_type = lista_eventos->tipo;

        //  ---- Consume event on the list ----
        lista_eventos = remover(lista_eventos);


        if (event_type == 0) {

            //  ---- Random generate time for the next call ----
            u = (double)((unsigned)rand() + 1U)/((unsigned)RAND_MAX + 1U);
            c = -log(u) / (lambda*K);
            
            if (free_channels > 0) {
                do {
                    u = (double)((unsigned)rand() + 1U)/((unsigned)RAND_MAX + 1U);
                    d = -dm * log(u);
                } while((int)(d / delta) > n_bins-1);
                free_channels--;

                // ---- Add new event to the list of events ----
                lista_eventos = adicionar(lista_eventos, 1, current_time + d);

                //  ---- Calculate average service time ----
                avg_d = ((avg_d*i) + d) / (i+1);

                //  ---- Increment bins on histogram array ----
                int tmp;
                tmp = (int)(d / delta);
                count_bins[tmp]++;
                if (count_bins[tmp] > max_count) max_count = count_bins[tmp];
                printf("tmp: %d\n", tmp);
            }

            lista_eventos = adicionar(lista_eventos, 0, current_time + c);
        } else {
            free_channels++;
        }

        imprimir(lista_eventos);
        printf("Free channels: %d\n", free_channels);
    }

    printf("Avg service time: %lf\n", avg_d);

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

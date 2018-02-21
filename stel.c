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


int lambda = 8;

void printhistogram(double *vetor, int vetor_size, double max);

int main(int argc, char const *argv[]) {

    lista  * lista_eventos;
	int tipo_ev, i; double tempo_ev;
    double current_time = 0, last_event_time = 0, c, u;
    // int n_clients, n_servers;
	lista_eventos = NULL;
    lista_eventos = adicionar(lista_eventos, 0, 0);

    /* Intializes random number generator */
    time_t t;
    srand((unsigned) time(&t));

    int n_simulations = 0;
    // printf("N clients: ");
    // scanf("%d\n", &n_simulations);
    //
    // printf("N servers: ");
    // scanf("%d\n", &n_simulations);

    printf("N simulations (thousands): ");
    scanf("%d", &n_simulations);
    printf("\n");
    n_simulations *= 1000;
    double max = 0, min = -1;
    double times_array[n_simulations];

    for (i = 0; i < n_simulations; i++) {

        last_event_time = current_time;
        current_time = lista_eventos->tempo;
        lista_eventos = remover(lista_eventos);

        u = (double)((unsigned)rand() + 1U)/((unsigned)RAND_MAX + 1U);
        c = -log(u) / lambda;

        if (min = -1)
            min = c;
        else if (c < min)
            min = c;

        if (c > max)
            max = c;

        times_array[i] = current_time - last_event_time;

        lista_eventos = adicionar(lista_eventos, 0, current_time + c);
    }

    printf("Max: %lf, Min: %lf\n", max, min);

    printhistogram(times_array, n_simulations, max);

    return 0;
}

void printhbar(int size) {
    int i;
    for (i = 0; i < size; i++) {
        printf("_");
    }
    printf("\n");
}

void printhistogram(double *vetor, int vetor_size, double max){
	int	i, j;
    double val, delta = ((1.0/lambda)/5);
    double tmp_n_bins = max / delta;

    struct winsize size;
    ioctl(STDOUT_FILENO,TIOCGWINSZ,&size);

    if (tmp_n_bins - (int) tmp_n_bins > 0) {
        tmp_n_bins += 1;
    }
    int n_bins = (int)tmp_n_bins;
    char tmp;

    int count_bins[n_bins], max_count = 0;
    for (i=0; i < n_bins; i++) count_bins[i] = 0;

	for( i = 0 ; i < vetor_size ; i++ ) {
		val	=	vetor[i];
		int tmp = (int)(val / delta);
        count_bins[tmp]++;
        if (count_bins[tmp] > max_count) max_count = count_bins[tmp];
	}

    float bar_size = (float)(size.ws_col - 35);
    if (bar_size < 50) {
        bar_size = 50.0;
    }
    printhbar(size.ws_col);
	for (i=0; i < n_bins; i++){
        printf(" %.5lf - %.5lf ( %6d )|", i * delta, (i+1) * delta, count_bins[i]);
		for (j=0; j < (int)(count_bins[i]*(bar_size/max_count)); j++){
			printf("*");
		}

        printf("\n");
	}
    printhbar(size.ws_col);
}

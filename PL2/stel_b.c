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

// Defenition of histogram structure
typedef struct{
	int n_bins;
	int *bins;
	int max_count;
} histogram;

double delta = 0;
double lambda=0, dm=0;
int K=0, buffer_size=0;

lista *new_out_event(lista* lista_eventos, double current_time, double *d, int n_bins);
void printhistogram(histogram *htg);

int main(int argc, char const *argv[]) {

    lista *lista_eventos;
    lista *buffer;
    histogram *call_times_hst = (histogram *)malloc(sizeof( histogram));

	int tipo_ev, i; double tempo_ev;
    int buffer_free = 0;
    int n_channels = 0, free_channels = 0, n_simulations = 0, event_type, n_delays = 0;
    double current_time = 0, last_event_time = 0, c, u, d, n_coming_events = 0, n_loss = 0, delays_cnt = 0;

    //  ---- Initialize events list ----
    buffer = NULL;
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

    //  ---- Ask and set lambda ----
    printf("Lambda (calls/hour): ");
    scanf("%lf", &lambda);
    printf("\n");
    lambda = lambda/3600;

    //  ---- Ask and set precess time ----
    printf("Processing times (minutes): ");
    scanf("%lf", &dm);
    printf("\n");
    dm *= 60;

    //  ---------------------------------------------------
    //  ---- Calculate number of bins to the histogram ----
    //  ---------------------------------------------------
    delta = (double)(dm)/5;
    double tmp_n_bins = dm * 10 / delta;

    if (tmp_n_bins - (int) tmp_n_bins > 0) {
        tmp_n_bins += 1;
    }
    call_times_hst->n_bins = (int)tmp_n_bins;

    call_times_hst->bins = (int *)malloc(sizeof(int) * call_times_hst->n_bins);
    call_times_hst->max_count = 0;
    for (i=0; i < call_times_hst->n_bins; i++) (call_times_hst->bins)[i] = 0;

    //  ---- Ask and set clients ----
    printf("N clients: ");
    scanf("%d", &K);
    printf("\n");

    //  ---- Ask and set number of buffers ----
    printf("Buffer size: ");
    scanf("%d", &buffer_size);
    printf("\n");
    buffer_free = buffer_size;

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
            int active_clients = K - (buffer_size - buffer_free) - (n_channels - free_channels);
            if (active_clients < 0) {
                active_clients = 0;
            }
            c = -log(u) / (lambda*(active_clients));

            n_coming_events++;
            if (free_channels > 0) {

                lista_eventos = new_out_event(lista_eventos, current_time, &d, call_times_hst->n_bins);
                free_channels--;

                //  ---- Calculate average service time ----
                avg_d = ((avg_d*i) + d) / (i+1);

                //  ---- Increment bins on histogram array ----
                int tmp;
                tmp = (int)(d / delta);

                (call_times_hst->bins)[tmp]++;
                if ((call_times_hst->bins)[tmp] > call_times_hst->max_count)
                    call_times_hst->max_count = (call_times_hst->bins)[tmp];

            } else {
                if (buffer_free > 0) {
                    buffer = adicionar(buffer, event_type, current_time);
                    n_delays++;
                    buffer_free--;
                } else {
                    n_loss++;
                }
            }

            lista_eventos = adicionar(lista_eventos, 0, current_time + c);
        } else {
            if (buffer_free < buffer_size) {
                delays_cnt += current_time - buffer->tempo;
                buffer = remover(buffer);
                lista_eventos = new_out_event(lista_eventos, current_time, &d, call_times_hst->n_bins);

                //  ---- Calculate average service time ----
                avg_d = ((avg_d*i) + d) / (i+1);

                //  ---- Increment bins on histogram array ----
                int tmp;
                tmp = (int)(d / delta);

                (call_times_hst->bins)[tmp]++;
                if ((call_times_hst->bins)[tmp] > call_times_hst->max_count)
                    call_times_hst->max_count = (call_times_hst->bins)[tmp];

                buffer_free++;
            } else {
                free_channels++;
            }
        }

        // imprimir(lista_eventos);
        // printf("Free channels: %d\n", free_channels);
    }

    printf("\nAvg service time: %lf\n", avg_d);
    printf("Probability of blocking (B): %.3lf%%\n", ((double)n_loss/n_coming_events)*100);
    printf("Probability of customer delay: %.3lf%%\n", ((double)n_delays/n_coming_events)*100);
    printf("Average of customer service delay: %.3lf\n", delays_cnt/n_coming_events);

    printhistogram(call_times_hst);

    return 0;
}

lista *new_out_event(lista* lista_eventos, double current_time, double *d, int n_bins) {
    double u;
    do {
        u = (double)((unsigned)rand() + 1U)/((unsigned)RAND_MAX + 1U);
        *d = -dm * log(u);
    } while((int)(*d / delta) > n_bins-1);

    // ---- Add new out event to the list of events ----
    return adicionar(lista_eventos, 1, current_time + *d);
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
void printhistogram(histogram *htg){
	int	i, j;
    double val;

    struct winsize size;
    ioctl(STDOUT_FILENO,TIOCGWINSZ,&size);

    float bar_size = (float)(size.ws_col - 30);
    if (bar_size < 50) {
        bar_size = 50.0;
    }

    printhbar(size.ws_col);
	for (i=0; i < htg->n_bins; i++){
        printf("%6.1lf - %6.1lf ( %6d )|", i * delta, (i+1) * delta, (htg->bins)[i]);
		for (j=0; j < (int)((htg->bins)[i]*(bar_size/htg->max_count)); j++){
			printf("*");
		}

        printf("\n");
	}
    printhbar(size.ws_col);
}

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

// event types
#define PC_IN 0
#define INEM_IN 1
#define PC_OUT 2
#define INEM_OUT 3

// distribution exponential
#define lambda 600.0/3600
#define dm 90.0

// distribution gaussian
#define mu 45.0
#define sigma 15.0


// Defenition of histogram structure
typedef struct{
	int n_bins;
	int *bins;
	int max_count;
	double delta;
	int offset;
} histogram;

int K=0, buffer_size=0;
double avg_d=0;
double INEM_duration_sum = 0, delays_sum = 0;
int n_INEM_IN = 0;
int n_channels_PC = 0, free_channels_PC = 0, n_channels_INEM = 0, free_channels_INEM = 0;
double estimated_time = 0;
int n_estimated_time = 0;

void update_histograms(double current_time, lista* buffer_PC, histogram* prevision_error, histogram* delay_distribution);
lista *terminate_PC_call(lista* lista_eventos, double current_time, int iteration, double* d);
lista *new_out_event(lista* lista_eventos, int event_type, double current_time, double *d);
void printhistogram(histogram *htg);

int main(int argc, char const *argv[]) {

    lista *lista_eventos;
    lista *buffer_PC, *buffer_INEM;
	lista *buffer_PC_prv;
    histogram *delay_distribution = (histogram *)malloc(sizeof(histogram));
	histogram *prevision_error = (histogram *)malloc(sizeof(histogram));

	int tipo_ev, i; double tempo_ev;
    int buffer_free = 0, buffer_INEM_used = 0;
	int n_simulations = 0, event_type, new_event_type;
	int n_delays_PC = 0, n_delays_INEM;
	int n_loss = 0, n_coming_events = 0;
    double current_time = 0, last_event_time = 0;
	double c, u, d;
	int id_buffer_INEM = 0;

    //  ---- Initialize events list ----
	buffer_PC = NULL;
	buffer_INEM = NULL;
	lista_eventos = NULL;
    lista_eventos = adicionar(lista_eventos, PC_IN, 0, 0);

	//	---- Initialize delay distribution histogram ----
	delay_distribution->delta = dm/20;
	delay_distribution->n_bins = 60*2/delay_distribution->delta;
	delay_distribution->bins = (int *)malloc(sizeof(int) * delay_distribution->n_bins);
	for(i=0; i<delay_distribution->n_bins; i++) (delay_distribution->bins)[i] = 0;
	delay_distribution->max_count = 0;
	delay_distribution->offset = 0;

	prevision_error->delta = sigma/10;
	prevision_error->n_bins = 45*2/prevision_error->delta;
	prevision_error->bins = (int *)malloc(sizeof(int) * prevision_error->n_bins);
	for(i=0; i<prevision_error->n_bins; i++) (prevision_error->bins)[i] = 0;
	prevision_error->max_count = 0;
	prevision_error->offset = -prevision_error->n_bins/2;

    /* Intializes random number generator */
    time_t t;
    srand((unsigned) time(&t));

    //  ---- Ask and set number of simulations ----
    printf("N simulations (thousands): ");
    scanf("%d", &n_simulations);
    printf("\n");
    n_simulations *= 1000;

    //  ---- Ask and set number of channels PC ----
    printf("N channels PC: ");
    scanf("%d", &n_channels_PC);
    printf("\n");
    free_channels_PC = n_channels_PC;

	//  ---- Ask and set number of channels INEM----
    printf("N channels INEM: ");
    scanf("%d", &n_channels_INEM);
    printf("\n");
    free_channels_INEM = n_channels_INEM;

    //  ---- Ask and set number of buffers ----
    printf("Buffer size: ");
    scanf("%d", &buffer_size);
    printf("\n");
    buffer_free = buffer_size;

    //  ---- Starting simulations ---
    for (i = 0; i < n_simulations; i++) {

        //  ---- Update times ---
        current_time = lista_eventos->tempo;
        event_type = lista_eventos->tipo;

		// printf("\nCurrent time: %lf\n", current_time);
		// printf("Event type: %d\n", event_type);

        //  ---- Consume event on the list ----
        lista_eventos = remover(lista_eventos);

		// events IN

		switch (event_type){
			// Handle event PC_OUT
            case PC_IN:

				// printf("New event type: %d\n", new_event_type);
				if (free_channels_PC > 0) {

					lista_eventos = terminate_PC_call(lista_eventos, current_time, i, &d);

					free_channels_PC--;

				} else {
					if (buffer_free > 0) {
						buffer_PC = adicionar(buffer_PC, event_type, current_time, estimated_time);
						n_delays_PC++;
						buffer_free--;
					} else {
						n_loss++;
					}
				}

				//  ---- Random generate time for the next call ----
				u = rand() * (1.0 / RAND_MAX);
				c = (double) -log(u) / (lambda * 1.0);

				// printf("lambda: %lf, -log(u): %lf| c: %lf\n", lambda, -log(u), c);

				n_coming_events++;
				lista_eventos = adicionar(lista_eventos, PC_IN, current_time + c, 0);

            	break;

			// Handle event INEM_IN
            case INEM_IN:

				if (free_channels_INEM > 0) {
					lista_eventos = new_out_event(lista_eventos, INEM_OUT, current_time, &d);
					// printf("INEM_OUT duration (time=%lf): %lf\n", current_time, d);
					free_channels_INEM--;
					if (buffer_free < buffer_size) {

						update_histograms(current_time, buffer_PC, prevision_error, delay_distribution);

						buffer_PC = remover(buffer_PC);

						lista_eventos = terminate_PC_call(lista_eventos, current_time, i, &d);

						buffer_free++;
					} else {
						free_channels_PC++;
					}
				} else {
					buffer_INEM = adicionar(buffer_INEM, event_type, current_time, id_buffer_INEM++);
					// printf("INEM_OUT duration (time=%lf)\n", current_time);
					// printf("Current time (in buffer %d): %lf\n", id_buffer_INEM, current_time);
					buffer_INEM_used++;
					n_delays_INEM++;
				}

            	break;

			// Handle event PC_OUT
			case PC_OUT:
				if (buffer_free < buffer_size) {

					update_histograms(current_time, buffer_PC, prevision_error, delay_distribution);

					buffer_PC = remover(buffer_PC);

					lista_eventos = terminate_PC_call(lista_eventos, current_time, i, &d);

					buffer_free++;
				} else {
					free_channels_PC++;
				}
				break;

			case INEM_OUT:
				if (buffer_INEM_used > 0) {
					INEM_duration_sum += current_time - buffer_INEM->tempo;
					// printf("Current time (out buffer %d): %lf\n", current_time, (int)buffer_INEM->tempo_previsto);
					// printf("time to INEM_IN (buffer): %lf\n", current_time - buffer_INEM->tempo);
					buffer_INEM = remover(buffer_INEM);
					lista_eventos = new_out_event(lista_eventos, INEM_OUT, current_time, &d);
					buffer_INEM_used--;
					if (buffer_free < buffer_size) {

						update_histograms(current_time, buffer_PC, prevision_error, delay_distribution);

						buffer_PC = remover(buffer_PC);

						lista_eventos = terminate_PC_call(lista_eventos, current_time, i, &d);

						buffer_free++;
					} else {
						free_channels_PC++;
					}
				} else {
					free_channels_INEM++;
				}
				break;

        }
		// printf("\nLista de eventos: \n");
        // imprimir(lista_eventos);
		// printf("%lf\n", current_time);
		// printf("Buffer INEM: \n");
        // imprimir(buffer_INEM);
        // printf("Free channels: %d (PC) | %d (INEM)\n", free_channels_PC, free_channels_INEM);
    }

	printf("Proteção Civil:\n");
	printf("\tProbabilidade de chamada ser atrasada: %.3lf (n=%d)\n", (double) n_delays_PC/n_coming_events, n_delays_PC);
	printf("\tProbabilidade de chamada ser perdida: %.3lf (n=%d)\n", (double) n_loss/n_coming_events, n_loss);
	printf("\tTempo médio de atraso: %.3lf\n", delays_sum/n_delays_PC);
	printf("\tTempo médio de serviço: %.3lf\n", avg_d);
	printhistogram(delay_distribution);
	// printhistogram(prevision_error);

	printf("\nINEM:\n");
	printf("\tAverage delay between coming call to PC and the answer by INEM: %lf\n", INEM_duration_sum/n_INEM_IN);

    return 0;
}

void update_histograms(double current_time, lista* buffer_PC, histogram* prevision_error, histogram* delay_distribution) {
	double delay = (current_time - buffer_PC->tempo);
	// printf("delay: %lf | current_time: %lf | buffer time: %lf\n", delay, current_time, buffer_PC->tempo);
	double estimated_error = buffer_PC->tempo_previsto - delay;
	int tmp_error = (int)(estimated_error / prevision_error->delta) - prevision_error->offset;
	// printf("tmp_error: %d\n", tmp_error);
	if (tmp_error >= 0 && tmp_error < prevision_error->n_bins) {
		if (prevision_error->max_count < ++(prevision_error->bins)[tmp_error]) prevision_error->max_count = (prevision_error->bins)[tmp_error];
		// printf("estimated_error: %lf | prevision_error[%d] = %d | max_count = %d\n", estimated_error, tmp_error, (prevision_error->bins)[tmp_error], prevision_error->max_count);
		// if (prevision_error->max_count > n_simulations) {
		// 	return;
		// }
	}

	if (n_estimated_time == 100) {
		estimated_time = estimated_time*0.99 + delay*0.01;
		int tmp = (int)(delay / delay_distribution->delta);
		// printf("tmp: %d\n", tmp);
		if (tmp >= 0 && tmp < delay_distribution->n_bins) {
			if (delay_distribution->max_count < ++(delay_distribution->bins)[tmp]) delay_distribution->max_count = (delay_distribution->bins)[tmp];
		}
	} else {
		estimated_time = (estimated_time * (n_estimated_time++) + delay) / n_estimated_time;
	}
	delays_sum += delay;
}

lista *terminate_PC_call(lista* lista_eventos, double current_time, int iteration, double* d) {
	int new_event_type = (rand() % 10) < 4 ? PC_OUT : INEM_IN;

	// Genrating new event
	switch (new_event_type) {

		// new event PC_OUT
		case PC_OUT:

			lista_eventos = new_out_event(lista_eventos, new_event_type, current_time, d);

			//  ---- Calculate average service time ----
			avg_d = ((avg_d*iteration) + *d) / (iteration+1);
			break;

		// new event INEM_IN
		case INEM_IN:

			lista_eventos = new_out_event(lista_eventos, new_event_type, current_time, d);

			n_INEM_IN++;
			// printf("time to INEM_IN: %lf\nn_INEM_IN: %d\n", d, n_INEM_IN);
			INEM_duration_sum += *d;
			break;
	}

	return lista_eventos;
}

lista *new_out_event(lista* lista_eventos, int event_type, double current_time, double *d) {

	static const double two_pi = 2.0*3.14159265358979323846;

    double u1, u2;
	while (1) {

		u1 = rand() * (1.0 / RAND_MAX);
		u2 = rand() * (1.0 / RAND_MAX);

		if (event_type == PC_OUT || event_type == INEM_OUT) {

			*d = -log(u1) * dm + 60;
			if(event_type == PC_OUT && *d > 4*60) continue;

			break;

		} else if (event_type == INEM_IN) {

			double z0, z1;
			z0 = sqrt(-2.0 * log(u1)) * cos(two_pi * u2);
			z1 = sqrt(-2.0 * log(u1)) * sin(two_pi * u2);
			*d = z0 * sigma + mu;

			if (*d < 30 || *d > 75) continue;

			break;
		}
	}

	// printf("d (type %d) %lf\n", event_type, *d);

    // ---- Add new out event to the list of events ----
    return adicionar(lista_eventos, event_type, current_time + *d, 0);
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
        printf("%6.1lf - %6.1lf ( %6d )|", (i + htg->offset) * htg->delta, (i+1 + htg->offset) * htg->delta, (htg->bins)[i]);
		for (j=0; j < (int)((htg->bins)[i]*(bar_size/htg->max_count)); j++){
			printf("*");
		}

        printf("\n");
	}
    printhbar(size.ws_col);
}

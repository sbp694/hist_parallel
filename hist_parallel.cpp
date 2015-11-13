#define NDEBUG
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cassert>
#include "Timer.h"
#include <pthread.h>
#include "assert.h"

extern "C" {
#include "ppmb_io.h"
}

int *hist_r, *hist_g, *hist_b;
pthread_mutex_t mutexsum;

struct thread_data{
  struct img *input;
  int *histr;
  int *histg;
  int *histb;
  int pix_to_read;
  int index;
};

struct img {
  int xsize;
  int ysize;
  int maxrgb;
  unsigned char *r;
  unsigned char *g;
  unsigned char *b;
};

void print_histogram(FILE *f, int *hist, int N) {
  fprintf(f, "%d\n", N+1);
  for(int i = 0; i <= N; i++) {
    fprintf(f, "%d %d\n", i, hist[i]);
  }
  printf("finished print\n");
}

void *histogram(void *data) {
  // we assume hist_r, hist_g, hist_b are zeroed on entry.
  struct thread_data *my_data = (struct thread_data *) data;
  for(int pix = my_data->index; pix < (my_data->index + my_data->pix_to_read); pix++) {
    assert(pix < my_data->input->xsize * my_data->input->ysize);
    my_data->histr[my_data->input->r[pix]] += 1;
    my_data->histg[my_data->input->g[pix]] += 1;
    my_data->histb[my_data->input->b[pix]] += 1;
  }
  //add values to master histogram
  pthread_mutex_lock (&mutexsum);
  int a;
  for (a = 0; a < my_data->input->maxrgb; a ++){
    hist_r[a] += my_data->histr[a];
    hist_g[a] += my_data->histg[a];
    hist_b[a] += my_data->histb[a];
  }
  pthread_mutex_unlock (&mutexsum);

  pthread_exit(0);
}

char *get_output_file(const char *input) {
  char *out;

  out = (char *) malloc(strlen(input) + strlen(".hist") + 1);
  if(!out) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(1);
  }

  if(sprintf(out, "%s.hist", input) < 0) {
    fprintf(stderr, "sprintf error\n");
    exit(1);
  }
  
  return out;
}

int main(int argc, char *argv[]) {
  if(argc != 3) {
    printf("Usage: %s input-file num-of-threads-to-use\n", argv[0]);
    exit(1);
  }
  
  struct img input;
  if (argv[2] <= 0){
    printf("Number of threads must be an integer greater than zero\n");
    exit(1);
  } 
  int num_threads = atoi(argv[2]);
  
  pthread_t threads[num_threads];

  if(!ppmb_read(argv[1], &input.xsize, &input.ysize, &input.maxrgb, 
		&input.r, &input.g, &input.b)) {
    if(input.maxrgb > 255) {
      printf("Maxrgb %d not supported\n", input.maxrgb);
      exit(1);
    }

    int num_pix = (input.xsize * input.ysize);
    struct thread_data *td[num_threads];


    hist_r = (int *) calloc(input.maxrgb+1, sizeof(int));
    hist_g = (int *) calloc(input.maxrgb+1, sizeof(int));
    hist_b = (int *) calloc(input.maxrgb+1, sizeof(int));

    ggc::Timer t("histogram");
    int a;
    int b;
    int remainder = num_pix % num_threads;
    int portion = num_pix / num_threads;
    t.start();
    for (a = 0; a < num_threads; a++){
      struct thread_data *new_thread = (struct thread_data *) malloc(sizeof(struct thread_data));
      new_thread->input = &input;
      new_thread->histr = (int *) calloc(input.maxrgb+1, sizeof(int));
      new_thread->histb = (int *) calloc(input.maxrgb+1, sizeof(int));
      new_thread->histg = (int *) calloc(input.maxrgb+1, sizeof(int));
      if (a == num_threads - 1){
        new_thread->pix_to_read = portion + remainder;
      } else {
        new_thread->pix_to_read = portion;
      }
      new_thread->index =  portion * a;
      td[a] = new_thread;
      pthread_create(&threads[a], NULL, histogram, (void *)td[a]);
    }
    void **return_value;
    for (b = 0; b < num_threads; b++){
      pthread_join(threads[b], return_value);
    }

    t.stop();

    char *output = get_output_file(argv[1]);

    FILE *out = fopen(output, "w");
    if(out) {
      print_histogram(out, hist_r, input.maxrgb);
      print_histogram(out, hist_g, input.maxrgb);
      print_histogram(out, hist_b, input.maxrgb);
      fclose(out);
    } else {
      fprintf(stderr, "Unable to output!\n");
    }
    printf("Time: %llu ns\n", t.duration());
  }  
}

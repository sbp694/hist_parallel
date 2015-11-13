#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cassert>
#include "Timer.h"
#include <pthread.h>

extern "C" {
#include "ppmb_io.h"
}

struct thread_data{
  int portion;
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

struct img input, output;
int *xlat_r, *xlat_g, *xlat_b;
pthread_mutex_t xlat_lockr;
pthread_mutex_t xlat_lockb;
pthread_mutex_t xlat_lockg;


int alloc_img(struct img *p) {
  p->r = (unsigned char *) malloc(p->xsize * p->ysize * sizeof(char));
  p->g = (unsigned char *) malloc(p->xsize * p->ysize * sizeof(char));
  p->b = (unsigned char *) malloc(p->xsize * p->ysize * sizeof(char));

  if(!p->r  || !p->g || !p->b) {
    fprintf(stderr, "Failed to allocate memory for image\n");
    return 0;
  }

  return 1;
}

void read_histogram(FILE *f, int **hist, int *N) 
{
  fprintf(stderr,"start of read_histogram\n");
  int j;

  if(fscanf(f, "%d", N) != 1) {
    fprintf(stderr, "Unable to read length of histogram\n");
    exit(1);    
  };

  fprintf(stderr, "Allocating histogram for %d entries\n", *N);
  *hist = (int *) calloc(*N, sizeof(int));

  for(int i = 0; i < *N; i++) {
    fscanf(f, "%d %d", &j, *hist + i);
    if(i != j) {
      fprintf(stderr, "Histogram indices corrupt!\n");
      exit(1);
    }
  }
      printf("end of read_histogram\n");

}

void gen_xlat(int pixels, int N, int *hist, int **xlat)
{
  *xlat = (int *) calloc(N, sizeof(int));
  if(!*xlat) {
    fprintf(stderr, "Unable to allocate memory for xlat\n");
    exit(1);
  }

  int *xl = *xlat;
  int cdf_min = 0;

  // first calculate cdf using xl as a temporary space
  xl[0] = hist[0];
  for(int i = 1; i < N; i++) {
    xl[i] = hist[i] + xl[i - 1];
  }

  // then calculate cdf_min
  for(int i = 0; i < N; i++) {
    cdf_min = xl[i];
    if(cdf_min > 0)
      break;
  }

  // then calculate xlat
  xl[0] = hist[0];
  for(int i = 1; i < N; i++) {
    xl[i] = ((xl[i] - cdf_min) * (N - 1)) / (pixels - cdf_min);

    //xl[i] = ((float) (xl[i] - cdf_min) / (pixels - cdf_min)) * (N - 1);

    if(xl[i] > 255) xl[i] = 255;
    if(xl[i] < 0) xl[i] = 0;					     
  } 
}

void *equalize(void *thread_data) {
  struct thread_data *my_data = (struct thread_data *) thread_data;
  
  fprintf(stderr,"my_data info: index = %d and portion = %d\n", my_data->index, my_data->portion);
  for(int pix = my_data->index; pix < (my_data->index + my_data->portion); pix++) {
    assert(pix < input.xsize * input.ysize);
    pthread_mutex_lock(&xlat_lockr);
    output.r[pix] = xlat_r[input.r[pix]];
    pthread_mutex_unlock(&xlat_lockr);

    pthread_mutex_lock(&xlat_lockg);
    output.g[pix] = xlat_g[input.g[pix]];
    pthread_mutex_unlock(&xlat_lockg);
    
    pthread_mutex_lock(&xlat_lockb);
    output.b[pix] = xlat_b[input.b[pix]];
    pthread_mutex_unlock(&xlat_lockb);
  }
  fprintf(stderr,"Thread index %d finished\n", my_data->index);
  pthread_exit(0);
}

int main(int argc, char *argv[]) {
  if(argc != 5) {
    printf("Usage: %s input-file histogram output-file num-of-threads-to-use\n", argv[0]);
    exit(1);
  }
  if (argv[4] <= 0){
    printf("Number of threads must be larger than zero.\n");
    exit(1);
  }
  int num_threads = atoi(argv[4]);
  pthread_t threads[num_threads];
  struct thread_data *td[num_threads];
  if(!ppmb_read(argv[1], &input.xsize, &input.ysize, &input.maxrgb, 
		&input.r, &input.g, &input.b)) {
    if(input.maxrgb > 255) {
      printf("Maxrgb %d not supported\n", input.maxrgb);
      exit(1);
    }

    int *hist_r, *hist_g, *hist_b;
    int N;
    FILE *hist;
    fprintf(stderr,"call file open\n");
    hist = fopen(argv[2], "r");
    if(!hist) {
      fprintf(stderr, "Unable to read histogram file '%s'\n", argv[2]);
      exit(1);
    }
    read_histogram(hist, &hist_r, &N);
    if(N != input.maxrgb+1) { fprintf(stderr, "maxrgb != red histogram length\n"); exit(1); }
    read_histogram(hist, &hist_g, &N);
    if(N != input.maxrgb+1) { fprintf(stderr, "maxrgb != green histogram length\n"); exit(1); }
    read_histogram(hist, &hist_b, &N);
    if(N != input.maxrgb+1) { fprintf(stderr, "maxrgb != blue histogram length\n"); exit(1); }

    gen_xlat(input.xsize * input.ysize, N, hist_r, &xlat_r);
    gen_xlat(input.xsize * input.ysize, N, hist_g, &xlat_g);
    gen_xlat(input.xsize * input.ysize, N, hist_b, &xlat_b);
    
    ggc::Timer t("equalize\n");

    void **return_value;
    int img_size = input.xsize * input.ysize;
    int portion = img_size / num_threads;
    int remainder = img_size % num_threads;
    t.start();
    struct img* output_p = &output;
    output.xsize = input.xsize;
    output.ysize = input.ysize;
    alloc_img(output_p);
    for (int a = 0; a < num_threads; a++){
      struct thread_data * data = (struct thread_data *) malloc(sizeof(struct thread_data));
      if (a == num_threads - 1){
        data->portion = portion + remainder;
      } else {
        data->portion = portion;
      }
      data->index = portion * a;
      td[a] = data;
      pthread_create(&threads[a], NULL, equalize, (void *)td[a]);
    }
    for (int b = 0; b < num_threads; b++){
      pthread_join(threads[b], return_value);
    }
    t.stop();


    fprintf(stderr, "threads have been reaped\n");
    if(ppmb_write(argv[3], output.xsize, output.ysize, output.r, output.g, output.b)) {
      fprintf(stderr, "Unable to write output\n");
      exit(1);
    }
    printf("Time: %llu ns\n", t.duration());
  }  
}

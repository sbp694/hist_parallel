#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cassert>
#include "Timer.h"

extern "C" {
#include "ppmb_io.h"
}

struct img {
  int xsize;
  int ysize;
  int maxrgb;
  unsigned char *r;
  unsigned char *g;
  unsigned char *b;
};

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

void equalize(struct img *in, struct img *out, int *xlat_r, int *xlat_g, int *xlat_b) {
  out->xsize = in->xsize;
  out->ysize = in->ysize;
  alloc_img(out);

  for(int pix = 0; pix < in->xsize * in->ysize; pix++) {
    out->r[pix] = xlat_r[in->r[pix]];
    out->g[pix] = xlat_g[in->g[pix]];
    out->b[pix] = xlat_b[in->b[pix]];
  }
}

int main(int argc, char *argv[]) {
  if(argc != 4) {
    printf("Usage: %s input-file histogram output-file\n", argv[0]);
    exit(1);
  }
  
  struct img input, output;

  if(!ppmb_read(argv[1], &input.xsize, &input.ysize, &input.maxrgb, 
		&input.r, &input.g, &input.b)) {
    if(input.maxrgb > 255) {
      printf("Maxrgb %d not supported\n", input.maxrgb);
      exit(1);
    }

    int *hist_r, *hist_g, *hist_b;
    int *xlat_r, *xlat_g, *xlat_b;
    int N;
    FILE *hist;

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
    
    ggc::Timer t("equalize");
     
    t.start();
    equalize(&input, &output, xlat_r, xlat_g, xlat_b);
    t.stop();

    if(ppmb_write(argv[3], output.xsize, output.ysize, output.r, output.g, output.b)) {
      fprintf(stderr, "Unable to write output\n");
      exit(1);
    }
    printf("Time: %llu ns\n", t.duration());
  }  
}

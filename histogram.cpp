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

void print_histogram(FILE *f, int *hist, int N) {
  fprintf(f, "%d\n", N+1);
  for(int i = 0; i <= N; i++) {
    fprintf(f, "%d %d\n", i, hist[i]);
  }
}

void histogram(struct img *input, int *hist_r, int *hist_g, int *hist_b) {
  // we assume hist_r, hist_g, hist_b are zeroed on entry.

  for(int pix = 0; pix < input->xsize * input->ysize; pix++) {
    hist_r[input->r[pix]] += 1;
    hist_g[input->g[pix]] += 1;
    hist_b[input->b[pix]] += 1;
  }
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
  if(argc != 2) {
    printf("Usage: %s input-file\n", argv[0]);
    exit(1);
  }
  
  struct img input;

  if(!ppmb_read(argv[1], &input.xsize, &input.ysize, &input.maxrgb, 
		&input.r, &input.g, &input.b)) {
    if(input.maxrgb > 255) {
      printf("Maxrgb %d not supported\n", input.maxrgb);
      exit(1);
    }

    int *hist_r, *hist_g, *hist_b;

    hist_r = (int *) calloc(input.maxrgb+1, sizeof(int));
    hist_g = (int *) calloc(input.maxrgb+1, sizeof(int));
    hist_b = (int *) calloc(input.maxrgb+1, sizeof(int));

    ggc::Timer t("histogram");

    t.start();
    histogram(&input, hist_r, hist_g, hist_b);
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

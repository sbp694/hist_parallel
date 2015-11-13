all: histogram equalize hist_parallel equalize_parallel

histogram: histogram.cpp ppmb_io.a
	gcc -O3 $^ -o $@ -lm -lrt

equalize: equalize.cpp ppmb_io.a
	gcc -O3 $^ -o $@ -lm -lrt

ppmb_io.a: ppmb_io.o 
	ar rs $@ $<

hist_parallel: hist_parallel.cpp ppmb_io.a
	gcc -O3 $^ -o $@ -lm -lrt -lpthread

equalize_parallel: equalize_parallel.cpp ppmb_io.a
	gcc -O3 $^ -o $@ -lm -lrt -lpthread


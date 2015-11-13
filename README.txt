To compile:

  make

To run:

  ./histogram file.ppm

  ./equalize file.ppm file.ppm.hist outfile.ppm

You can use any image viewer (like eog) to view the ppm files.

Examples:

  ./histogram moon-small.ppm

  ./equalize moon-small.ppm moon-small.ppm.hist moon-small-equal.ppm

Note, after parallelization, you'll need to support a command line like below:

  ./histogram file.ppm 4

  ./equalize file.ppm file.ppm.hist outfile.ppm 4

where 4 is the number of threads to use.


ACKNOWLEDGEMENTS

moon-small.ppm, phobos.ppm, moon-large.ppm and earth.ppm are courtesy
NASA, the latter two from the Project Apollo Archive.

so.ppm is from a StackOverflow question
(https://dsp.stackexchange.com/questions/3372/relation-between-histogram-equalization-and-auto-levels),
hopefully as fair use.

PPMB_IO is (c) John Burkardt, and is used under the LGPL.

300px-Unequalized_Hawkes_Bay_NZ.ppm is from Wikipedia.

flood.ppm is from University of Regina.



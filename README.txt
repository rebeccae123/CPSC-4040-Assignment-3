Rebecca Edson
CPSC 4040 Assignment 3
"Green Screening"
October 6, 2019

Compile both programs with Makefile (type "make" in command line).

Run alphamask:
  ./alphamask <input_filename.img> <output_filename>.png

Run compose:
  ./compose <imageA_filename>.png <imageB_filename.img>
  ./compose <imageA_filename>.png <imageB_filename.img> <output_filename>.png


alphamask reads in an image, removes the green background, and writes new image
to file.

compose reads in a foreground and background image, composites the foreground
over the background, and displays image in window; also writes image to file
if output file was specified in command line.

Known problems:
compose - when closing image window, program ends with segmentation fault, but
correct image displays in window and writes to file if specified

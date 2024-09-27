//
//                            makedatafile
//
//           Author: Noah Mendelsohn
//
//     A simple program to fill a file with numbers
//

#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdlib.h>

using namespace std;

int
main(int argc, char *argv[]) {

  int linesToWrite;
  int lineNumber;
  int numberNumber;
  int outputNumber =0;
  char *filename;
  int NUMBERSPERLINE=10;

  if (argc != 3) {
    fprintf(stderr,"Correct syntax is %s <filename> <linesToWrite>\n", argv[0]);
    exit (4);
  }

  filename = argv[1];
  linesToWrite = atoi(argv[2]);

  if (linesToWrite <= 0) {
    fprintf(stderr,"Correct syntax is %s <filename> <linesToWrite>\n", argv[0]);    
    exit (4);
  }

  printf("Writing %d lines to file %s\n", linesToWrite, filename);

  ofstream of(filename);

  outputNumber = 0;

  for (lineNumber = 0; lineNumber < linesToWrite; lineNumber++) {
    for (numberNumber = 0; numberNumber< NUMBERSPERLINE; numberNumber++) {
      of << setw(6) << (outputNumber++) << ' '; 
    }
    of << endl;
  }

  of.close();

  printf("Wrote %d lines to file %s\n", linesToWrite, filename);

  
}

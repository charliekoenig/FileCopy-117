// 
//            sha1tstFIXED
//
//     Author: Noah Mendelsohn
//
//     Test programming showing use of computation of 
//     sha1 hash function.
//
//     NOTE: problems were discovered using the incremental
//     version of the computation with SHA1_Update. This 
//     version, which computes the entire checksum at once,
//     seems to be reliable (if less flexible).
//
//     Note: this must be linked with the g++ -lssl directive. 
//


#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <openssl/sha.h>

using namespace std;

int main(int argc, char *argv[])
{
  int i, j;
  ifstream *t;
  stringstream *buffer;

  unsigned char obuf[20];

  if (argc < 2) 	{
		fprintf (stderr, "usage: %s file [file...]\n", argv[0]);
		exit (1);
  }

  for (j = 1; j < argc; ++j) {
		printf ("SHA1 (\"%s\") = ", argv[j]);
		t = new ifstream(argv[j]);
		buffer = new stringstream;
		*buffer << t->rdbuf();
		SHA1((const unsigned char *)buffer->str().c_str(), 
		     (buffer->str()).length(), obuf);
		for (i = 0; i < 20; i++)
		{
			printf ("%02x", (unsigned int) obuf[i]);
		}
		printf ("\n");
		delete t;
		delete buffer;
  }
  return 0;
}

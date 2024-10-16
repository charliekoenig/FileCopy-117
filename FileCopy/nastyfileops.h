#include <fstream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include "c150nastyfile.h"

using namespace C150NETWORK;

string makeFileName(string dir, string fname);
bool isFile(string fname);

bool safeFRead(unsigned int bytesToRead, NASTYFILE &inputFile, int size, 
               unsigned char **data, int offset);

bool safeFWrite(unsigned int bytesToWrite, NASTYFILE &outputFile, int size, 
                unsigned char *data, int offset, string targetName);

ssize_t readFile(string dir, string fname, int nastiness, unsigned char **obuff);

/**********************************************************
               nastyfileops.h - 10/17/2024
    Authors:
        * Charlie Koenig
        * Idil Kolabas

    This module is an the interfact for the nastyfileops
    operations which support the use of C150NASTYFILE 
    including filename assembly, file format checking, 
    safe reading, and safe writing
    
***********************************************************/

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
               unsigned char *data, int offset);

bool safeFWrite(unsigned int bytesToWrite, NASTYFILE &outputFile, int size, 
                unsigned char *data, int offset, string targetName,
                unsigned char *memFile);

ssize_t readFile(string dir, string fname, int nastiness, unsigned char **obuff);

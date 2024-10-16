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
int safeFRead(unsigned int bytesToSend, NASTYFILE &inputFile, int size, unsigned char **fileContent, int offset);
ssize_t readFile(string dir, string fname, int nastiness, unsigned char **obuff);

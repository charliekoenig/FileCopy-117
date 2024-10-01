#include "nastyfileops.h"

using namespace std;
using namespace C150NETWORK;

string
makeFileName(string dir, string fname) {
    stringstream ss;

    ss << dir;
    // make sure dir name ends in /
    if (dir.substr(dir.length()-1,1) != "/")
        ss << '/';
    ss << fname;     // append file name to dir
    return ss.str();  // return dir/name
  
}

bool
isFile(string fname) {
    const char *filename = fname.c_str();
    struct stat statbuf;  
    if (lstat(filename, &statbuf) != 0) {
        fprintf(stderr,"isFile: Error stating supplied source file %s\n", filename);
        return false;
    }

    if (!S_ISREG(statbuf.st_mode)) {
        fprintf(stderr,"isFile: %s exists but is not a regular file\n", filename);
        return false;
    }
    return true;
}

ssize_t
readFile(string dir, string fname, int nastiness, unsigned char **obuff) {
    struct stat statbuff;
    void *fopenretval;
    size_t length;
    size_t sourceSize;

    string fullFilePath = makeFileName(dir, fname);

    if (!isFile(fullFilePath)) {
        cerr << "Input file " << fullFilePath << " is a directory or other non-regular file. Skipping" << endl;
        return -1; 
    }

    try {

        if (lstat(fullFilePath.c_str(), &statbuff) != 0) {
            fprintf(stderr,"copyFile: Error stating supplied source file %s\n", fullFilePath.c_str());
            exit(20);
        }

        sourceSize = statbuff.st_size;
        *obuff = (unsigned char *)malloc(sourceSize);
        
        NASTYFILE inputFile(nastiness); 

        fopenretval = inputFile.fopen(fullFilePath.c_str(), "rb");  
        if (fopenretval == NULL) {
            cerr << "Error opening input file " << fullFilePath << " errno=" << strerror(errno) << endl;
            exit(12);
        }

        length = inputFile.fread(*obuff, 1, sourceSize);
        if (length != sourceSize) {
            cerr << "Error reading file " << fullFilePath << " errno=" << strerror(errno) << endl;
            exit(16);
        }

        if (inputFile.fclose() != 0 ) {
            cerr << "Error closing input file " << fullFilePath << " errno=" << strerror(errno) << endl;
            exit(16);
        }


        return length;

    } catch (C150Exception& e) {
        cerr << "endtoendserver:readfile(): Caught C150Exception: " << e.formattedExplanation() << endl;
    } 

    return -1;
}

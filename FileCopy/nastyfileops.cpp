#include "nastyfileops.h"
#include <openssl/sha.h>
#include <cstring>

using namespace std;
using namespace C150NETWORK;

/**********************************************************
 * Function: makeFileName
 
 * Parameters: 
    * string dir -> Directory name containing the file
    * string fname -> Name of the file within the directory

 * Return: A string representing a relative file path

 * written by: Noah Mendelsohn 
   from: nastyfiletest.cpp
***********************************************************/
string
makeFileName(string dir, string fname) {
    stringstream ss;
    ss << dir;

    // make sure dir name ends in /
    if (dir.substr(dir.length()-1,1) != "/")
        ss << '/';

    // append file name to dir
    ss << fname;    

    // return dir/name
    return ss.str(); 
}

/**********************************************************
 * Function: isFile

 * Parameters: 
    * string fname -> Relative path of a file

 * Return: A boolean (true if fname is a file, false if not)

 * written by: Noah Mendelsohn 
   from: nastyfiletest.cpp
***********************************************************/
bool
isFile(string fname) {
    const char *filename = fname.c_str();
    struct stat statbuf;  

    // confirm fname is a file
    if (lstat(filename, &statbuf) != 0) {
        fprintf(stderr,"isFile: Error stating supplied source file %s\n", filename);
        return false;
    }

    // confirm file is regular
    if (!S_ISREG(statbuf.st_mode)) {
        fprintf(stderr,"isFile: %s exists but is not a regular file\n", filename);
        return false;
    }
    return true;
}

bool
safeFRead(unsigned int bytesToRead, NASTYFILE &inputFile, int size, 
          unsigned char **data, int offset) {
    size_t bytesRead = 0;

    float freq = 0;
    float tries = 0;

    unordered_map<string, float> contentStrings;
    *data = (unsigned char *)malloc(bytesToRead);

    do {
        do {
            inputFile.fseek(offset, SEEK_SET);
            bytesRead = inputFile.fread(*data, size, bytesToRead);
        } while (bytesRead != bytesToRead);
        
        string contentStr((const char *) *data, bytesRead);
        tries += 1;

        int hits = contentStrings[contentStr] += 1;
        freq = hits/tries;
    } while ((freq < 0.75 || tries < 50) && tries < 200);

    return (freq >= 0.75);
}

bool
safeFWrite(unsigned int bytesToWrite, NASTYFILE &outputFile, int size, 
           unsigned char *data, int offset, string targetName) {

    int failedAttempts = 0;
    bool success       = false;

    void *filePtr = NULL;
    unsigned int bytesWritten = 0;
    int fileClosed = 1;

    unsigned char *dataRead = NULL;

    try {
        do {
            failedAttempts += 1;
            do {
                filePtr = outputFile.fopen(targetName.c_str(), "wb");
            } while (filePtr == NULL);

            do {
                outputFile.fseek(offset, SEEK_SET);
                bytesWritten = outputFile.fwrite(data, 1, bytesToWrite);
            } while (bytesWritten != bytesToWrite);

            do {
                fileClosed = outputFile.fclose(); 
            } while (fileClosed != 0);

            do {
                filePtr = outputFile.fopen(targetName.c_str(), "rb");
            } while (filePtr == NULL);
            
            safeFRead(bytesToWrite, outputFile, size, &dataRead, offset);
            success = memcmp(data, dataRead, bytesToWrite);

            do {
                fileClosed = outputFile.fclose(); 
            } while (fileClosed != 0);

        } while (!success && (failedAttempts < 10));
    } catch (C150Exception& e) {
        cerr << "nastyfiletest:copyfile(): Caught C150Exception: " << 
                e.formattedExplanation() << endl;
    }

    return failedAttempts < 10;
}

/**********************************************************
 * Function: readFile

 * Parameters: 
    * string dir -> Directory containing file
    * string fname -> File name
    * int nastiness -> File reading nastiness level
    * unsigned char **obuff -> Pointer to output buffer to hold file contents

 * Return: Length of file as ssize_t, -1 if failure

 * Notes: 
    * Allocates memory for *obuff that must be freed by caller
    * Exits program on unsuccessful file operations
***********************************************************/
ssize_t
readFile(string dir, string fname, int nastiness, unsigned char **obuff) {
    struct stat statbuff;
    void *fopenretval;
    size_t length;
    size_t sourceSize;

    // combines directory and file name for full relative file path
    string fullFilePath = makeFileName(dir, fname);

    // checks that the name given is a file
    if (!isFile(fullFilePath)) {
        cerr << "Input file " << fullFilePath << " is a directory or other non-regular file. Skipping" << endl;
        return -1; 
    }

    try {
        if (lstat(fullFilePath.c_str(), &statbuff) != 0) {
            fprintf(stderr,"copyFile: Error stating supplied source file %s\n", fullFilePath.c_str());
            exit(20);
        }

        // allocate space for *obuff to hold file contents
        sourceSize = statbuff.st_size;
        *obuff = (unsigned char *)malloc(sourceSize);
        
        // create nasty file based on nastiness given
        NASTYFILE inputFile(nastiness); 

        // open the file, exit on failure
        fopenretval = inputFile.fopen(fullFilePath.c_str(), "rb");  
        if (fopenretval == NULL) {
            cerr << "Error opening input file " << fullFilePath << " errno=" << strerror(errno) << endl;
            exit(12);
        }

        // read entire file, exit if bytes read is not equal to file length
        length = inputFile.fread(*obuff, 1, sourceSize);
        if (length != sourceSize) {
            cerr << "Error reading file " << fullFilePath << " errno=" << strerror(errno) << endl;
            exit(16);
        }

        // close input file, exit on failure
        if (inputFile.fclose() != 0 ) {
            cerr << "Error closing input file " << fullFilePath << " errno=" << strerror(errno) << endl;
            exit(16);
        }

        return length;

    } catch (C150Exception& e) {
        if (*obuff != NULL) {
            free(*obuff);
        }
        cerr << "endtoendserver:readfile(): Caught C150Exception: " << e.formattedExplanation() << endl;
    } 

    return -1;
}

/**********************************************************
               nastyfileops.cpp - 10/17/2024
    Authors:
        * Charlie Koenig
        * Idil Kolabas
        * Contributions from Noah Mendelsohn

    This module provides operations to support the use 
    of C150NASTYFILE including filename assembly, file
    format checking, safe reading, and safe writing
    
***********************************************************/
#include "nastyfileops.h"
#include <openssl/sha.h>
#include <cstring>

using namespace std;
using namespace C150NETWORK;




/**********************************************************
 * Function: makeFileName
 
 * Parameters: 
    * string dir   : Directory name containing the file
    * string fname : Name of the file within the directory

 * Return: A string representing a relative file path

 * Notes:
    * written by: Noah Mendelsohn from: nastyfiletest.cpp
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
    * string fname: Relative path of a file

 * Return: A boolean (true if fname is a file, false if not)

 * Notes:
    * written by: Noah Mendelsohn from: nastyfiletest.cpp
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

/**********************************************************
 * Function: safeFRead
 
 * Parameters: 
    * unsigned int bytesToRead : number of bytes to read from given file
    * NASTYFILE &inputFile     : a reference to the NASTYFILE to read from
    * int size                 : the size of data in bytes to read
    * unsigned char *data      : char array to hold the data read from file
    * int offset               : offset in the file to read from

 * Return: A boolean representing a successful read

 * Notes: 
    * A successful read is defined as a read frequency of 75% after 50+ 
      reads and before 200 reads
    * Serves as a wrapper function for NASTYFILE's fread()
***********************************************************/
bool
safeFRead(unsigned int bytesToRead, NASTYFILE &inputFile, int size, 
          unsigned char *data, int offset) {
    size_t bytesRead = 0;

    float freq = 0;
    float tries = 0;

    unordered_map<string, float> contentStrings;

    do {

        // jump to offset to read from
        do {
            inputFile.fseek(offset, SEEK_SET);
            bytesRead = inputFile.fread(data, size, bytesToRead);
        } while (bytesRead != bytesToRead);
        
        // create key out of content read and increment hits
        string contentStr((const char *) data, bytesRead);
        int hits = contentStrings[contentStr] += 1;

        tries += 1;
        freq = hits/tries;
    } while ((freq < 0.75 || tries < 50) && tries < 200);

    return (freq >= 0.75);
}


/**********************************************************
 * Function: safeFWrite
 
 * Parameters: 
    * unsigned int bytesToWrite : number of bytes to write to given file
    * NASTYFILE &outputFile     : a reference to the NASTYFILE to write to
    * int size                  : the size of data in bytes to write
    * unsigned char *data       : char array holding that data to write
    * int offset                : offset in the file to write from
    * string targetName         : relative path of the file being written to
    * unsigned char *memFile    : char pointer stores the data written to memory

 * Return: A boolean representing a successful write

 * Notes: 
    * A successful write is defined as safeFRead reading the same values
      as passed in through *data within 10 tries
    * Makes at most 10 calls to safeFRead to certify a valid write
***********************************************************/
bool
safeFWrite(unsigned int bytesToWrite, NASTYFILE &outputFile, int size, 
           unsigned char *data, int offset, string targetName, unsigned char *memFile) {

    int failedAttempts = 0;
    bool success       = false;

    void *filePtr = NULL;
    unsigned int bytesWritten = 0;
    int fileClosed = 1;

    struct stat statbuf;
    size_t bytesAppended;
    int statReadSuccess = 0;

    // to store the newly written data read through safeFRead
    unsigned char dataRead[bytesToWrite];

    do {
        failedAttempts += 1;

        // open file in r+b to avoid wiping data
        do {
            filePtr = outputFile.fopen(targetName.c_str(), "r+b");
        } while (filePtr == NULL);

        // jump to offset in file
        do {
            outputFile.fseek(offset, SEEK_SET);
            bytesWritten = outputFile.fwrite(data, 1, bytesToWrite);
        } while (bytesWritten != bytesToWrite);

        // close file before read
        do {
            fileClosed = outputFile.fclose(); 
        } while (fileClosed != 0);

        // confirm new file length
        do {
            statReadSuccess = lstat(targetName.c_str(), &statbuf);
        } while (statReadSuccess != 0);

        bytesAppended = statbuf.st_size - offset;
        if (bytesAppended != bytesToWrite) {
            continue;
        }

        // reopen file and perform safeFRead
        do {
            filePtr = outputFile.fopen(targetName.c_str(), "rb");
        } while (filePtr == NULL);
        
        safeFRead(bytesToWrite, outputFile, size, dataRead, offset); 
        success = (memcmp(data, dataRead, bytesToWrite) == 0);

        // close file
        do {
            fileClosed = outputFile.fclose(); 
        } while (fileClosed != 0);

    } while (!success && (failedAttempts < 10));

    if (success && (memFile != NULL)) {
        memcpy(memFile + offset, dataRead, bytesToWrite);
    }

    return success;
}
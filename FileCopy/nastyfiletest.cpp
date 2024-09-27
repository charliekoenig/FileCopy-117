// 
//            nastytestfile.cpp
//
//     Author: Noah Mendelsohn
//
//     This is a demonstration program designed to illustrate
//
//
//        1) The use of the C150NastyFile class, which must
//           be used for all file input/output in the
//           end-to-end programming assignment.
//
//        2) Some techniques for iterating through
//           the files in a directory. You can learn
//           these from Kerrisk, but there's a lot of
//           detail that's a nuissance.
//
//    You MAY steal code from this sample for use
//    in your end-to-end project.
//
//    Command line:
//
//         nastyfile <nastiness> <sourceDir> <targetDir>
//
//    Copies all files in sourceDir into targetDir.
//
//    targetDir must be an existing directory. Copied files
//    are added to the target, and existing files of the
//    same name are overwritten.
//
//    Note that this nastiness refers to errors that the
//    framework introduces into your file reads and writes.
//    These are separate from packet-related errors.
//    introduced by the networking classes.
//


#include "c150nastyfile.h"        // for c150nastyfile & framework
#include "c150grading.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>                // for errno string formatting
#include <cerrno>
#include <cstring>               // for strerro
#include <iostream>               // for cout
#include <fstream>                // for input files

//
// Always use namespace C150NETWORK with COMP 150 IDS framework!
//
using namespace C150NETWORK;

void copyFile(string sourceDir, string fileName, string targetDir, int nastiness); // fwd decl
bool isFile(string fname);
void checkDirectory(char *dirname);

// ------------------------------------------------------
//                   Main program
// ------------------------------------------------------

int
main(int argc, char *argv[]) {

  int nastiness;
  DIR *SRC;                   // Unix descriptor for open directory
  DIR *TARGET;                // Unix descriptor for target
  struct dirent *sourceFile;  // Directory entry for source file

  //
  //  DO THIS FIRST OR YOUR ASSIGNMENT WON'T BE GRADED!
  //
  
  GRADEME(argc, argv);

  //
  // Check command line and parse arguments
  //
  if (argc != 4)  {
    fprintf(stderr,"Correct syntxt is: %s <nastiness_number> <SRC dir> (TARGET dir>\n", argv[0]);
       exit(1);
  }

  if (strspn(argv[1], "0123456789") != strlen(argv[1])) {
    fprintf(stderr,"Nastiness %s is not numeric\n", argv[1]);     
    fprintf(stderr,"Correct syntxt is: %s <nastiness_number> <SRC dir> (TARGET dir>\n", argv[0]);     
     exit(4);
  }

  nastiness = atoi(argv[1]);   // convert command line string to integer

  //
  // Make sure source and target dirs exist
  //
  checkDirectory(argv[2]);
  checkDirectory(argv[3]);

  //
  // Open the source directory
  //
  SRC = opendir(argv[2]);
  if (SRC == NULL) {
    fprintf(stderr,"Error opening source directory %s\n", argv[2]);     
    exit(8);
  }

  //
  // Open the target directory (we don't really need to do
  // this here, but it will give cleaner error messages than
  // waiting for individual file writes to fail.
  //
  TARGET = opendir(argv[3]);
  if (TARGET == NULL) {
    fprintf(stderr,"Error opening target directory %s \n", argv[3]);     
    exit(8);
  }
  closedir(TARGET);          // we just wanted to be sure it was there
                             // SRC dir remains open for loop below
  
  //
  //  Loop copying the files
  //
  //    copyfile takes name of target file
  //
  while ((sourceFile = readdir(SRC)) != NULL) {
    // skip the . and .. names
    if ((strcmp(sourceFile->d_name, ".") == 0) ||
	(strcmp(sourceFile->d_name, "..")  == 0 )) 
      continue;          // never copy . or ..

    // do the copy -- this will check for and 
    // skip subdirectories
    copyFile(argv[2], sourceFile->d_name,  argv[3], nastiness);
  }
  closedir(SRC);
}



// ------------------------------------------------------
//
//                   makeFileName
//
// Put together a directory and a file name, making
// sure there's a / in between
//
// ------------------------------------------------------

string
makeFileName(string dir, string name) {
  stringstream ss;

  ss << dir;
  // make sure dir name ends in /
  if (dir.substr(dir.length()-1,1) != "/")
    ss << '/';
  ss << name;     // append file name to dir
  return ss.str();  // return dir/name
  
}


// ------------------------------------------------------
//
//                   checkDirectory
//
//  Make sure directory exists
//     
// ------------------------------------------------------

void
checkDirectory(char *dirname) {
  struct stat statbuf;  
  if (lstat(dirname, &statbuf) != 0) {
    fprintf(stderr,"Error stating supplied source directory %s\n", dirname);
    exit(8);
  }

  if (!S_ISDIR(statbuf.st_mode)) {
    fprintf(stderr,"File %s exists but is not a directory\n", dirname);
    exit(8);
  }
}


// ------------------------------------------------------
//
//                   isFile
//
//  Make sure the supplied file is not a directory or
//  other non-regular file.
//     
// ------------------------------------------------------

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


// ------------------------------------------------------
//
//                   copyFile
//
// Copy a single file from sourcdir to target dir
//
// ------------------------------------------------------

void
copyFile(string sourceDir, string fileName, string targetDir, int nastiness) {

  //
  //  Misc variables, mostly for return codes
  //
  void *fopenretval;
  size_t len;
  string errorString;
  char *buffer;
  struct stat statbuf;  
  size_t sourceSize;

  //
  // Put together directory and filenames SRC/file TARGET/file
  //
  string sourceName = makeFileName(sourceDir, fileName);
  string targetName = makeFileName(targetDir, fileName);

  //
  // make sure the file we're copying is not a directory
  // 
  if (!isFile(sourceName)) {
    cerr << "Input file " << sourceName << " is a directory or other non-regular file. Skipping" << endl;
    return;
  }
  

  cout << "Copying " << sourceName << " to " << targetName << endl;

  // - - - - - - - - - - - - - - - - - - - - -
  // LOOK HERE! This demonstrates the 
  // COMP 150 Nasty File interface
  // - - - - - - - - - - - - - - - - - - - - -

  try {

    //
    // Read whole input file 
    //
    if (lstat(sourceName.c_str(), &statbuf) != 0) {
      fprintf(stderr,"copyFile: Error stating supplied source file %s\n", sourceName.c_str());
     exit(20);
    }
  
    //
    // Make an input buffer large enough for
    // the whole file
    //
    sourceSize = statbuf.st_size;
    buffer = (char *)malloc(sourceSize);

    //
    // Define the wrapped file descriptors
    //
    // All the operations on outputFile are the same
    // ones you get documented by doing "man 3 fread", etc.
    // except that the file descriptor arguments must
    // be left off.
    //
    // Note: the NASTYFILE type is meant to be similar
    //       to the Unix FILE type
    //
    NASTYFILE inputFile(nastiness);      // See c150nastyfile.h for interface
    NASTYFILE outputFile(nastiness);     // NASTYFILE is supposed to
                                         // remind you of FILE
                                         //  It's defined as: 
                                         // typedef C150NastyFile NASTYFILE
  
    // do an fopen on the input file
    fopenretval = inputFile.fopen(sourceName.c_str(), "rb");  
                                         // wraps Unix fopen
                                         // Note rb gives "read, binary"
                                         // which avoids line end munging
  
    if (fopenretval == NULL) {
      cerr << "Error opening input file " << sourceName << 
	      " errno=" << strerror(errno) << endl;
      exit(12);
    }
  

    // 
    // Read the whole file
    //
    len = inputFile.fread(buffer, 1, sourceSize);
    if (len != sourceSize) {
      cerr << "Error reading file " << sourceName << 
	      "  errno=" << strerror(errno) << endl;
      exit(16);
    }
  
    if (inputFile.fclose() != 0 ) {
      cerr << "Error closing input file " << targetName << 
	      " errno=" << strerror(errno) << endl;
      exit(16);
    }


    //
    // do an fopen on the output file
    //
    fopenretval = outputFile.fopen(targetName.c_str(), "wb");  
                                         // wraps Unix fopen
                                         // Note wb gives "write, binary"
                                         // which avoids line and munging
  
    //
    // Write the whole file
    //
    len = outputFile.fwrite(buffer, 1, sourceSize);
    if (len != sourceSize) {
      cerr << "Error writing file " << targetName << 
	      "  errno=" << strerror(errno) << endl;
      exit(16);
    }
  
    if (outputFile.fclose() == 0 ) {
       cout << "Finished writing file " << targetName <<endl;
    } else {
      cerr << "Error closing output file " << targetName << 
	      " errno=" << strerror(errno) << endl;
      exit(16);
    }

    //
    // Free the input buffer to avoid memory leaks
    //
    free(buffer);


    //
    // Handle any errors thrown by the file framekwork
    //
  }   catch (C150Exception& e) {
       cerr << "nastyfiletest:copyfile(): Caught C150Exception: " << 
	       e.formattedExplanation() << endl;
    
  }

       
}
  

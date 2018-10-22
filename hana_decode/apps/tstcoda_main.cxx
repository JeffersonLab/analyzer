// Test of THaCodaData abstract class
// THaCodaFile and THaEtClient both inherit from THaCodaData.
// R. Michaels, March 2001.

#include "THaCodaFile.h"
#include "THaEtClient.h"
#include "TString.h"
#include <iostream>

using namespace std;
using namespace Decoder;

void usage();
void do_something(UInt_t* data);

int main(int argc, char* argv[])
{

  THaCodaData *coda;      // THaCodaData is abstract

  if (argc < 3) {
     usage();
     return 1;
  }

  int choice1 = atoi(argv[1]);
  int choice2 = atoi(argv[2]);

  if (choice1 == 1) {  // CODA File

// CODA file "snippet.dat" is a disk file of CODA data.  
      TString filename("snippet.dat");

      if (choice2 == 1) { // Two types of THaCodaFile constructors 
         coda = new THaCodaFile();
         if (coda->codaOpen(filename) != 0) {
            cout << "ERROR:  Cannot open CODA data" << endl;
            return 1;
         }
      } else {  // 2nd type of c'tor
         coda = new THaCodaFile(filename);
      }

  } else {         // Online ET connection

      int mymode = 1;
      TString mycomputer("129.57.32.30");
      TString mysession("eel1");

      if (choice2 == 1) {   // Three different types of constructor 
         coda = new THaEtClient();
         if (coda->codaOpen(mycomputer, mysession, mymode) != 0) {
            cout << "ERROR:  Cannot open ET connection" << endl;
            return 1;
          }
      } else if (choice2 == 2) {
         coda = new THaEtClient(mycomputer, mymode); 
      } else {
         coda = new THaEtClient(mycomputer, mysession, mymode);  
      }

  }

// Loop over events
  int NUMEVT=20000;

  for (int iev = 0; iev < NUMEVT; iev++) {

      int status = coda->codaRead();  

      if (status != 0) {
        if ( status == -1) {
           if (choice1 == 1) cout << "End of CODA file. Bye bye." << endl;
           if (choice1 == 2) cout << "CODA/ET not running. Bye bye." << endl;
        } else {
	   cout << "ERROR: codaRread status = " << hex << status << endl;
        }
        coda->codaClose();
        return 0;

      } else {
        
        do_something( coda->getEvBuffer() );

      }
  }

  coda->codaClose();
  return 0;
};


void usage() {  
  cout << "Usage:  'tstcoda choice1 choice2' " << endl;
  cout << "\n where choice1 = " << endl;
  cout << "   1. open a CODA file and print data" << endl;
  cout << "   2. open an ET connection and print data "<<endl;
  cout << "\n and choice2 = " << endl;
  cout << "1 - 3 are different ways to open connection "<<endl;
  cout << "If CODA file, you have 2 choices "<<endl;
  cout << "If ET connection, you have 3 choices "<<endl;
};

void do_something (UInt_t* data) {
  int len = data[0] + 1;
  int evtype = data[1]>>16;
  int evnum = data[4];
 // Crude event dump
  cout << "\n\n Event number " << dec << evnum;
  cout << " length " << len << " type " << evtype << endl;
  int ipt = 0;
  for (int j = 0; j < (len/5); j++) {
      cout << dec << "\n evbuffer[" << ipt << "] = ";
      for (int k=j; k<j+5; k++) {
    	 cout << hex << data[ipt++] << " ";
      }
      cout << endl;
  }
  if (ipt < len) {
      cout << dec << "\n evbuffer[" << ipt << "] = ";
      for (int k=ipt; k<len; k++) {
	 cout << hex << data[ipt++] << " ";
      }
      cout << endl;
  }
};






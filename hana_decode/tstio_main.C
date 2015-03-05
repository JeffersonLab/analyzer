// Test of I/O library and codaFile class
// R. Michaels, Jan 2000

#include <iostream>
#include <cstdlib>
#include "Decoder.h"
#include "THaCodaFile.h"
#include "THaCodaDecoder.h"
#include "THaEvData.h"
#include "evio.h"
#include "THaSlotData.h"
#include "TString.h"

using namespace std;
using namespace Decoder;

int main(int argc, char* argv[])
{

  if (argc < 2) {
     cout << "Usage:  tstio <choice> " << endl;
     cout << "where choice = " << endl;
     cout << "   1. open a file and print raw data" << endl;
     cout << "   2. filter data from file1 to file2" << endl;
     cout << "   3. Make a fast sum of event types" << endl;
     exit(0);
  } 

  int Choice = atoi(argv[1]);

  if ( Choice == 1 ) {

   // CODA file "snippet.dat" is a disk file of CODA data.  
   //TString filename("snippet.dat");
   TString filename("snippet.dat");

   // Two types of constructor
   //      THaCodaFile datafile;   
   THaCodaFile datafile(filename);

   // codaOpen is only necessary if one uses the constructor
   // without the filename arg.
   //   if (datafile.codaOpen(filename) != S_SUCCESS) {
   //        cout << "ERROR:  Cannot open CODA data" << endl;
   //        exit(0);
   //   }
      
   // Loop over events
      int NUMEVT=100;
      for (int iev=0; iev<NUMEVT; iev++) {
   	 int status = datafile.codaRead();  
         if (status != S_SUCCESS) {
	   if ( status == EOF) {
             cout << "Normal end of file.  Bye bye." << endl;
	   } else {
  	     cout << hex << "ERROR: codaRread status = " << status << endl;
 	   }
           exit(1);
	 } else {
            UInt_t *data = datafile.getEvBuffer();
            int len = data[0] + 1;
            int evtype = data[1]>>16;
    // Crude event dump
            cout << "\n\n Event number " << dec << iev;
            cout << " length " << len << " type " << evtype << endl;
            int ipt = 0;
	    for (int j=0; j<(len/5); j++) {
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
	 }
      }

  // Filter from file1 to file2

  } else if ( Choice == 2 ) {

      THaCodaFile file1("snippet.dat");
     
      // Filter criteria.  If no criteria, write out all events.
      // Possible criteria are logically inclusive.

      file1.addEvTypeFilt(1);  // filter this event type    
      file1.addEvTypeFilt(5);  // and this event type

     // Optionally filter a list of event numbers
      int my_event_list[] = { 4, 15, 20, 1050, 80014, 5605001 };
      for (int jlist=0; jlist<6; jlist++) {
          file1.addEvListFilt(my_event_list[jlist]);
      }

      // Max num of events to filter to output file.
      file1.setMaxEvFilt(20);  

      // Now filter
      const char* output_file = "filter_output.dat";
      int ret = file1.filterToFile(output_file);
      cout << "\n Return status from filtering " << dec << ret << endl;
      exit(1);

  } else {    // Rapidly read file and count event types
  
      const char* filename = "snippet.dat";
      THaCodaFile datafile(filename);
      static const int MAXEVTYPE = 200;
      int evtype_sum[MAXEVTYPE];
      for (int kk=0; kk<MAXEVTYPE; kk++) evtype_sum[kk] = 0;
      int nevt=0;
      while (datafile.codaRead() == S_SUCCESS) {
          nevt++;
          UInt_t* dbuff = datafile.getEvBuffer();
          int event_type = dbuff[1]>>16;
          if ((event_type >= 0) && (event_type < MAXEVTYPE)) {
              evtype_sum[event_type]++;
	  }
      }
      cout << "\n Summary of event types in file " << filename;
      cout << "\n Total number events read "<<dec<<nevt<<endl;
      for (int ety=0; ety<MAXEVTYPE; ety++) {
	if (evtype_sum[ety] != 0) {
	  cout << "Event type " << ety;
          cout << "  Number in file = " << evtype_sum[ety] << endl;
	}
      }

  }

}
     






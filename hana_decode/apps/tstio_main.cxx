// Test of I/O library and codaFile class
// R. Michaels, Jan 2000

#include <iostream>
#include <iomanip>
#include "THaCodaFile.h"
#include "CodaDecoder.h"
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
    exit(1);
  } 

  // CODA file "snippet.dat" is a disk file of CODA data.
  TString filename("snippet.dat");
  THaCodaFile datafile;
  if (datafile.codaOpen(filename) != CODA_OK) {
    cerr << "ERROR:  Cannot open CODA data" << endl;
    cerr << "Perhaps you mistyped it" << endl;
    cerr << "... exiting." << endl;
    exit(2);
  }

  int Choice = atoi(argv[1]);
  switch(Choice) {
  case 1: {
    // Loop over events

    int NUMEVT=100;
    for (int iev=0; iev<NUMEVT; iev++) {
      int status = datafile.codaRead();
      if (status != CODA_OK) {
        if ( status == EOF) {
          cout << "Normal end of file.  Bye bye." << endl;
          break;
        } else {
          cout << hex << "ERROR: codaRread status = " << status << endl;
          exit(status);
        }
      } else {
        UInt_t *data = datafile.getEvBuffer();
        UInt_t len = data[0] + 1;
        UInt_t evtype = data[1]>>16;
        // Crude event dump
        cout << endl<<endl<<"Event number " << dec << iev;
        cout << " length " << len << " type " << evtype << endl;
        CodaDecoder::hexdump((const char*)data, len);
      }
    }
    break;
  }

  case 2: {
    // Filter from file1 to file2

    // Filter criteria.  If no criteria, write out all events.
    // Possible criteria are logically inclusive.

    datafile.addEvTypeFilt(1);  // filter this event type
    datafile.addEvTypeFilt(5);  // and this event type

    // Optionally filter a list of event numbers
    int my_event_list[] = { 4, 15, 20, 1050, 80014, 5605001 };
    for (int jlist=0; jlist<6; jlist++) {
      datafile.addEvListFilt(my_event_list[jlist]);
    }

    // Max num of events to filter to output file.
    datafile.setMaxEvFilt(20);

    // Now filter
    const char* output_file = "filter_output.dat";
    int ret = datafile.filterToFile(output_file);
    cout << endl << "Return status from filtering " << dec << ret << endl;
    break;
  }

  default: {
    // Rapidly read file and count event types

    static const int MAXEVTYPE = 200;
    int evtype_sum[MAXEVTYPE];
    for (int kk=0; kk<MAXEVTYPE; kk++) evtype_sum[kk] = 0;
    int nevt=0;
    cout << "Scanning " << filename << endl;
    while (datafile.codaRead() == CODA_OK) {
      if( (nevt%1000)==0 ) cout << "." << flush;
      nevt++;
      UInt_t* dbuff = datafile.getEvBuffer();
      int event_type = dbuff[1]>>16;
      if ((event_type >= 0) && (event_type < MAXEVTYPE)) {
        evtype_sum[event_type]++;
      }
    }
    ios_base::fmtflags fmt = cout.flags();
    cout << endl << "Summary of event types";
    cout << endl << " Total number events read "<<dec<<nevt<<endl;
    for (int ety=0; ety<MAXEVTYPE; ety++) {
      if (evtype_sum[ety] != 0) {
        cout << " Event type " << setw(3) << right << ety;
        cout << ", count = "   << setw(7) << right << evtype_sum[ety] << endl;
      }
    }
    cout.flags(fmt);
    break;
  }
  } // switch(Choice)

  datafile.codaClose();
  return 0;
}

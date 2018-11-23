//------------------------------------------------
// prfact  -- To print out the prescale factors found
//            in CODA file, then exit.
// 
#include <iostream>
#include <cstdlib>
#include "THaCodaFile.h"
#include "CodaDecoder.h"

using namespace std;
using namespace Decoder;

int main(int argc, char* argv[])
{
  if (argc < 2) {
    cout << "You made a mistake... \n" << endl;
    cout << "Usage:   prfact filename" << endl;
    cout << "  where 'filename' is the CODA file"<<endl;
    cout << "\n... exiting." << endl;
    exit(1);
  }
  const char* filename = argv[1];
  THaCodaFile datafile;
  if (datafile.codaOpen(filename) != CODA_OK) {
    cout << "ERROR:  Cannot open CODA data" << endl;
    cout << "Perhaps you mistyped it" << endl;
    cout << "... exiting." << endl;
    exit(2);
  }

  CodaDecoder* evdata = new CodaDecoder();
  evdata->SetCodaVersion(datafile.getCodaVersion());

  // Can tell evdata whether to use evtype
  // 133 or 120 for prescale data.  Default is 120.
  evdata->SetOrigPS(133);   // args are 120 or 133
  cout << "Origin of PS data "<<evdata->GetOrigPS()<<endl;

  // Loop over a finite number of events
  int NUMEVT = 500;
  bool found = false;
  for (int i=0; i<NUMEVT; i++) {

    int status = datafile.codaRead();

    if ( status != CODA_OK ) {
      if ( status == EOF) {
        cout << "This is end of file !" << endl;
        cout << "... exiting " << endl;
        exit(1);
      } else {
        cout << hex << "ERROR: codaRead status = "
            << status << dec << endl;
        exit(2);
      }
    }

    status = evdata->LoadEvent( datafile.getEvBuffer() );

    if( status != CodaDecoder::HED_OK && status != CodaDecoder::HED_WARN ) {
      cerr << "ERROR: LoadEvent status " << status << endl;
      exit(3);
    }

    if(evdata->IsPrescaleEvent()) {
      cout << endl << " Prescale factors from CODA file = "
          << filename << endl;
      cout << endl << " Trigger      Prescale Factor" << endl;
      for (int trig=1; trig<=8; trig++) {
        cout <<"   "<<dec<<trig<<"             ";
        int ps = evdata->GetPrescaleFactor(trig);
        int psmax;
        if (trig <= 4) psmax = 16777216;  // 2^24
        if (trig >= 5) psmax = 65536;     // 2^16
        ps = ps % psmax;
        if (ps == 0) ps = psmax;
        cout << ps << endl;
      }
      cout << endl;
      cout << "Reminder: A 'zero' was interpreted as maximum." << endl;
      cout << "Max for trig 1-4 = 2^24, for trig 5-8 = 2^16 \n"<< endl;
      found = true;
      break;
    }
  }  //  end of event loop

  if( !found ) {
    cout << "ERROR: prescale factors not found in the first ";
    cout << dec << NUMEVT << " events " << endl;
    exit(4);
  }

  datafile.codaClose();
  exit(0);
}

//------------------------------------------------
// epics_main.C   -- to find EPICS data and 
//                   do something with them.
// 
#include <iostream>
#include <cstdlib>
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "TString.h"

using namespace std;
using namespace Decoder;

int main(int argc, char* argv[])
{
  if (argc < 2) {
    cout << "You made a mistake... \n" << endl;
    cout << "Usage:   epicsd filename" << endl;
    cout << "  where 'filename' is the CODA file"<<endl;
    cout << "\n... exiting." << endl;
    exit(1);
  }
  TString filename = argv[1];
  THaCodaFile datafile;
  if (datafile.codaOpen(filename) != CODA_OK) {
    cout << "ERROR:  Cannot open CODA data" << endl;
    cout << "Perhaps you mistyped it" << endl;
    cout << "... exiting." << endl;
    exit(2);
  }

  CodaDecoder *evdata = new CodaDecoder();
  evdata->SetCodaVersion(datafile.getCodaVersion());

  // Loop over a finite number of events

  int NUMEVT=1000000;
  for (int i=0; i<NUMEVT; i++) {

    int status = datafile.codaRead();

    if ( status != CODA_OK ) {
      if ( status == EOF) {
        cout << "This is end of file !" << endl;
        cout << "... exiting " << endl;
        break;
      } else {
        cout << hex << "ERROR: codaRread status = " << status << endl;
        exit(status);
      }
    }

    status = evdata->LoadEvent( datafile.getEvBuffer() );

    if( status != CodaDecoder::HED_OK && status != CodaDecoder::HED_WARN ) {
      cerr << "ERROR " << status << " while decoding event " << i
          << ". Exiting." << endl;
      exit(status);
    }

    if(evdata->IsEpicsEvent()) {

      cout << "Event " << i << " EPICS data --> "<<endl;
//FIXME: needs update for Podd 1.6+
      cout << "Not implemented in this example program" << endl;
#if 0
      cout << "hac_bcm_average "<<
          evdata->GetEpicsData("hac_bcm_average")<<endl;
      cout << "IPM1H04A.XPOS  "<<
          evdata->GetEpicsData("IPM1H04A.XPOS")<<endl;
      cout << "IPM1H04A.YPOS  "<<
          evdata->GetEpicsData("IPM1H04A.YPOS")<<endl;
#endif
    }

  }  //  end of event loop

  datafile.codaClose();
  return 0;
}








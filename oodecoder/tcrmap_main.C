// Test of the crate map manipulations
//
// R. Michaels, May, 2014

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include "THaCrateMap.h"

#include "CodaDecoder.h"
#include "THaEvData.h"
#include "THaCodaFile.h"
#include "TDatime.h"
#include "evio.h"
#include "THaSlotData.h"
#include "TString.h"

using namespace std;


int main(int argc, char* argv[])
{

  TDatime *dtime = new TDatime();
  dtime->Set(2014,2,25,0,0,0);
  UInt_t mydate = dtime->Get();
  cout << "mydate  =  0x"<<hex<<mydate<<"    dec "<<dec<<mydate<<endl;

  THaCrateMap *map = new THaCrateMap();

  map->init(mydate);

  cout << "-----------------------------------------------"<<endl;
  cout << " print of cratemap "<<endl;

  map->print();

  CodaDecoder *deco = new CodaDecoder();

  TString filename("snippet.dat");

  THaCodaFile datafile(filename);
  THaEvData *evdata = new CodaDecoder();

  // Loop over events
  int NUMEVT=20;
  for (int iev=0; iev<NUMEVT; iev++) {
      int status = datafile.codaRead();  
      if (status != S_SUCCESS) {
	 if ( status == EOF) {
             cout << "Normal end of file.  Bye bye." << endl;
         } else {
  	     cout << hex << "ERROR: codaRread status = " << status << endl;	         }
         exit(1);
      } else {

      int *data = datafile.getEvBuffer();

      evdata->LoadEvent( data );   

      }
  }


}





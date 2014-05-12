// Test of the crate map manipulations
//
// R. Michaels, May, 2014

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include "THaCrateMap.h"
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
  cout << "55 mydate  =  0x"<<hex<<mydate<<"    dec "<<dec<<mydate<<endl;

  cout << "aaaa"<<endl;

  THaCrateMap *map = new THaCrateMap();

  cout << "bbbb"<<endl;

  map->init(mydate);

  cout << "-----------------------------------------------"<<endl;
  cout << " print of cratemap "<<endl;

  map->print();


}




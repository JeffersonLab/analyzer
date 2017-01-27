//////////////////////////////////////////////////////////////////////////
//
//  The Hall A analyzer interactive interface
//
//////////////////////////////////////////////////////////////////////////

#include "THaInterface.h"
#include <iostream>
#include <cstring>
#include "ha_compiledata.h"

using namespace std;

int main(int argc, char **argv)
{
  // Create a ROOT-style interactive interface

  if( argc > 1 &&
      (!strcmp(argv[1],"--version") || !strcmp(argv[1],"-v")) ) {
    cout << "Jefferson Lab Hall A Data Analysis Software version "
	 << HA_VERSION << endl;
    cout << "Built " << HA_DATETIME << " on " << HA_PLATFORM;
#ifdef HA_GITVERS
    if( strlen(HA_GITVERS) > 0 )
      cout << " git @" << HA_GITVERS;
#endif
    cout << endl;
    return 0;
  }

  TApplication *theApp = 
    new THaInterface( "The Hall A analyzer", &argc, argv );
  theApp->Run(kFALSE);

  cout << endl;
  delete theApp;

  return 0;
}

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

  bool print_version = false, no_logo = false;
  for( int i=1; i<argc; ++i ) {
    if( !strcmp(argv[i],"-l") )
      no_logo = true;
    else if(!strcmp(argv[1],"--version") || !strcmp(argv[1],"-v")) {
      print_version = true;
      break;
    }
  }

  if( print_version ) {
    cout << "Jefferson Lab Hall A/C Data Analysis Software version "
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
    new THaInterface( "The Hall A analyzer", &argc, argv, 0, 0, no_logo );
  theApp->Run(kFALSE);

  cout << endl;
  delete theApp;

  return 0;
}

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

  // Handle convenience command line options
  bool print_version = false, no_logo = false;
  for( int i=1; i<argc; ++i ) {
    if( !strcmp(argv[i],"-l") )
      no_logo = true;
    else if( !strcmp(argv[1],"-v") || !strcmp(argv[1],"--version") ) {
      print_version = true;
      break;
    }
  }

  if( print_version ) {
    cout << "Podd " << HA_VERSION << " " << HA_PLATFORM;
    if( strlen(HA_GITVERS) > 0 )
      cout << " git @" << HA_GITVERS;
    if( strlen(HA_ROOTVERS) )
      cout << " ROOT " << HA_ROOTVERS;
    cout << endl;
//     cout << "Jefferson Lab Hall A/C Data Analysis Software (Podd)" << endl;
//     cout << "Version " << HA_VERSION;
// #ifdef LINUXVERS
//     cout << " for Linux";
// #endif
// #ifdef MACVERS
//     cout << " for Mac OS X";
// #endif
//     if( print_buildinfo ) {
//       cout << " built " << HA_DATETIME << endl;
//       cout << "by " << HA_BUILDUSER << " on " HA_BUILDNODE
// 	   << " in " << HA_BUILDDIR;
// if( strlen(HA_GITVERS) > 0 )
//   cout << " git @" << HA_GITVERS;
// cout << endl;
//       cout << "Build platform" << endl;
//       cout << "  " << HA_PLATFORM << endl;
//       if( strlen(HA_CXXVERS) > 0 ) {
// 	cout << "  " << HA_CXXVERS << endl;
//       }
//       if( strlen(HA_ROOTVERS) ) {
// 	cout << "  ROOT " << HA_ROOTVERS << endl;
//       }
//     } else {
//       cout << " built " << HA_DATE << endl;
//     }
    return 0;
  }

  TApplication *theApp =
    new THaInterface( "The Hall A analyzer", &argc, argv, 0, 0, no_logo );
  theApp->Run(kFALSE);

  cout << endl;
  delete theApp;

  return 0;
}

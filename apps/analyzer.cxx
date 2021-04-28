//////////////////////////////////////////////////////////////////////////
//
//  The Hall A analyzer interactive interface
//
//////////////////////////////////////////////////////////////////////////

#include "THaInterface.h"
#include <iostream>
#include <cstring>
#include <memory>

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
    cout << THaInterface::GetVersionString() << endl;
    return 0;
  }

  unique_ptr<TApplication> theApp{
    new THaInterface("The Hall A analyzer", &argc, argv, nullptr, 0, no_logo)};
  theApp->Run(false);

  cout << endl;

  return 0;
}

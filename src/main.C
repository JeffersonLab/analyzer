//////////////////////////////////////////////////////////////////////////
//
//  The Hall A analyzer interactive interface
//
//////////////////////////////////////////////////////////////////////////

#include "THaInterface.h"
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
  // Create a ROOT-style interactive interface

  TApplication *theApp = 
    new THaInterface( "The Hall A analyzer", &argc, argv );
  theApp->Run(kFALSE);

  cout << endl;

  delete theApp;

  return 0;
}

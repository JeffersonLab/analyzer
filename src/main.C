//////////////////////////////////////////////////////////////////////////
//
//  The Hall A analyzer interactive interface
//
//////////////////////////////////////////////////////////////////////////

#include "THaInterface.h"

int main(int argc, char **argv)
{
  // Create a ROOT-style interactive interface

  TRint *theApp = new THaInterface( "The Hall A analyzer", &argc, argv );
  theApp->Run(kTRUE);

  delete theApp;

  return 0;
}

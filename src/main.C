//////////////////////////////////////////////////////////////////////////
//
//  The Hall A analyzer interactive interface
//
//////////////////////////////////////////////////////////////////////////

#include "TROOT.h"
#include "THaInterface.h"

extern void InitGui();
VoidFuncPtr_t initfuncs[] = { InitGui, 0 };

TROOT root( "Analyzer", "The Hall A Analyzer Interactive Interface", initfuncs );

int main(int argc, char **argv)
{
  // Create a ROOT-style interactive interface

  TRint *theApp = new THaInterface( "The Hall A analyzer", &argc, argv );
  theApp->Run();

  delete theApp;

  return 0;
}

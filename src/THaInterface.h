#ifndef ROOT_THaInterface
#define ROOT_THaInterface

//////////////////////////////////////////////////////////////////////////
//
// THaInterface
// 
//////////////////////////////////////////////////////////////////////////

#include "TRint.h"
#include "THaGlobals.h"

class THaInterface : public TRint {

protected:
  static THaInterface*  fgAint;  //Pointer indicating that interface already exists

public:
  THaInterface( const char* appClassName, int* argc, char** argv,
		void* options = NULL, int numOptions = 0, 
		Bool_t noLogo = kFALSE );
  virtual ~THaInterface();

  virtual void          PrintLogo();

  ClassDef(THaInterface,0)  //Hall A Analyzer Interactive Interface
};

#endif

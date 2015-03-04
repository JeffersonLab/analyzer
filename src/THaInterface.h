#ifndef ROOT_THaInterface
#define ROOT_THaInterface

//////////////////////////////////////////////////////////////////////////
//
// THaInterface
// 
//////////////////////////////////////////////////////////////////////////

#include "TRint.h"
#include "Decoder.h"

class TClass;

class THaInterface : public TRint {

public:
  THaInterface( const char* appClassName, int* argc, char** argv,
		void* options = NULL, int numOptions = 0, 
		Bool_t noLogo = kFALSE );
  virtual ~THaInterface();

#if ROOT_VERSION_CODE < 332288  // 5.18/00
  virtual void PrintLogo();
#else
  virtual void PrintLogo(Bool_t lite = kFALSE);
#endif
  static TClass* GetDecoder();
  static TClass* SetDecoder( TClass* c );

protected:
  static THaInterface*  fgAint;  //Pointer indicating that interface already exists

  ClassDef(THaInterface,0)  //Hall A Analyzer Interactive Interface
};

#endif

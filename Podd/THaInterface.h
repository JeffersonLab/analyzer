#ifndef Podd_THaInterface_h_
#define Podd_THaInterface_h_

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
		void* options = nullptr, int numOptions = 0,
		Bool_t noLogo = false );
  virtual ~THaInterface();

  virtual void PrintLogo(Bool_t lite = false);
  static TClass* GetDecoder();
  static TClass* SetDecoder( TClass* c );
  static const char* GetVersion();
  static const char* GetVersionString();

  virtual const char* SetPrompt(const char *newPrompt);

protected:
  static THaInterface*  fgAint;  //Pointer indicating that interface already exists

  ClassDef(THaInterface,0)  //Hall A Analyzer Interactive Interface
};

#endif

#ifndef Podd_THaInterface_h_
#define Podd_THaInterface_h_

//////////////////////////////////////////////////////////////////////////
//
// THaInterface
//
//////////////////////////////////////////////////////////////////////////

#include "TRint.h"

class TClass;

class THaInterface : public TRint {

public:
  THaInterface( const char* appClassName, int* argc, char** argv,
		void* options = nullptr, int numOptions = 0,
		Bool_t noLogo = false );
  ~THaInterface() override;

  void PrintLogo(Bool_t lite = false) override;
  static TClass* GetDecoder();
  static TClass* SetDecoder( TClass* c );
  static const char* GetVersion();
  static const char* GetHaDate();
  static const char* GetVersionString();

  const char* SetPrompt(const char *newPrompt) override;

protected:
  static THaInterface*  fgAint;  //Pointer indicating that interface already exists

  static TString extract_short_date( const char* long_date );

  ClassDefOverride(THaInterface,0)  //Hall A Analyzer Interactive Interface
};

#endif

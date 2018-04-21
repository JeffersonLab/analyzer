#ifndef Podd_UserModule_h_
#define Podd_UserModule_h_

//////////////////////////////////////////////////////////////////////////
//
// UserModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"

class THaTrackingModule;

class UserModule : public THaPhysicsModule {
  
public:
  UserModule( const char* name, const char* description,
	      const char* spectro = "",
	      Double_t parameter = 0.0 );
  virtual ~UserModule();
  
  virtual void  Clear( Option_t* opt="" );

  Double_t      GetResultA() const { return fResultA; }
  Double_t      GetResultB() const { return fResultB; }
  
  Double_t      GetParameter() const { return fParameter; }

  void          SetParameter( Double_t value );
  void          SetSpectrometer( const char* name );

  virtual EStatus Init( const TDatime& run_time );
  virtual Int_t   Process( const THaEvData& );

protected:

  Double_t      fResultA;      // Example result A
  Double_t      fResultB;      // Example result B

  Double_t      fParameter;    // Example parameter

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadRunDatabase( const TDatime& date );

  void PrintInitError( const char* here );

  TString             fSpectroName;  // Name of spectrometer
  THaTrackingModule*  fSpectro;      // Pointer to spectrometer object

  ClassDef(UserModule,0)   //Example physics module
};

#endif

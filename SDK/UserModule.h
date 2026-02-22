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
  ~UserModule() override;

  void    Clear( Option_t* opt="" ) override;

  Data_t  GetResultA() const { return fResultA; }
  Data_t  GetResultB() const { return fResultB; }
  
  Data_t  GetParameter() const { return fParameter; }

  void    SetParameter( Data_t value );
  void    SetSpectrometer( const char* name );

  EStatus Init( const TDatime& run_time ) override;
  Int_t   Process( const THaEvData& ) override;

protected:

  Data_t        fResultA;      // Example result A
  Data_t        fResultB;      // Example result B

  Data_t        fParameter;    // Example parameter

  TString       fSpectroName;  // Name of spectrometer
  THaTrackingModule*  fSpectro;      // Pointer to spectrometer object

  Int_t   DefineVariables( EMode mode = kDefine ) override;
  Int_t   ReadRunDatabase( const TDatime& date ) override;

  ClassDefOverride(UserModule,0)   //Example physics module
};

#endif

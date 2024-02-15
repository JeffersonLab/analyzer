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

  Data_t        GetResultA() const { return fResultA; }
  Data_t        GetResultB() const { return fResultB; }
  
  Data_t        GetParameter() const { return fParameter; }

  void          SetParameter( Data_t value );
  void          SetSpectrometer( const char* name );

  virtual EStatus Init( const TDatime& run_time );
  virtual Int_t   Process( const THaEvData& );

protected:

  Data_t        fResultA;      // Example result A
  Data_t        fResultB;      // Example result B

  Data_t        fParameter;    // Example parameter

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadRunDatabase( const TDatime& date );

  TString             fSpectroName;  // Name of spectrometer
  THaTrackingModule*  fSpectro;      // Pointer to spectrometer object

  ClassDef(UserModule,0)   //Example physics module
};

#endif

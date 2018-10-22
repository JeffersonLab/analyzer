#ifndef Podd_THaReacPointFoil_h_
#define Podd_THaReacPointFoil_h_

//////////////////////////////////////////////////////////////////////////
//
// THaReacPointFoil
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "THaVertexModule.h"
#include "TString.h"

class THaSpectrometer;
class THaBeam;

class THaReacPointFoil : public THaPhysicsModule, public THaVertexModule {
  
public:
  THaReacPointFoil( const char* name, const char* description,
		    const char* spectro="", const char* beam="" );
  virtual ~THaReacPointFoil();
  
  virtual void      Clear( Option_t* opt="" );

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );
          void      SetSpectrometer( const char* name );
          void      SetBeam( const char* name );

protected:

  TString                 fSpectroName;  // Name of spectrom. to consider
  TString                 fBeamName;     // Name of beam position apparatus
  THaSpectrometer*        fSpectro;      // Pointer to spectrometer object
  THaBeam*                fBeam;         // Pointer to beam position apparatus

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(THaReacPointFoil,0)   //Single arm track-beam vertex module
};

//_________ inlines __________________________________________________________
inline 
void THaReacPointFoil::SetSpectrometer( const char* name ) {
  fSpectroName = name; 
}

//_____________________________________________________________________________
inline
void THaReacPointFoil::SetBeam( const char* name ) {
  fBeamName = name; 
}

#endif

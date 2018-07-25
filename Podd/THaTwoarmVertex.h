#ifndef Podd_THaTwoarmVertex_h_
#define Podd_THaTwoarmVertex_h_

//////////////////////////////////////////////////////////////////////////
//
// THaTwoarmVertex
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "THaVertexModule.h"
#include "TString.h"

class THaTrackingModule;

class THaTwoarmVertex : public THaPhysicsModule, public THaVertexModule {
  
public:
  THaTwoarmVertex( const char* name, const char* description,
		   const char* spectro1="", const char* spectro2="" );
  virtual ~THaTwoarmVertex();
  
  virtual void      Clear( Option_t* opt="" );

  Double_t          GetX()       const { return fVertex.X(); }
  Double_t          GetY()       const { return fVertex.Y(); }
  Double_t          GetZ()       const { return fVertex.Z(); }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );
          void      SetSpectrometers( const char* name1, const char* name2 );

protected:

  TString                 fName1;      // Name of spectrometer #1
  TString                 fName2;      // Name of spectrometer #2
  THaTrackingModule*      fSpectro1;   // Pointer to spectrometer #1 object
  THaTrackingModule*      fSpectro2;   // Pointer to spectrometer #2 object

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(THaTwoarmVertex,0)   //Two-arm vertex module
};

//_________ inlines __________________________________________________________
inline 
void THaTwoarmVertex::SetSpectrometers( const char* name1, const char* name2 ) 
{
  fName1 = name1; 
  fName2 = name2;
}

#endif

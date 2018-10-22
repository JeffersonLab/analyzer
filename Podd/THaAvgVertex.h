#ifndef Podd_THaAvgVertex_h_
#define Podd_THaAvgVertex_h_

//////////////////////////////////////////////////////////////////////////
//
// THaAvgVertex
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "THaVertexModule.h"
#include "TString.h"

class THaAvgVertex : public THaPhysicsModule, public THaVertexModule {
  
public:
  THaAvgVertex( const char* name, const char* description,
		const char* spectro1="", const char* spectro2="" );
  virtual ~THaAvgVertex();
  
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
  THaVertexModule*        fSpectro1;   // Pointer to spectrometer #1 object
  THaVertexModule*        fSpectro2;   // Pointer to spectrometer #2 object

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(THaAvgVertex,0)   //Two-arm vertex module
};

//_________ inlines __________________________________________________________
inline 
void THaAvgVertex::SetSpectrometers( const char* name1, 
				     const char* name2 ) 
{
  fName1 = name1; 
  fName2 = name2;
}

#endif

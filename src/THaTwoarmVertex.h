#ifndef ROOT_THaTwoarmVertex
#define ROOT_THaTwoarmVertex

//////////////////////////////////////////////////////////////////////////
//
// THaTwoarmVertex
//
//////////////////////////////////////////////////////////////////////////

#include "THaVertexModule.h"
#include "TVector3.h"
#include "TString.h"

class THaSpectrometer;

class THaTwoarmVertex : public THaVertexModule {
  
public:
  THaTwoarmVertex( const char* name, const char* description,
		   const char* spectro1="", const char* spectro2="" );
  virtual ~THaTwoarmVertex();
  
  virtual void      Clear( Option_t* opt="" );

  Double_t          GetX()       const { return fVertex.X(); }
  Double_t          GetY()       const { return fVertex.Y(); }
  Double_t          GetZ()       const { return fVertex.Z(); }
  const TVector3&   GetVertex()  const { return fVertex; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process();
          void      SetSpectrometers( const char* name1, const char* name2 );

protected:

  THaTwoarmVertex() : fSpectro1(NULL), fSpectro2(NULL) {}
  THaTwoarmVertex( const THaTwoarmVertex& ) {}
  THaTwoarmVertex& operator=( const THaTwoarmVertex& ) { return *this; }

  TVector3                fVertex;     // Intersection point of the two Golden Tracks

  TString                 fName1;      // Name of spectrometer #1
  const THaSpectrometer*  fSpectro1;   // Pointer to spectrometer #1 object
  TString                 fName2;      // Name of spectrometer #2
  const THaSpectrometer*  fSpectro2;   // Pointer to spectrometer #2 object

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(THaTwoarmVertex,0)   //Two-arm vertex module
};

//_________ inlines __________________________________________________________
inline 
void THaTwoarmVertex::SetSpectrometers( const char* name1, 
					    const char* name2 ) {
  fName1 = name1; 
  fName2 = name2;
}

#endif

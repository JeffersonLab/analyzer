#ifndef ROOT_THaAvgVertex
#define ROOT_THaAvgVertex

//////////////////////////////////////////////////////////////////////////
//
// THaAvgVertex
//
//////////////////////////////////////////////////////////////////////////

#include "THaVertexModule.h"
#include "TVector3.h"
#include "TString.h"

class THaSpectrometer;

class THaAvgVertex : public THaVertexModule {
  
public:
  THaAvgVertex( const char* name, const char* description,
		const char* spectro1="", const char* spectro2="" );
  virtual ~THaAvgVertex();
  
  virtual void      Clear( Option_t* opt="" );

  Double_t          GetX()       const { return fVertex.X(); }
  Double_t          GetY()       const { return fVertex.Y(); }
  Double_t          GetZ()       const { return fVertex.Z(); }
  const TVector3&   GetVertex()  const { return fVertex; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process();
          void      SetSpectrometers( const char* name1, const char* name2 );

protected:

  THaAvgVertex() : fSpectro1(NULL), fSpectro2(NULL) {}
  THaAvgVertex( const THaAvgVertex& ) {}
  THaAvgVertex& operator=( const THaAvgVertex& ) { return *this; }

  TVector3                fVertex;     // Intersection point of the two Golden Tracks

  TString                 fName1;      // Name of spectrometer #1
  const THaSpectrometer*  fSpectro1;   // Pointer to spectrometer #1 object
  TString                 fName2;      // Name of spectrometer #2
  const THaSpectrometer*  fSpectro2;   // Pointer to spectrometer #2 object

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

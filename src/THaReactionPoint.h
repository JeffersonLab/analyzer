#ifndef ROOT_THaReactionPoint
#define ROOT_THaReactionPoint

//////////////////////////////////////////////////////////////////////////
//
// THaReactionPoint
//
//////////////////////////////////////////////////////////////////////////

#include "THaVertexModule.h"
#include "TString.h"
#include "TVector3.h"

class THaSpectrometer;
class THaBeam;

class THaReactionPoint : public THaVertexModule {
  
public:
  THaReactionPoint( const char* name, const char* description,
		    const char* spectro="", const char* beam="" );
  virtual ~THaReactionPoint();
  
  virtual void      Clear( Option_t* opt="" );

  virtual const TVector3& GetVertex() const { return fVertex; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );
          void      SetSpectrometer( const char* name );
          void      SetBeam( const char* name );

protected:

  THaReactionPoint() : fSpectro(NULL), fBeam(NULL) {}
  THaReactionPoint( const THaReactionPoint& ) {}
  THaReactionPoint& operator=( const THaReactionPoint& ) { return *this; }

  TVector3                fVertex;       // Vertex of Golden Track

  TString                 fSpectroName;  // Name of spectrom. to consider
  const THaSpectrometer*  fSpectro;      // Pointer to spectrometer object
  TString                 fBeamName;     // Name of beam position apparatus
  const THaBeam*          fBeam;         // Pointer to beam position apparatus

  ClassDef(THaReactionPoint,0)   //Single arm track-beam vertex module
};

//_________ inlines __________________________________________________________
inline 
void THaReactionPoint::SetSpectrometer( const char* name ) {
  fSpectroName = name; 
}

//_____________________________________________________________________________
inline
void THaReactionPoint::SetBeam( const char* name ) {
  fBeamName = name; 
}

#endif

#ifndef ROOT_THaPhysVertex
#define ROOT_THaPhysVertex

//////////////////////////////////////////////////////////////////////////
//
// THaPhysVertex
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TString.h"

class THaSpectrometer;
class THaBeam;

class THaPhysVertex : public THaPhysicsModule {
  
public:
  THaPhysVertex( const char* name, const char* description,
		 const char* spectro="", const char* beam="" );
  virtual ~THaPhysVertex();
  
  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process();
          void      SetSpectrometer( const char* name );
          void      SetBeam( const char* name );

protected:

  THaPhysVertex() : fSpectro(NULL), fBeam(NULL) {}
  THaPhysVertex( const THaPhysVertex& ) {}
  THaPhysVertex& operator=( const THaPhysVertex& ) { return *this; }

  TString                 fSpectroName;  // Name of spectrom. to consider
  const THaSpectrometer*  fSpectro;      // Pointer to spectrometer object
  TString                 fBeamName;     // Name of beam position apparatus
  const THaBeam*          fBeam;         // Pointer to beam position apparatus

  ClassDef(THaPhysVertex,0)   //Single arm kinematics module
};

//_________ inlines __________________________________________________________
inline 
void THaPhysVertex::SetSpectrometer( const char* name ) {
  fSpectroName = name; 
}

//_____________________________________________________________________________
inline
void THaPhysVertex::SetBeam( const char* name ) {
  fBeamName = name; 
}

#endif

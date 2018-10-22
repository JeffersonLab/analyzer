#ifndef Podd_THaBeam_h_
#define Podd_THaBeam_h_

//////////////////////////////////////////////////////////////////////////
//
// THaBeam
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include "THaBeamModule.h"
#include "THaRunParameters.h"
#include "TVector3.h"
#include "VarDef.h"

class THaBeam : public THaApparatus, public THaBeamModule {
  
public:
  virtual ~THaBeam();
  
  virtual EStatus Init( const TDatime& run_time );

  virtual const TVector3& GetPosition()  const { return fPosition; }
  virtual const TVector3& GetDirection() const { return fDirection; }
  THaRunParameters*   GetRunParameters() const { return fRunParam; }

protected:

  virtual Int_t  DefineVariables( EMode mode = kDefine );
          void   Update();

  TVector3  fPosition;   // Beam position at the target (usually z=0) (meters)
  TVector3  fDirection;  // Beam direction vector (arbitrary units)

  THaRunParameters* fRunParam; // Pointer to parameters of current run

  THaBeam( const char* name, const char* description ) ;

  ClassDef(THaBeam,1)    // ABC for an apparatus providing beam information
};

#endif


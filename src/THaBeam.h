#ifndef ROOT_THaBeam
#define ROOT_THaBeam

//////////////////////////////////////////////////////////////////////////
//
// THaBeam
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include "TVector3.h"
#include "VarDef.h"

class THaBeam : public THaApparatus {
  
public:
  virtual ~THaBeam() {}
  
  virtual const TVector3& GetPosition()  const { return fPosition; }
  virtual const TVector3& GetDirection() const { return fDirection; }

protected:

  virtual Int_t  DefineVariables( EMode mode );

  TVector3  fPosition;   // Beam position at the target (usually z=0) (meters)
  TVector3  fDirection;  // Beam direction vector (arbitrary units)

  THaBeam( const char* name, const char* description ) ;

  ClassDef(THaBeam,1)    // ABC for an apparatus providing beam information
};

#endif


#ifndef ROOT_THaBeamDet
#define ROOT_THaBeamDet

//////////////////////////////////////////////////////////////////////////
//
// THaBeam
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"
#include "TVector3.h"

class THaBeamDet : public THaDetector {
  
public:
  virtual ~THaBeamDet();
  
  virtual TVector3 GetPosition()  const = 0;
  virtual TVector3 GetDirection() const = 0;
  virtual Int_t Process() = 0;

 protected:

  // Only derived classes can construct me
  //  THaBeamDet() {}
  THaBeamDet( const char* name, const char* description ="" ,
              THaApparatus* a = NULL ) :
    THaDetector( name, description , a) {}

 public:
  ClassDef(THaBeamDet,0)    // ABC for an detector providing beam information
};

#endif


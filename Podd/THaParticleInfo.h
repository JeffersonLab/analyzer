#ifndef Podd_THaParticleInfo_h_
#define Podd_THaParticleInfo_h_

//////////////////////////////////////////////////////////////////////////
//
// THaParticleInfo
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"

class THaParticleInfo : public TNamed {

public:
  THaParticleInfo();
  THaParticleInfo( const char* shortname, const char* name, 
		   Double_t mass, Int_t charge )
    : TNamed( shortname, name ), fMass(mass), fCharge(charge) {}
  THaParticleInfo( const THaParticleInfo& rhs );
  THaParticleInfo& operator=( const THaParticleInfo& rhs );
  virtual ~THaParticleInfo() {}

  Double_t          GetMass() const          { return fMass; }
  Double_t          GetMass2() const         { return fMass*fMass; }
  Double_t          GetCharge() const        { return fCharge; }
  virtual void      Print( Option_t* opt="" ) const;
  void              SetMass( Double_t mass ) { fMass = mass; }
  void              SetCharge( Int_t c )     { fCharge = c; }

private:
  Double_t          fMass;              //Particle mass
  Int_t             fCharge;            //Charge

  ClassDef(THaParticleInfo,1)  //Information for a particle type
};


#endif

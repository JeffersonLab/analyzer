#ifndef Podd_THaSAProtonEP_h_
#define Podd_THaSAProtonEP_h_

//////////////////////////////////////////////////////////////////////////
//
// THaSAProtonEP
//
//////////////////////////////////////////////////////////////////////////

#include "THaPrimaryKine.h"
#include "TLorentzVector.h"
#include "TString.h"

class THaSAProtonEP : public THaPrimaryKine {
  
public:
  THaSAProtonEP( const char* name, const char* description,
		   const char* spectro = "",
		   Double_t target_mass = -1.0 /* GeV/c2 */ );
  THaSAProtonEP( const char* name, const char* description,
		   const char* spectro, const char* beam,
		   Double_t target_mass = -1.0 /* GeV/c2 */ );
  virtual ~THaSAProtonEP();
  
  virtual Int_t     Process( const THaEvData& );

  ClassDef(THaSAProtonEP,0)   //Single arm proton kinematics
};

#endif

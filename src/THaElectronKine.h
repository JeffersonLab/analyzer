#ifndef Podd_THaElectronKine_h_
#define Podd_THaElectronKine_h_

//////////////////////////////////////////////////////////////////////////
//
// THaElectronKine
//
//////////////////////////////////////////////////////////////////////////

#include "THaPrimaryKine.h"

class THaElectronKine : public THaPrimaryKine {
  
public:
  THaElectronKine( const char* name, const char* description,
		   const char* spectro = "", Double_t mass = 0.0 /* GeV */ );
  THaElectronKine( const char* name, const char* description,
		   const char* spectro, const char* beam,
		   Double_t mass = 0.0 /* GeV */ );
  virtual ~THaElectronKine();
  
  ClassDef(THaElectronKine,0)   //Single arm kinematics module
};

#endif

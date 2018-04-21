#ifndef Podd_THaBeamModule_h_
#define Podd_THaBeamModule_h_

//////////////////////////////////////////////////////////////////////////
//
// THaBeamModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeamInfo.h"

struct RVarDef;

class THaBeamModule {

public:
  THaBeamModule();  // public for ROOT I/O
  virtual ~THaBeamModule();
  
  THaBeamInfo*  GetBeamInfo()  { return &fBeamIfo; }

  void BeamIfoClear() { fBeamIfo.Clear(); }
  static const RVarDef* GetRVarDef();

protected:

  THaBeamInfo  fBeamIfo;      // Beam information

  ClassDef(THaBeamModule,1)   // ABC for a beam module

};

#endif

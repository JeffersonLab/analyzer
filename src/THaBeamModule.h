#ifndef ROOT_THaBeamModule
#define ROOT_THaBeamModule

//////////////////////////////////////////////////////////////////////////
//
// THaBeamModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeamInfo.h"

struct RVarDef;

class THaBeamModule {

public:
  virtual ~THaBeamModule();
  
  THaBeamInfo*  GetBeamInfo()  { return &fBeamIfo; }

  void BeamIfoClear() { fBeamIfo.Clear(); }
  static const RVarDef* GetRVarDef();

protected:

  THaBeamInfo  fBeamIfo;      // Beam information

  THaBeamModule();

  ClassDef(THaBeamModule,0)   // ABC for a beam module

};

#endif

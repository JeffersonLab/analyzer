#ifndef Podd_THaBeamEloss_h_
#define Podd_THaBeamEloss_h_

//////////////////////////////////////////////////////////////////////////
//
// THaBeamEloss
//
//////////////////////////////////////////////////////////////////////////

#include "THaElossCorrection.h"
#include "THaBeamModule.h"

class THaBeamEloss : public THaElossCorrection, public THaBeamModule {
  
public:
  THaBeamEloss( const char* name, const char* description,
		const char* input_beam = "" );
  virtual ~THaBeamEloss();
  
  virtual void      Clear( Option_t* opt="" );

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );


protected:

  THaBeamModule*    fBeamModule; // Pointer to beam module

  // Function for updating fEloss based on input beamifo.
  virtual void      CalcEloss( THaBeamInfo* beamifo );

  // Setup functions
  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(THaBeamEloss,0)   //Beam energy loss correction module
};

#endif

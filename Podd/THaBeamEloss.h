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
  ~THaBeamEloss() override;

  void      Clear( Option_t* opt="" ) override;

  EStatus   Init( const TDatime& run_time ) override;
  Int_t     Process( const THaEvData& ) override;


protected:

  THaBeamModule*    fBeamModule; // Pointer to beam module

  // Function for updating fEloss based on input beamifo.
  virtual void      CalcEloss( THaBeamInfo* beamifo );

  // Setup functions
  Int_t DefineVariables( EMode mode = kDefine ) override;

  ClassDefOverride(THaBeamEloss,0)   //Beam energy loss correction module
};

#endif

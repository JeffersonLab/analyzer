#ifndef Podd_THaTrackEloss_h_
#define Podd_THaTrackEloss_h_

//////////////////////////////////////////////////////////////////////////
//
// THaTrackEloss
//
//////////////////////////////////////////////////////////////////////////

#include "THaElossCorrection.h"
#include "THaTrackingModule.h"

class THaTrackEloss : public THaElossCorrection, public THaTrackingModule {
  
public:
  THaTrackEloss( const char* name, const char* description,
		 const char* input_tracks = "", 
		 Double_t particle_mass = 0.511e-3 /* GeV/c^2 */,
		 Int_t hadron_charge = 1 );
  ~THaTrackEloss() override;

  void      Clear( Option_t* opt="" ) override;

  EStatus   Init( const TDatime& run_time ) override;
  Int_t     Process( const THaEvData& ) override;


protected:

  THaTrackingModule* fTrackModule; // Pointer to tracking module

  // Function for updating fEloss based on input trkifo.
  virtual void       CalcEloss( THaTrackInfo* trkifo );

  // Setup functions
  Int_t DefineVariables( EMode mode = kDefine ) override;

  ClassDefOverride(THaTrackEloss,0)   //Track energy loss correction module
};

#endif

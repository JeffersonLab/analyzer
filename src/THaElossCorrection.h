#ifndef ROOT_THaElossCorrection
#define ROOT_THaElossCorrection

//////////////////////////////////////////////////////////////////////////
//
// THaElossCorrection
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "THaTrackingModule.h"
#include "TString.h"

class THaElossCorrection : public THaPhysicsModule, public THaTrackingModule {
  
public:
  THaElossCorrection( const char* name, const char* description,
		      const char* input_tracks = "", 
		      Double_t particle_mass = 0.511e-3 /* GeV/c^2 */,
		      Int_t hadron_charge = 1 );
  virtual ~THaElossCorrection();
  
  virtual void      Clear( Option_t* opt="" );

  Double_t          GetMass()       const { return fM; }
  Double_t          GetEloss()      const { return fEloss; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );
          void      SetInputModule( const char* name );
          void      SetMass( Double_t m /* GeV/c^2 */ );
          void      SetTestMode( Bool_t enable=kTRUE,
				 Double_t eloss_value=0.0 /* GeV */ );
          void      SetMedium( Double_t Z, Double_t A,
			       Double_t density     /* g/cm^3 */,
			       Double_t pathlength  /* cm */ );

  static  Double_t  ElossElectron( Double_t beta, Double_t z_med,
				   Double_t a_med, Double_t d_med, 
				   Double_t pathlength );
  static  Double_t  ElossHadron( Int_t Z_hadron, Double_t beta, 
				 Double_t z_med, Double_t a_med, 
				 Double_t d_med, Double_t pathlength );

protected:

  // Event-by-event data  
  Double_t           fEloss;       // Energy loss correction (GeV)

  // Parameters
  Double_t           fM;           // Mass of particle (GeV/c^2)
  Int_t              fZ;           // Charge (Z) of particle (only used for hadrons)

  Double_t           fZmed;        // Effective Z of medium 
  Double_t           fAmed;        // Effective A of medium
  Double_t           fDensity;     // Density of medium (g/cm^3)
  Double_t           fPathlength;  // Pathlength through medium (cm)

  Bool_t             fTestMode;    // If true, use fixed value for fEloss
  Bool_t             fElectronMode;// Particle is electron or positron
  TString            fInputName;   // Name of spectrometer/tracking module
  THaTrackingModule* fTrackModule; // Pointer to tracking module

  // Function for updating fEloss based on input trkifo.
  virtual void  CalcEloss( THaTrackInfo* trkifo );

  // Setup functions
  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadRunDatabase( const TDatime& date );

private:
  // Energy loss library functions
  static Double_t ExEnerg( Double_t z_med, Double_t d_med );
  static void     HaDensi( Double_t z_med, Double_t d_med,
			   Double_t& X0, Double_t& X1, Double_t& M );

  ClassDef(THaElossCorrection,0)   //Track energy loss correction module
};

#endif

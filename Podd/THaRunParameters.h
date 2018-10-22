#ifndef Podd_THaRunParameters_h_
#define Podd_THaRunParameters_h_

//////////////////////////////////////////////////////////////////////////
//
// THaRunParameters
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TString.h"
#include "TArrayI.h"

class TDatime;

class THaRunParameters : public TObject {
public:
  THaRunParameters();
  // Derived classes must implement operator= and copy ctor!
  THaRunParameters( const THaRunParameters& run );
  virtual THaRunParameters& operator=( const THaRunParameters& rhs );
  virtual ~THaRunParameters();
  
  virtual void   Clear( Option_t* opt="" );
  const char*    GetBeamName()    const { return fBeamName.Data(); }
  Double_t       GetBeamE()       const { return fBeamE; }
  Double_t       GetBeamdE()      const { return fBeamdE; }
  Double_t       GetBeamM()       const { return fBeamM; }
  Double_t       GetBeamP()       const { return fBeamP; }
  Int_t          GetBeamQ()       const { return fBeamQ; }
  Double_t       GetBeamPol()     const { return fBeamPol; }
  const char*    GetTgtName()     const { return fTgtName.Data(); }
  Double_t       GetTgtM()        const { return fTgtM; }
  Double_t       GetTgtPol()      const { return fTgtPol; }
  const char*    GetRunName()     const { return fRunName; }

  virtual Bool_t IsFixedTarget()  const { return kTRUE; }
  Bool_t         IsPolzarized()   const { return fIsPol; }

  virtual void   Print( Option_t* opt="" ) const;
  virtual Int_t  ReadDatabase( const TDatime& date );

  void           SetBeam( Double_t E, Double_t M, Int_t Q, Double_t dE = 0.0 );
  void           SetTgtM( Double_t M )    { fTgtM = M; }
  void           SetPolarizations( Double_t pb, Double_t pt );

  void           SetBeamName( const char* name ) { fBeamName = name; }
  void           SetTgtName(  const char* name ) { fTgtName  = name; }
  void           SetRunName(  const char* name ) { fRunName  = name; }

  const TArrayI& GetPrescales() const { return fPrescale; }
  TArrayI&       Prescales()          { return fPrescale; }

protected:
  // Beam parameters
  TString       fBeamName;     // Description of beam particle
  Double_t      fBeamE;        // Total nominal beam energy (GeV)
  Double_t      fBeamP;        // Calculated beam momentum (GeV/c)
  Double_t      fBeamM;        // Rest mass of beam particles (GeV/c^2)
  Int_t         fBeamQ;        // Charge of beam particles (electron: -1)
  Double_t      fBeamdE;       // Beam energy uncertainty (GeV)
  Double_t      fBeamPol;      // Beam polarization

  // Target parameters
  TString       fTgtName;      // Description of target
  Double_t      fTgtM;         // Target rest mass (GeV/c^2)
  Double_t      fTgtPol;       // Target polarization
  Bool_t        fIsPol;        // Flag for beam and/or target polarized

  // DAQ parameters
  TString       fRunName;      // Description of run type
  TArrayI       fPrescale;     // Prescale factors

  ClassDef(THaRunParameters,1) // Beam & fixed target run parameters
};


#endif

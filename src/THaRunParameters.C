//*-- Author :    Ole Hansen   04-Nov-2003

//////////////////////////////////////////////////////////////////////////
//
// THaRunParameters
//
//////////////////////////////////////////////////////////////////////////

#include "THaRunParameters.h"
#include "THaAnalysisObject.h"
#include "TDatime.h"
#include "TMath.h"

using namespace std;

//_____________________________________________________________________________
THaRunParameters::THaRunParameters()
{
  // Default constructor

}

//_____________________________________________________________________________
THaRunParameters::THaRunParameters( const THaRunParameters& rhs ) :
  fBeamName(rhs.fBeamName), fBeamE(rhs.fBeamE), fBeamP(rhs.fBeamP),
  fBeamM(rhs.fBeamM), fBeamQ(rhs.fBeamQ), fBeamdE(rhs.fBeamdE), 
  fBeamPol(rhs.fBeamPol),
  fTgtName(rhs.fTgtName), fTgtM(rhs.fTgtM), fTgtPol(rhs.fTgtPol),
  fIsPol(rhs.fIsPol),
  fRunName(rhs.fRunName), fPrescale(rhs.fPrescale)
{
  // Copy ctor. Explicitly implemented to work around limitations of CINT
  // with derived classes.

}

//_____________________________________________________________________________
THaRunParameters& THaRunParameters::operator=(const THaRunParameters& rhs)
{
  // THaRunParameters assignment operator.

  if (this != &rhs) {
    fBeamName = rhs.fBeamName;
    fBeamE    = rhs.fBeamE; 
    fBeamP    = rhs.fBeamP;
    fBeamM    = rhs.fBeamM;
    fBeamQ    = rhs.fBeamQ; 
    fBeamdE   = rhs.fBeamdE; 
    fBeamPol  = rhs.fBeamPol;
    fTgtName  = rhs.fTgtName; 
    fTgtM     = rhs.fTgtM;
    fTgtPol   = rhs.fTgtPol;
    fIsPol    = rhs.fIsPol;
    fRunName  = rhs.fRunName;
    fPrescale = rhs.fPrescale;
  }
  return *this;
}

//_____________________________________________________________________________
THaRunParameters::~THaRunParameters()
{
  // Destructor
}

//_____________________________________________________________________________
void THaRunParameters::Print( Option_t* opt ) const
{
  // Print run parameters
}

//_____________________________________________________________________________
Int_t THaRunParameters::ReadDatabase( const TDatime& date )
{
  // Qeury the run database for the beam and target parameters for the 
  // date/time of this run.  The run date should have been set, otherwise
  // the time when the run object was created (usually shortly before the 
  // current time) will be used, which is often not meaningful.
  //
  // Return 0 if success, <0 if file error, >0 if not all required data found.

#define OPEN THaAnalysisObject::OpenFile
#define READ THaAnalysisObject::LoadDBvalue

  FILE* f = OPEN( "run", date, "THaRunParameters::ReadDatabase()" );
  if( !f ) 
    return -1;

  Int_t iq, st;
  Double_t E, M = 0.511e-3, Q = -1.0, dE = 0.0;

  if( (st = READ( f, date, "Ebeam", E )) ) 
    return st;                      // Beam energy required
  READ( f, date, "mbeam", M );
  READ( f, date, "qbeam", Q );
  READ( f, date, "dEbeam", dE );
  iq = int(Q);
  SetBeam( E, M, iq, dE );

  fclose(f);
  return 0;
}
  
//_____________________________________________________________________________
void THaRunParameters::SetBeam( Double_t E, Double_t M, Int_t Q, Double_t dE )
{
  // Set beam parameters.

  fBeamE = E;
  fBeamM = M;
  fBeamQ = Q;
  fBeamP = (E>M) ? TMath::Sqrt(E*E-M*M) : 0.0;
  if( fBeamP == 0.0 )
    Warning( "SetBeam()", "Beam momentum = 0 ??");
  fBeamdE = dE;
}
 
//_____________________________________________________________________________
void THaRunParameters::SetPolarizations( Double_t pb, Double_t pt )
{
  // Set beam and target polarization values (user-defined meaning)

  fBeamPol = pb;
  fTgtPol  = pt;
  fIsPol   = (TMath::Abs(pb*pt) > 1e-8);
    
}

//_____________________________________________________________________________
ClassImp(THaRunParameters)

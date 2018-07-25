//*-- Author :    Ole Hansen   04-Nov-2003

//////////////////////////////////////////////////////////////////////////
//
// THaRunParameters
//
//////////////////////////////////////////////////////////////////////////

#include "THaRunParameters.h"
#include "THaAnalysisObject.h"
#include "TDatime.h"
#include "TError.h"
#include "TMath.h"
#include "THaEvData.h"
#include <iostream>

using namespace std;

//_____________________________________________________________________________
THaRunParameters::THaRunParameters() :
  fBeamE(0), fBeamP(0), fBeamM(0), fBeamQ(0), fBeamdE(0), fBeamPol(0),
  fTgtM(0), fTgtPol(0), fIsPol(kFALSE)
{
  // Default constructor

  fPrescale.Set(THaEvData::MAX_PSFACT);
  for( int i=0; i<fPrescale.GetSize(); i++)
    fPrescale[i] = -1;
}

//_____________________________________________________________________________
THaRunParameters::THaRunParameters( const THaRunParameters& rhs ) :
  TObject(rhs),
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
void THaRunParameters::Clear( Option_t* )
{
  // Clear run parameters
  fBeamName = fTgtName = fRunName = "";
  fBeamE = fBeamP = fBeamM = fBeamdE = fBeamPol = fTgtM = fTgtPol = 0;
  fBeamQ = 0;
  fIsPol = kFALSE;
  fPrescale.Reset();
}

//_____________________________________________________________________________
void THaRunParameters::Print( Option_t* ) const
{
  // Print run parameters

  cout << "Run parameters: " << fRunName << endl;
  cout << " Beam: " << fBeamName <<endl;
  cout << "  Energy      = " << fBeamE << " +/- " << fBeamdE << " GeV\n";
  cout << "  Momentum    = " << fBeamP << " GeV/c\n";
  cout << "  Mass/Charge = " << fBeamM << "/" << fBeamQ << " GeV/c^2\n";
  if( fIsPol ) {
    cout << "  Polarization= " << 100.*fBeamPol << "%\n";
  }
  cout << " Target: " << fTgtName << endl;
  cout << "  Mass:       = " << fTgtM << " GeV/c^2\n";
  if( fIsPol ) {
    cout << "  Polarization= " << 100.*fTgtPol << "%\n";
  }
  cout << " DAQ:\n";
  Int_t np = fPrescale.GetSize();
  if( np > 0 ){
    cout << "  Prescale factors: (1-"<<np<<")\n   ";
    for( int i=0; i<np; i++ ) {
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,3,0)
      cout << fPrescale[i];
#else
      // const version of operator[] missing in ROOT < 3.3
      cout << const_cast<THaRunParameters*>(this)->fPrescale[i];
#endif
      if( i != np-1 )
	cout << "/";
    }
    cout << endl;
  }
}

//_____________________________________________________________________________
Int_t THaRunParameters::ReadDatabase( const TDatime& date )
{
  // Query the run database for the beam and target parameters at the 
  // specified date/time.
  //
  // Return 0 if success, <0 if file error, >0 if not all required data found.

#define OPEN THaAnalysisObject::OpenFile
#define LOAD THaAnalysisObject::LoadDB

  FILE* f = OPEN( "run", date, "THaRunParameters::ReadDatabase", "r", 1 );
  if( !f ) 
    return -1;

  Double_t E, M = 0.511e-3, Q = -1.0, dE = 0.0;

  DBRequest request[] = {
    { "ebeam",  &E },
    { "mbeam",  &M,  kDouble, 0, 1 },
    { "qbeam",  &Q,  kDouble, 0, 1 },
    { "dEbeam", &dE, kDouble, 0, 1 },
    { 0 }
  };
  Int_t err = LOAD( f, date, request, "" );
  fclose(f);
  if( err )
    return err;

  Int_t iq = int(Q);
  SetBeam( E, M, iq, dE );

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

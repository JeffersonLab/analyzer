//*-- Author:   LEDEX collaboration   June 2006

// THaSAProtonEP
// 
// This module is calculates the kinematics for elastic X(e,X)e'
// (and only that reaction!!!) when only the outbound X is detected.
//
// By default X = proton. For heavier targets, use SetMass() or
// giv ethe target mass explicitly in the constructor.

#include "THaSAProtonEP.h"
#include "THaRunBase.h"
#include "THaTrackingModule.h"
#include "THaBeam.h"
#include "TMath.h"

using namespace std;

// Assume ep by default
static const Double_t electron_mass = 0.511e-3;
static const Double_t Mp = 0.938272;

//_____________________________________________________________________________
THaSAProtonEP::THaSAProtonEP( const char* name, const char* description,
				  const char* spectro,
				  Double_t target_mass ) 
  : THaPrimaryKine(name,description,spectro,electron_mass,
		   (target_mass > 0.0) ? target_mass : Mp )
{
  // Standard constructor. Assumes ideal beam along z_lab.
}


//_____________________________________________________________________________
THaSAProtonEP::THaSAProtonEP( const char* name, const char* description,
				  const char* spectro, const char* beam,
				  Double_t target_mass ) 
  : THaPrimaryKine(name,description,spectro,beam,
		   (target_mass > 0.0) ? target_mass : Mp )
{
  // Constructor with specification of optional beam module.

  SetMass( electron_mass );
}

//_____________________________________________________________________________
THaSAProtonEP::~THaSAProtonEP()
{
  // Destructor

}

//_____________________________________________________________________________
Int_t THaSAProtonEP::Process( const THaEvData& )
{
  // Calculate the electron kinematics for elastic eX -> eX using the 
  // 4-vector from the outgoing X.

  if( !IsOK() || !gHaRun ) return -1;

  THaTrackInfo* trkifo = fSpectro->GetTrackInfo();
  if( !trkifo || !trkifo->IsOK() ) return 1;

  // 4-momentum of incident particle. If a beam module given
  // then use it, otherwise assume beam along z_lab
  if( fBeam ) {
    fP0.SetVectM( fBeam->GetBeamInfo()->GetPvect(), fM );
  } else {
    Double_t p_in  = gHaRun->GetParameters()->GetBeamP();
    fP0.SetXYZM( 0.0, 0.0, p_in, fM );
  }

  // Assume target at rest
  fA.SetXYZM( 0.0, 0.0, 0.0, fMA );

  //Only for elastic ep
  fA1.SetVectM( trkifo->GetPvect(), fMA );
  fQ  = fA1 - fA;
  fP1 = fP0 - fQ;

  // Reconstructed electron kinematics
  fQ2        = -fQ.M2();
  fQ3mag     = fQ.P();
  fOmega     = fQ.E();
  fW2        = fA1.M2();  // = fMA
  fScatAngle = fP0.Angle( fP1.Vect() );
  fEpsilon   = 1.0 / ( 1.0 + 2.0*fQ3mag*fQ3mag/fQ2*
		       pow( tan(fScatAngle/2.0), 2.0 ));
  fThetaQ    = fQ.Theta();
  fPhiQ      = fQ.Phi();
  fXbj       = fQ2/(2.0*Mp*fOmega);

  fDataValid = true;
  return 0;
}


ClassImp(THaSAProtonEP)


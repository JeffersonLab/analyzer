//*-- Author :    Ole Hansen   5 April 2001

//////////////////////////////////////////////////////////////////////////
//
// THaPIDinfo
//
// Implements a basic Baysian likelihood analysis.
//
// One can define an arbitrary number of detectors (= measurements) and
// particle types (hypotheses). Each detector is expected to calculate
// the probability that the event or track to which this object is
// attached corresponds to each of the particle types.
//
// After the per-detector/per-particle type probabilities are set,
// CombinePID() calculates the overall conditional (posterior)
// probabilities for each particle hypothesis.
//
// For accurate results, prior probabilities (prevalences) should be set
// for each particle type via SetPrior(). Such numbers will typically
// come from simulations. By default, all prior probabilities are assumed
// equal (=no prior knowledge).
//
// Prior probabilities may be different for each event or track,
// especially for large-acceptance detectors, as they usually depend on
// the reaction kinematics and thus on track 4-momentum. For small
// acceptance spectrometers, one may get away with constant values
// (averages) calculated for a given spectrometer setting (angle,
// momentum).
//
//////////////////////////////////////////////////////////////////////////

#include "THaPIDinfo.h"
#include "THaTrack.h"
#include "THaSpectrometer.h"
#include "THaTrackingDetector.h"
#include <iostream>

using namespace std;

//_____________________________________________________________________________
THaPIDinfo::THaPIDinfo() : fNdet{0}, fNpart{0}
{
  // Default constructor
}

//_____________________________________________________________________________
THaPIDinfo::THaPIDinfo( UInt_t ndet, UInt_t npart ) :
  fPrior(npart), fProb(ndet*npart), fCombinedProb(npart),
  fNdet{ndet}, fNpart{npart}
{
  // Normal constructor
  SetDefaultPriors();
}

//_____________________________________________________________________________
THaPIDinfo::THaPIDinfo( const THaTrack* track ) : fNdet{0}, fNpart{0}
{
  // Normal constructor from a track. Retrieves array dimensions from
  // the size of the detector and particle arrays of the track's spectrometer.

  if( track ) {
    if( const auto* td = track->GetCreator() ) {
      if( const auto* sp = dynamic_cast<THaSpectrometer*>(td->GetApparatus()) ) {
        UInt_t ndet = sp->GetNpidDetectors();
        UInt_t npart = sp->GetNpidParticles();
        SetSize(ndet, npart);
      }
    }
  }
}

//_____________________________________________________________________________
void THaPIDinfo::Clear( Option_t* )
{
  // Zero contents of the arrays

  fProb.assign(fProb.size(), 0.0);
  fCombinedProb.assign(fCombinedProb.size(), 0.0);
}

//_____________________________________________________________________________
void THaPIDinfo::CombinePID()
{
  // Compute combined PID of all detectors

  // Likelihood products per particle hypothesis across all detectors
  for( UInt_t p = 0; p < fNpart; p++ ) {
    fCombinedProb[p] = 1.0;
    for( UInt_t d = 0; d < fNdet; d++ ) {
      fCombinedProb[p] *= fProb[idx(d, p)];
    }
  }

  // Weigh above results with the priors to get the conditional
  // probabilities for each particle hypothesis
  Double_t sum = 0.0;
  for( UInt_t p = 0; p < fNpart; p++ ) {
    sum += fPrior[p] * fCombinedProb[p];
  }
  if( sum != 0.0 ) {
    for( UInt_t p = 0; p < fNpart; p++ ) {
      fCombinedProb[p] *= fPrior[p] / sum;
    }
  }

}

//_____________________________________________________________________________
void THaPIDinfo::Print( Option_t* ) const
{
  // Display contents of arrays
  // The first line shows the combined probabilities, followed by the
  // probabilities for each detector
  for( UInt_t d = 0; d <= fNdet; d++ ) {
    for( UInt_t p = 0; p < fNpart; p++ ) {
      if( d == 0 )
	cout << fCombinedProb[p];
      else
        cout << fProb[idx(d-1,p)];
      if( p != fNpart-1 ) cout << "   ";
    }
    cout << endl;
  }
}

//_____________________________________________________________________________
void THaPIDinfo::SetDefaultPriors()
{
  // Set prior probabilities equal to 1/fNpart (= no prior knowledge).

  for( UInt_t p = 0; p < fNpart; p++ ) {
    fPrior[p] = 1.0/fNpart;
  }
}

//_____________________________________________________________________________
void THaPIDinfo::SetSize( UInt_t ndet, UInt_t npart )
{
  // Resize the arrays. Creates new arrays if they don't yet exist,
  // Old contents are preserved as much as possible.

  if( ndet == fNdet && npart == fNpart ) return;
  if( ndet == 0 || npart == 0 ) {
    fPrior.clear();
    fProb.clear();
    fCombinedProb.clear();
    fNdet = fNpart = 0;
    return;
  }
  if( ndet != fNdet || npart != fNpart )
    fProb.resize(ndet * npart);

  if( npart != fNpart ) {
    fPrior.resize(npart);
    fCombinedProb.resize(npart);
    if( fNpart == 0 )
      SetDefaultPriors();
  }

  fNdet  = ndet;
  fNpart = npart;
}

//_____________________________________________________________________________
ClassImp(THaPIDinfo)

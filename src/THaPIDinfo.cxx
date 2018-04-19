//*-- Author :    Ole Hansen   5 April 2001

//////////////////////////////////////////////////////////////////////////
//
// THaPIDinfo
//
//////////////////////////////////////////////////////////////////////////

#include "THaPIDinfo.h"
#include "THaTrack.h"
#include "THaSpectrometer.h"
#include "THaTrackingDetector.h"

#include <iostream>

using namespace std;


//_____________________________________________________________________________
THaPIDinfo::THaPIDinfo() : fProb(0), fCombinedProb(0), fNdet(0), fNpart(0)
{
  // Default constructor
}

//_____________________________________________________________________________
THaPIDinfo::THaPIDinfo( UInt_t ndet, UInt_t npart ) : fNdet(ndet), fNpart(npart)
{
  // Normal constructor

  SetupArrays( ndet, npart );
}

//_____________________________________________________________________________
THaPIDinfo::THaPIDinfo( const THaTrack* track )
{
  // Normal constructor from a track. Retrieves array dimensions from
  // the size of the detector and particle arrays of the track's spectrometer.

  const THaSpectrometer* sp;
  const THaTrackingDetector* td;
  if( track && (td = track->GetCreator()) &&
      (sp = static_cast<THaSpectrometer*>(td->GetApparatus())) ) {
    fNdet  = sp->GetNpidDetectors();
    fNpart = sp->GetNpidParticles();
    SetupArrays( fNdet, fNpart );
  } else {
    fNdet = fNpart = 0;
    fProb = fCombinedProb = 0;
  }
}

//_____________________________________________________________________________
THaPIDinfo::~THaPIDinfo()
{
  // Destructor

  delete fCombinedProb;
  delete fProb;
}

//_____________________________________________________________________________
void THaPIDinfo::Clear( Option_t* )
{
  // Zero contents of the arrays

  if( fProb )          fProb->Reset();
  if( fCombinedProb )  fCombinedProb->Reset();
}

//_____________________________________________________________________________
void THaPIDinfo::CombinePID()
{
  // Compute combined PID for all detectors
  // FIXME: Check normalization!

  for( UInt_t p = 0; p < fNpart; p++ ) {
    (*fCombinedProb)[p] = 1.0;
    for( UInt_t d = 0; d < fNdet; d++ ) {
      (*fCombinedProb)[p] *= (*fProb)[idx(d,p)];
    }
  }
}
    
//_____________________________________________________________________________
void THaPIDinfo::Print( Option_t* ) const
{
  // Display contents of arrays

  for( UInt_t d = 0; d < fNdet; d++ ) {
    for( UInt_t p = 0; p < fNpart; p++ ) {
      if( d == 0 )
	cout << (*fCombinedProb)[p];
      else
	cout << (*fProb)[ idx(d-1,p) ];
      if( p != fNpart-1 ) cout << "   ";
    }
    cout << endl;
  }
}

//_____________________________________________________________________________
void THaPIDinfo::SetupArrays( UInt_t ndet, UInt_t npart )
{
  // Dimension arrays. Protected function used by constructors.

  if( ndet > 0 && npart > 0 ) {
    fProb         = new TArrayD( ndet*npart );
    fCombinedProb = new TArrayD( npart );
  } else {
    fProb = fCombinedProb = 0;
    fNdet = fNpart = 0;
  }
}

//_____________________________________________________________________________
void THaPIDinfo::SetSize( UInt_t ndet, UInt_t npart )
{
  // Resize the arrays. Creates new arrays if they don't yet exist,
  // Old contents are preserved as much as possible.

  if( ndet == fNdet && npart == fNpart ) return;
  if( ndet == 0 || npart == 0 ) {
    delete fProb;
    delete fCombinedProb;
    fProb = fCombinedProb = 0;
    fNdet = fNpart = 0;
    return;
  }
  if( fProb ) {
    if( ndet != fNdet || npart != fNpart ) {
      fProb->Set( ndet*npart );
    }
  } else
    fProb = new TArrayD( ndet*npart );

  if( fCombinedProb ) {
    if( npart != fNpart ) fCombinedProb->Set( npart );
  } else
    fCombinedProb = new TArrayD( npart );

  fNdet  = ndet;
  fNpart = npart;
}  

//_____________________________________________________________________________
ClassImp(THaPIDinfo)

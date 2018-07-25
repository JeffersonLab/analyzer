#ifndef Podd_THaPIDinfo_h_
#define Podd_THaPIDinfo_h_

//////////////////////////////////////////////////////////////////////////
//
// THaPIDinfo
//
// Particle ID information.  Usually associated with a THaTrack.
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TArrayD.h"

class THaTrack;

class THaPIDinfo : public TObject {

public:

  THaPIDinfo();
  THaPIDinfo( UInt_t ndet, UInt_t npart );
  THaPIDinfo( const THaTrack* track );
  virtual ~THaPIDinfo();

  virtual void      Clear( Option_t* opt="" );
  virtual void      CombinePID();
          Double_t  GetProb( UInt_t detector, UInt_t particle ) const;
          Double_t  GetCombinedProb( UInt_t particle ) const;
  virtual void      Print( Option_t* opt="" ) const;
          void      SetProb( UInt_t detector, UInt_t particle, Double_t prob );
          void      SetSize( UInt_t ndet, UInt_t prob );

protected:

  TArrayD*          fProb;          //Array of particle probabilities
  TArrayD*          fCombinedProb;  //Probabilities from each detector combined
  UInt_t            fNdet;          //Number of detectors
  UInt_t            fNpart;         //Number of particles

  UInt_t            idx( UInt_t detector, UInt_t particle ) const;
  void              SetupArrays( UInt_t ndet, UInt_t npart );

  ClassDef(THaPIDinfo,0)  //Particle ID information for a track
};

//---------------- inlines ----------------------------------------------------

inline UInt_t THaPIDinfo::idx( UInt_t det, UInt_t part ) const
{
  // Return index into array (2d array realized as 1d)

  return det + part*fNdet;
}

//_____________________________________________________________________________
inline Double_t THaPIDinfo::GetProb( UInt_t det, UInt_t part ) const
{
  return fProb ? (*fProb)[ idx(det,part) ] : 0.0;
}

//_____________________________________________________________________________
inline Double_t THaPIDinfo::GetCombinedProb( UInt_t part ) const
{
  return fCombinedProb ? (*fCombinedProb)[ part ] : 0.0;
}

//_____________________________________________________________________________
inline void THaPIDinfo::SetProb( UInt_t det, UInt_t part, Double_t prob )
{
  if( fProb ) (*fProb)[ idx(det,part) ] = prob;
}

#endif


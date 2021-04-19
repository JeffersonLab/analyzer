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
#include <vector>
#include <stdexcept>

class THaTrack;

class THaPIDinfo : public TObject {
public:
  THaPIDinfo();
  THaPIDinfo( UInt_t ndet, UInt_t npart );
  explicit THaPIDinfo( const THaTrack* track );
  virtual ~THaPIDinfo() = default;

  virtual void      Clear( Option_t* opt="" );
  virtual void      CombinePID();
          UInt_t    GetNdet()  const { return fNdet; }
          UInt_t    GetNpart() const { return fNpart; }
          Double_t  GetPrior( UInt_t particle ) const;
          Double_t  GetProb( UInt_t detector, UInt_t particle ) const;
          Double_t  GetCombinedProb( UInt_t particle ) const;
  virtual void      Print( Option_t* opt="" ) const;
          void      SetDefaultPriors();
          void      SetPrior( UInt_t particle, Double_t prob );
          void      SetProb( UInt_t detector, UInt_t particle, Double_t prob );
          void      SetSize( UInt_t ndet, UInt_t prob );

protected:
  // A priori particle probabilities
  std::vector<Double_t> fPrior;
  // Probabilities per detector & particle hypothesis
  std::vector<Double_t> fProb;
  // Probabilities for the particle hypotheses
  std::vector<Double_t> fCombinedProb;
  // Number of detectors
  UInt_t                fNdet;
  // Number of particles
  UInt_t                fNpart;

  UInt_t            idx( UInt_t detector, UInt_t particle ) const;

  ClassDef(THaPIDinfo,1)  //Particle ID information for a track
};

//---------------- inlines ----------------------------------------------------
inline UInt_t THaPIDinfo::idx( UInt_t det, UInt_t part ) const
{
  // Return index into array (2d array realized as 1d)

  return det*fNpart + part;
}

//_____________________________________________________________________________
inline Double_t THaPIDinfo::GetPrior( UInt_t part ) const
{
  if( part >= fNpart )
    throw std::logic_error("illegal particle index");
  return fPrior[part];
}

//_____________________________________________________________________________
inline Double_t THaPIDinfo::GetProb( UInt_t det, UInt_t part ) const
{
  if( det >= fNdet || part >= fNpart )
    throw std::logic_error("illegal detector or particle index");
  return fProb[idx(det, part)];
}

//_____________________________________________________________________________
inline Double_t THaPIDinfo::GetCombinedProb( UInt_t part ) const
{
  if( part >= fNpart )
    throw std::logic_error("illegal particle index");
  return fCombinedProb[part];
}

//_____________________________________________________________________________
inline void THaPIDinfo::SetPrior( UInt_t part, Double_t prob )
{
  if( part >= fNpart )
    throw std::logic_error("illegal particle index");
  fPrior[part] = prob;
}

//_____________________________________________________________________________
inline void THaPIDinfo::SetProb( UInt_t det, UInt_t part, Double_t prob )
{
  if( det >= fNdet || part >= fNpart )
    throw std::logic_error("illegal detector or particle index");
  fProb[idx(det, part)] = prob;
}

#endif


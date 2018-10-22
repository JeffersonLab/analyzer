///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCCluster                                                             //
//                                                                           //
// A group of VDC hits and routines for linear and non-linear fitting of     //
// drift distances.                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCCluster.h"
#include "THaVDCHit.h"
#include "THaVDCPlane.h"
#include "THaTrack.h"
#include "TMath.h"
#include "TClass.h"

#include <iostream>

using namespace VDC;
using namespace std;

const Double_t VDC::kBig = 1e38;  // Arbitrary large value
static const Int_t kDefaultNHit = 16;

//_____________________________________________________________________________
THaVDCCluster::THaVDCCluster( THaVDCPlane* owner )
  : fPlane(owner), fPointPair(0), fTrack(0), fTrkNum(0),
    fSlope(kBig), fLocalSlope(kBig), fSigmaSlope(kBig),
    fInt(kBig), fSigmaInt(kBig), fT0(kBig), fSigmaT0(kBig),
    fPivot(0), fTimeCorrection(0),
    fFitOK(false), fChi2(kBig), fNDoF(0.0), fClsBeg(kMaxInt), fClsEnd(-1)
{
  // Constructor

  fHits.reserve(kDefaultNHit);
  fCoord.reserve(kDefaultNHit);
}

//_____________________________________________________________________________
void THaVDCCluster::AddHit( THaVDCHit* hit )
{
  // Add a hit to the cluster

  assert( hit );

  fHits.push_back( hit );
  if( fClsBeg > hit->GetWireNum() )
    fClsBeg = hit->GetWireNum();
  if( fClsEnd < hit->GetWireNum() )
    fClsEnd = hit->GetWireNum();
}

//_____________________________________________________________________________
void THaVDCCluster::Clear( Option_t* )
{
  // Clear the contents of the cluster and reset status

  ClearFit();
  fHits.clear();
  fPivot   = 0;
  fPlane   = 0;
  fPointPair = 0;
  fTrack   = 0;
  fTrkNum  = 0;
  fClsBeg  = kMaxInt-1;
  fClsEnd  = -1;
}

//_____________________________________________________________________________
void THaVDCCluster::ClearFit()
{
  // Clear fit results only

  fSlope      = kBig;
  fLocalSlope = kBig;
  fSigmaSlope = kBig;
  fInt        = kBig;
  fSigmaInt   = kBig;
  fT0         = kBig;
  fSigmaT0    = kBig;
  fFitOK      = false;
  fLocalSlope = kBig;
  fChi2       = kBig;
  fNDoF       = 0.0;
}

//_____________________________________________________________________________
Int_t THaVDCCluster::Compare( const TObject* obj ) const
{
  // Compare this cluster to another via the wire number of the pivot.
  // Returns -1 if comparison cannot be made (unlike class, no pivot).

  if( !obj || IsA() != obj->IsA() )
    return -1;

  const THaVDCCluster* rhs = static_cast<const THaVDCCluster*>( obj );
  if( GetPivotWireNum() < rhs->GetPivotWireNum() )
    return -1;
  if( GetPivotWireNum() > rhs->GetPivotWireNum() )
    return +1;

  return 0;
}

//_____________________________________________________________________________
void THaVDCCluster::EstTrackParameters()
{
  // Estimate Track Parameters
  // Calculates pivot wire and uses its position as position of cluster
  // Estimates the slope based on the distance between the first and last
  // wires to be hit in the U (or V) and detector Z directions

  fFitOK = false;
  if( GetSize() == 0 )
    return;

  // Find pivot
  Double_t time = 0, minTime = 1000;  // drift-times in seconds
  for (int i = 0; i < GetSize(); i++) {
    time = fHits[i]->GetTime();
    if (time < minTime) { // look for lowest time
      minTime = time;
      fPivot = fHits[i];
    }
  }

  // Now set intercept
  fInt = fPivot->GetPos();

  // Now find the approximate slope
  //   X = Drift Distance (m)
  //   Y = Position of Wires (m)
  if( GetSize() > 1 ) {
    Double_t conv = fPlane->GetDriftVel();  // m/s
    Double_t dx = conv * (fHits[0]->GetTime() + fHits[GetSize()-1]->GetTime());
    Double_t dy = fHits[0]->GetPos() - fHits[GetSize()-1]->GetPos();
    fSlope = dy / dx;
  } else
    fSlope = 1.0;

  fFitOK = true;
}

//_____________________________________________________________________________
void THaVDCCluster::ConvertTimeToDist()
{
  // Convert TDC Times in wires to drift distances

  //Do conversion for each hit in cluster
  for (int i = 0; i < GetSize(); i++)
    fHits[i]->ConvertTimeToDist(fSlope);
}

//_____________________________________________________________________________
chi2_t THaVDCCluster::CalcDist()
{
  // Calculate and store the distance of the global track to the wires.
  // We can then inspect the quality of the fits.
  //
  // This is actually a bit tedious: we need to project the track
  // (given by its detector coordinates) onto this cluster's plane (u or v),
  // then find the track's z coordinate for each active wire's position.
  // It takes two pages of vector algebra.
  //
  // Return value: sum of squares of distances and number of hits used

  assert( fTrack );

  Int_t npt  = 0;
  Double_t chi2 = 0;
  for (int j = 0; j < GetSize(); j++) {
    // Use only hits marked to belong to this track
    if( fHits[j]->GetTrkNum() != fTrack->GetTrkNum() )
      continue;
    Double_t u     = fHits[j]->GetPos();    // u (v) coordinate of wire
    Double_t sina  = fPlane->GetSinWAngle();
    Double_t cosa  = fPlane->GetCosWAngle();
    Double_t denom = cosa*fTrack->GetDTheta() + sina*fTrack->GetDPhi();
    Double_t z     = (u - cosa*fTrack->GetDX() - sina*fTrack->GetDY()) / denom;
    Double_t dz    = z - fPlane->GetZ();
    fHits[j]->SetFitDist(TMath::Abs(dz));
    chi2 += dz*dz;
    npt++;
  }
  return make_pair( chi2, npt );
}

//_____________________________________________________________________________
void THaVDCCluster::CalcLocalDist()
{
  //FIXME: clean this up - duplicate of chi2 calculation
  // Calculate and store the distance of the local fitted track to the wires.
  // We can then inspect the quality of the local fits
  for (int j = 0; j < GetSize(); j++) {
    Double_t y = fHits[j]->GetPos();
    if (fLocalSlope != 0. && fLocalSlope < kBig) {
      Double_t X = (y-fInt)/fLocalSlope;
      fHits[j]->SetLocalFitDist(TMath::Abs(X));
    } else {
      fHits[j]->SetLocalFitDist(kBig);
    }
  }
}

//_____________________________________________________________________________
Int_t THaVDCCluster::GetTrackIndex() const
{
  // Return index of assigned track (-1 = none, 0 = first/best, etc.)

  if( !fTrack ) return -1;
  return fTrack->GetIndex();
}

//_____________________________________________________________________________
void THaVDCCluster::FitTrack( EMode mode )
{
  // Fit track to drift distances. Supports three modes:
  //
  // kSimple:    Linear fit, ignore t0 and multihits
  // kWeighted:  Linear fit with weights, ignore t0
  // kT0:        Fit t0, but ignore mulithits

  switch( mode ) {
  case kSimple:
    FitSimpleTrack(false);
    break;
  case kWeighted:
    FitSimpleTrack(true);
    break;
  case kT0:
    LinearClusterFitWithT0();
    break;
  }
  CalcLocalDist();
}

//_____________________________________________________________________________
void THaVDCCluster::FitSimpleTrack( Bool_t weighted )
{
  // Perform linear fit on drift times. Calculates slope, intercept, and errors.
  // Does not assume the uncertainty is the same for all hits.
  //
  // Assume t0 = 0.

  // For this function, the final results is (m,b) in  Y = m X + b
  //   X = Drift Distance
  //   Y = Position of Wires

  fFitOK = false;
  if( GetSize() < 3 ) {
    return;  // Too few hits to get meaningful results
	     // Do keep current values of slope and intercept
  }

  Double_t m, sigmaM;  // Slope, St. Dev. in slope
  Double_t b, sigmaB;  // Intercept, St. Dev in Intercept

  fCoord.clear();

  Double_t bestFit = 0.0;

  // Find the index of the pivot wire and copy distances into local arrays
  // Note that as the index of the hits is increasing, the position of the
  // wires is decreasing

  Int_t pivotNum = 0;
  for (int i = 0; i < GetSize(); i++) {

    // In order to take into account the varying uncertainty in the
    // drift distance, we will be working with the X' and Y', and
    //       Y' = F + G X'
    //  where Y' = X, and X' = Y  (swapping it around)
    if (fHits[i] == fPivot) {
      pivotNum = i;
    }

    Double_t x = fHits[i]->GetPos();
    Double_t y = fHits[i]->GetDist() + fTimeCorrection;
    Double_t w;
    if( weighted ) {
      w = fHits[i]->GetdDist();
      // the hit will be ignored if the uncertainty is < 0
      if (w>0)
	w = 1./(w*w); // sigma^-2 is the weight
      else
	w = -1.;
    } else {
      w = 1.0;
    }
    fCoord.push_back( FitCoord_t(x,y,w) );
  }

  const Int_t nSignCombos = 2; //Number of different sign combinations
  for (int i = 0; i < nSignCombos; i++) {
    Double_t F, sigmaF2;  // intermediate slope and St. Dev.**2
    Double_t G, sigmaG2;  // intermediate intercept and St. Dev.**2
    Double_t sigmaFG;     // correlated uncertainty

    Double_t sumX  = 0.0;   //Positions
    Double_t sumXW = 0.0;
    Double_t sumXX = 0.0;
    Double_t sumY  = 0.0;   //Drift distances
    Double_t sumXY = 0.0;
    Double_t sumYY = 0.0;
    Double_t sumXXW= 0.0;

    Double_t W = 0.0;
    Double_t WW= 0.0;

    if (i == 0)
      for (int j = pivotNum+1; j < GetSize(); j++)
	fCoord[j].y *= -1;
    else if (i == 1)
      fCoord[pivotNum].y *= -1;

    for (int j = 0; j < GetSize(); j++) {
      Double_t x = fCoord[j].x;   // Position of wire
      Double_t y = fCoord[j].y;   // Distance to wire
      Double_t w = fCoord[j].w;

      if (w <= 0) continue;
      W     += w;
      WW    += w*w;
      sumX  += x * w;
      sumXW += x * w * w;
      sumXX += x * x * w;
      sumXXW+= x * x * w * w;
      sumY  += y * w;
      sumXY += x * y * w;
      sumYY += y * y * w;
    }

    // Standard formulae for linear regression (see Bevington)
    Double_t Delta = W * sumXX - sumX * sumX;

    F  = (sumXX * sumY - sumX * sumXY) / Delta;
    G  = (W * sumXY - sumX * sumY) / Delta;
    sigmaF2 = ( sumXX / Delta );
    sigmaG2 = ( W / Delta );
    sigmaFG = ( -sumX / Delta );

    // calculate chi2 for the track given this slope and intercept
    chi2_t chi2 = CalcChisquare( G, F, 0 );

    m  =   1/G;
    b  = - F/G;

    sigmaM = m * m * TMath::Sqrt( sigmaG2 );
    sigmaB = TMath::Sqrt( sigmaF2 + F*F/(G*G)*sigmaG2 - 2*F/G*sigmaFG ) /
      TMath::Abs(G);

    // scale the uncertainty of the fit parameters based upon the
    // quality of the fit. This really should not be necessary if
    // we believe the drift-distance uncertainties
    // sigmaM *= chi2/(nhits - 2);
    // sigmaB *= chi2/(nhits - 2);

    // Pick the better value
    if (i == 0 || chi2.first < bestFit) {
      bestFit     = fChi2 = chi2.first;
      fNDoF       = chi2.second - 2;
      fLocalSlope = m;
      fInt        = b;
      fSigmaSlope = sigmaM;
      fSigmaInt   = sigmaB;
      fT0         = 0.0;
    }
  }

  fFitOK = true;
}

//_____________________________________________________________________________
Int_t THaVDCCluster::LinearClusterFitWithT0()
{
  // Perform linear fit on drift times. Calculates slope, intercept, time
  // offset t0, and errors.
  // Accounts for different uncertainties of each drift distance.
  //
  // Fits the parameters m, b, and d0 in
  //
  // s_i ( d_i + d0 ) = m x_i + b
  //
  // where
  //  s_i   = sign of ith drift distance (+/- 1)
  //  d_i   = ith measured drift distance
  //  x_i   = ith wire position
  //
  // The sign vector, s_i, is automatically optimized to yield the smallest
  // chi^2. Normally, s_i = -1 for i < i_pivot, and s_i = 1 for i >= i_pivot,
  // but this may occasionally not be the best choice in the presence of
  // a significant negative offset d0.
  //
  // The results, m, b, and d0, are converted to the "slope" and "intercept"
  // of the cluster geometery, where slope = 1/m and intercept = -b/m.
  // d0 is simply converted to time units to give t0, using the asymptotic
  // drift velocity.


  fFitOK = false;
  if( GetSize() < 4 ) {
    return -1;  // Too few hits to get meaningful results
		// Do keep current values of slope and intercept
  }

  Double_t m, sigmaM;   // Slope, St. Dev. in slope
  Double_t b, sigmaB;   // Intercept, St. Dev in Intercept
  Double_t d0, sigmaD0;

  sigmaM = sigmaB = sigmaD0 = 0;

  fCoord.clear();

  //--- Copy hit data into local arrays
  Int_t ihit, incr, ilast = GetSize()-1;
  // Ensure that the first element of the local arrays always corresponds
  // to the wire with the smallest x position
  bool reversed = ( fPlane->GetWSpac() < 0 );
  if( reversed ) {
    ihit = ilast;
    incr = -1;
  } else {
    ihit = 0;
    incr = 1;
  }
  for( Int_t i = 0; i < GetSize(); ihit += incr, ++i ) {
    assert( ihit >= 0 && ihit < GetSize() );
    Double_t x = fHits[ihit]->GetPos();
    Double_t y = fHits[ihit]->GetDist() + fTimeCorrection;

    Double_t wt = fHits[ihit]->GetdDist();
    if( wt>0 )   // the hit will be ignored if the uncertainty is <= 0
      wt = 1./(wt*wt);
    else
      wt = -1.;

    fCoord.push_back( FitCoord_t(x,y,wt) );

    assert( i == 0 || fCoord[i-1].x < fCoord[i].x );
  }

  Double_t bestFit = 0.0;

  //--- Perform 3-parameter for different sign coefficients
  //
  // We make some simplifying assumptions:
  // - The first wire of the cluster always has negative drift distance.
  // - The last wire always has positive drift.
  // - The sign flips exactly once from - to + somewhere in between
  fCoord[0].s = -1;
  for( Int_t i = 1; i < GetSize(); ++i )
    fCoord[i].s = 1;

  for( Int_t ipivot = 0; ipivot < ilast; ++ipivot ) {
    if( ipivot != 0 )
      fCoord[ipivot].s *= -1;

    // Do the fit
    Linear3DFit( m, b, d0 );

    // calculate chi2 for the track given this slope,
    // intercept, and distance offset
    chi2_t chi2 = CalcChisquare( m, b, d0 );

    // scale the uncertainty of the fit parameters based upon the
    // quality of the fit. This really should not be necessary if
    // we believe the drift-distance uncertainties
//     sigmaM *= chi2/(nhits - 2);
//     sigmaB *= chi2/(nhits - 2);

    // Pick the better value
    if( ipivot == 0 || chi2.first < bestFit ) {
      bestFit     = fChi2 = chi2.first;
      fNDoF       = chi2.second - 3;
      fLocalSlope = m;
      fInt        = b;
      fT0         = d0;
      fSigmaSlope = sigmaM;
      fSigmaInt   = sigmaB;
      fSigmaT0    = sigmaD0;
    }
  }

  // Rotate the coordinate system to match the VDC definition of "slope"
  fLocalSlope = 1.0/fLocalSlope;  // 1/m
  fInt   = -fInt * fLocalSlope;   // -b/m
  if( fPlane ) {
    fT0      /= fPlane->GetDriftVel();
    fSigmaT0 /= fPlane->GetDriftVel();
  }

  fFitOK = true;

  return 0;
}

//_____________________________________________________________________________
void THaVDCCluster::Linear3DFit( Double_t& m, Double_t& b, Double_t& d0 ) const
{
  // 3-parameter fit
//     Double_t F, sigmaF2;  // intermediate slope and St. Dev.**2
//     Double_t G, sigmaG2;  // intermediate intercept and St. Dev.**2
//     Double_t sigmaFG;     // correlated uncertainty

  Double_t sumX   = 0.0;   //Positions
  Double_t sumXX  = 0.0;
  Double_t sumD   = 0.0;   //Drift distances
  Double_t sumXD  = 0.0;
  Double_t sumS   = 0.0;   //sign vector
  Double_t sumSX  = 0.0;
  Double_t sumSD  = 0.0;
  Double_t sumSDX = 0.0;

  //     Double_t sumXW = 0.0;
  //     Double_t sumXXW= 0.0;

  Double_t sumW  = 0.0;
  //     Double_t sumWW = 0.0;
  //     Double_t sumDD = 0.0;


  for (int j = 0; j < GetSize(); j++) {
    Double_t x = fCoord[j].x;   // Position of wire
    Double_t d = fCoord[j].y;   // Distance to wire
    Double_t w = fCoord[j].w;   // Weight/error of distance measurement
    Int_t    s = fCoord[j].s;   // Sign of distance

    if (w <= 0) continue;

    sumX   += x * w;
    sumXX  += x * x * w;
    sumD   += d * w;
    sumXD  += x * d * w;
    sumS   += s * w;
    sumSX  += s * x * w;
    sumSD  += s * d * w;
    sumSDX += s * d * x * w;
    sumW   += w;
    //       sumWW  += w*w;
    //       sumXW  += x * w * w;
    //       sumXXW += x * x * w * w;
    //       sumDD  += d * d * w;
  }

  // Standard formulae for linear regression (see Bevington)
  Double_t Delta =
    sumXX   * ( sumW  * sumW - sumW * sumS  ) -
    sumX    * ( sumX  * sumW - sumX * sumS  );

  m =
    sumSDX  * ( sumW  * sumW - sumW * sumS  ) -
    sumSD   * ( sumX  * sumW - sumW * sumSX ) +
    sumD    * ( sumX  * sumS - sumW * sumSX );

  b =
    -sumSDX * ( sumX  * sumW - sumX * sumS  ) +
    sumSD   * ( sumXX * sumW - sumX * sumSX ) -
    sumD    * ( sumXX * sumS - sumX - sumSX );

  d0 = ( sumD - sumSD ) * ( sumXX * sumW - sumX * sumX );

  m  /= Delta;
  b  /= Delta;
  d0 /= Delta;


  //     F  = (sumXX * sumD - sumX * sumXD) / Delta;
  //     G  = (sumW * sumXD - sumX * sumD) / Delta;
  //     sigmaF2 = ( sumXX / Delta );
  //     sigmaG2 = ( sumW / Delta );
  //     sigmaFG = ( sumXW * sumW * sumXX - sumXXW * sumW * sumX
  //		- sumWW * sumX * sumXX + sumXW * sumX * sumX ) / (Delta*Delta);

  //     m  =   1/G;
  //     b  = - F/G;

  //     sigmaM = m * m * TMath::Sqrt( sigmaG2 );
  //     sigmaB = TMath::Sqrt( sigmaF2/(G*G) + F*F/(G*G*G*G)*sigmaG2 - 2*F/(G*G*G)*sigmaFG);

}

//_____________________________________________________________________________
Int_t THaVDCCluster::GetPivotWireNum() const
{
  // Get wire number of cluster pivot (hit with smallest drift distance)

  return fPivot ? fPivot->GetWireNum() : -1;
}


//_____________________________________________________________________________
void THaVDCCluster::SetTrack( THaTrack* track )
{
  // Mark this cluster as used by the given track whose number (index+1) is num

  Int_t num = track ? track->GetTrkNum() : 0;

  // Mark all hits
  // FIXME: naahh - only the hits used in the fit! (ignore for now)
  for( int i=0; i<GetSize(); i++ ) {
    // Bugcheck: hits must either be unused or been used by this cluster's
    // track (so SetTrack(0) can release a cluster from a track)
    assert( fHits[i] );
    assert( fHits[i]->GetTrkNum() == 0 || fHits[i]->GetTrkNum() == fTrkNum );

    fHits[i]->SetTrkNum( num );
  }
  fTrack = track;
  fTrkNum = num;
}

//_____________________________________________________________________________
chi2_t THaVDCCluster::CalcChisquare( Double_t slope, Double_t icpt,
				     Double_t d0 ) const
{
  Int_t npt = 0;
  Double_t chi2 = 0;
  for( int j = 0; j < GetSize(); ++j ) {
    Double_t x  = fCoord[j].x;
    Double_t y  = fCoord[j].s * fCoord[j].y;
    Double_t w  = fCoord[j].w;
    Double_t yp = x*slope + icpt + d0*fCoord[j].s;
    if( w < 0 ) continue;
    Double_t d  = y-yp;
    chi2       += d*d*w;
    ++npt;
  }
  return make_pair( chi2, npt );
}

//_____________________________________________________________________________
void THaVDCCluster::DoCalcChisquare( Double_t& chi2, Int_t& nhits,
				     Double_t slope, bool do_print ) const
{
  // Implementation of chi2 calculation for this cluster.

  if( TMath::Abs(slope) < 1e-3 ) {
    // Near zero slope = perpendicular incidence. No meaningful chi2 can be
    // calculated because such a track would only cross one, maybe two, cells.
    chi2 += kBig;
    nhits += GetSize();
    return;
  }

  // Parameters of the track in the flipped coordinate system where the
  // independent, uncertainty-free variable x is the wire position.
  Double_t m = 1.0/slope;
  Double_t b = -fInt*m;

  bool past_pivot = false;
  Int_t nHits = GetSize();
  for (int j = 0; j < nHits; ++j) {
    Double_t x  = fHits[j]->GetPos();
    Double_t y  = fHits[j]->GetDist() + fTimeCorrection;
    Double_t dy = fHits[j]->GetdDist();
    // The Y calculation is numerically not quite optimal since it is a
    // difference of two relatively large numbers. The result is 2 to 4 orders
    // of magnitude smaller than each term. This should fit easily within
    // double precision, however.
    Double_t Y  = m*x + b;

    // Handle negative slopes correctly for generality. With the VDCs,
    // negative slopes never correspond to signal tracks.
    if (m<0) y = -y;

    if (past_pivot)
      y = -y;
    else if (fHits[j] == fPivot) {
      // Test the other side of the pivot wire, take the 'best' choice.
      // This isn't statistically correct, but it's the best one can do
      // since the sign of the drift distances isn't measured.
      if (TMath::Abs(y-Y) > TMath::Abs(y+Y))
	y = -y;
      past_pivot = true;
    }
    if (dy <= 0) continue;

    if (do_print) {
      cout << " " << (y-Y)/dy;
      if( fHits[j] == fPivot )
	cout << "*";
    }

    chi2 += (Y - y)/dy * (Y - y)/dy;
    nhits++;
  }
}

//_____________________________________________________________________________
void THaVDCCluster::CalcChisquare( Double_t& chi2, Int_t& nhits ) const
{
  // Given the parameters of the track (global slope and intercept), calculate
  // the chi2 for the cluster.

  DoCalcChisquare( chi2, nhits, fSlope, false );
}

//_____________________________________________________________________________
void THaVDCCluster::Print( Option_t* ) const
{
  // Print contents of cluster

  Int_t nHits = GetSize();
  if( fPlane )
    cout << "Plane: " << fPlane->GetPrefix() << endl;
  cout << "Size: " << nHits << endl;

  cout << "Wire numbers:";
  for( int i = 0; i < nHits; i++ ) {
    cout << " " << fHits[i]->GetWireNum();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire raw times:";
  for( int i = 0; i < nHits; i++ ) {
    cout << " " << fHits[i]->GetRawTime();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire times:";
  for( int i = 0; i < nHits; i++ ) {
    cout << " " << fHits[i]->GetTime();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire positions:";
  for( int i = 0; i < nHits; i++ ) {
    cout << " " << fHits[i]->GetPos();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire drifts:";
  for( int i = 0; i < nHits; i++ ) {
    cout << " " << fHits[i]->GetDist();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire drifts sigmas:";
  for( int i = 0; i < nHits; i++ ) {
    cout << " " << fHits[i]->GetdDist();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;

  cout << "Fit done: " << fFitOK << endl;
  cout << "Crossing pos (err) = "
       << fInt        << " (" << fSigmaInt   << ")" << endl
       << "Local slope (err)  = "
       << fLocalSlope << " (" << fSigmaSlope << ")" << endl
       << "Global slope       = "
       << fSlope << endl
       << "Time offset (err)  = "
       << fT0         << " (" << fSigmaT0    << ")" << endl;

  if( fFitOK ) {
    Double_t chi2 = 0, lchi2 = 0;
    Int_t nhits = 0;
    cout << "Local residuals  = ";
    DoCalcChisquare( lchi2, nhits, fLocalSlope, true );
    cout << endl;
    nhits = 0;
    cout << "Global residuals = ";
    DoCalcChisquare( chi2, nhits, fSlope, true );
    cout << endl;
    cout << "Local/global chi2 = " << lchi2 << ", " << chi2 << endl;;
  }
}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDCCluster)

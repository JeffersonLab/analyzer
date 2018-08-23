///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDCCluster                                                             //
//                                                                           //
// A group of VDC hits and routines for linear fitting of drift distances.   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "OldVDCCluster.h"
#include "OldVDCHit.h"
#include "OldVDCPlane.h"
#include "OldVDCUVTrack.h"
#include "THaTrack.h"
#include "TMath.h"
#include "TClass.h"

#include <iostream>

using namespace std;

const Double_t OldVDCCluster::kBig = 1e38;  // Arbitrary large value

//_____________________________________________________________________________
OldVDCCluster::OldVDCCluster( const OldVDCCluster& rhs ) :
  TObject(rhs),
  fSize(rhs.fSize), fPlane(rhs.fPlane), fSlope(rhs.fSlope), 
  fSigmaSlope(rhs.fSigmaSlope), fInt(rhs.fInt), fSigmaInt(rhs.fSigmaInt), 
  fT0(rhs.fT0), fSigmaT0(rhs.fSigmaT0), fPivot(rhs.fPivot), 
  fTimeCorrection(rhs.fTimeCorrection), fFitOK(rhs.fFitOK),
  fLocalSlope(rhs.fLocalSlope), fChi2(rhs.fChi2), fNDoF(rhs.fNDoF)
{
  // Copy constructor

  for( int i = 0; i < fSize; i++ ) {
    fHits[i] = rhs.fHits[i];
  }
}

//_____________________________________________________________________________
OldVDCCluster& OldVDCCluster::operator=( const OldVDCCluster& rhs )
{
  // Assignment operator

  TObject::operator=( rhs );
  if( this != &rhs ) {
    fSize       = rhs.fSize;
    fPlane      = rhs.fPlane;
    fSlope      = rhs.fSlope;
    fSigmaSlope = rhs.fSigmaSlope;
    fInt        = rhs.fInt;
    fSigmaInt   = rhs.fSigmaInt;
    fT0         = rhs.fT0;
    fSigmaT0    = rhs.fSigmaT0;
    fPivot      = rhs.fPivot;
    fTimeCorrection = rhs.fTimeCorrection;
    fFitOK      = rhs.fFitOK;
    fLocalSlope = rhs.fLocalSlope;
    fChi2       = rhs.fChi2;
    fNDoF       = rhs.fNDoF;
    for( int i = 0; i < fSize; i++ )
      fHits[i] = rhs.fHits[i];
  }
  return *this;
}

//_____________________________________________________________________________
OldVDCCluster::~OldVDCCluster()
{
  // Destructor
}

//_____________________________________________________________________________
void OldVDCCluster::AddHit(OldVDCHit * hit)
{
  //Add a hit to the cluster

  if (fSize < MAX_SIZE) {
    fHits[fSize++] = hit;
  } else if( fPlane && fPlane->GetDebug()>0 ) {
    Warning( "AddHit()", "Max cluster size reached.");
  }
}

//_____________________________________________________________________________
void OldVDCCluster::Clear( Option_t* )
{
  // Clear the contents of the cluster

  ClearFit();
  fSize  = 0;
  fPivot = NULL;
  fPlane = NULL;
//    fUVTrack = NULL;
//    fTrack = NULL;

}

//_____________________________________________________________________________
void OldVDCCluster::ClearFit()
{
  // Clear fit results only
  
  fSlope      = kBig;
  fSigmaSlope = kBig;
  fInt        = kBig;
  fSigmaInt   = kBig;
  fT0         = 0.0;
  fSigmaT0    = kBig;
  fFitOK      = false;
  fLocalSlope = kBig;
  fChi2       = kBig;
  fNDoF       = 0.0;
}

//_____________________________________________________________________________
Int_t OldVDCCluster::Compare( const TObject* obj ) const
{
  // Compare this cluster to another via the wire number of the pivot.
  // Returns -1 if comparison cannot be made (unlike class, no pivot).

  if( !obj || IsA() != obj->IsA() )
    return -1;

  const OldVDCCluster* rhs = static_cast<const OldVDCCluster*>( obj );
  if( GetPivotWireNum() < rhs->GetPivotWireNum() )
    return -1;
  if( GetPivotWireNum() > rhs->GetPivotWireNum() )
    return +1;

  return 0;
}

//_____________________________________________________________________________
void OldVDCCluster::EstTrackParameters()
{
  // Estimate Track Parameters
  // Calculates pivot wire and uses its position as position of cluster
  // Estimates the slope based on the distance between the first and last
  // wires to be hit in the U (or V) and detector Z directions

  fFitOK = false;
  if( fSize == 0 )
    return;
  
  // Find pivot
  Double_t time = 0, minTime = 1000;  // drift-times in seconds
  for (int i = 0; i < fSize; i++) {
    time = fHits[i]->GetTime();
    if (time < minTime) { // look for lowest time
      minTime = time;
      fPivot = fHits[i];
    }
  }
  
  // Now set intercept
  fInt = fPivot->GetPos();

  // Now find the slope (this is a very coarse approximation)
  //   X = Drift Distance (m)
  //   Y = Position of Wires (m)
  if( fSize > 1 ) {
    Double_t conv = fPlane->GetDriftVel();  // m/s
    Double_t dx = conv * (fHits[0]->GetTime() + fHits[fSize-1]->GetTime());
    Double_t dy = fHits[0]->GetPos() - fHits[fSize-1]->GetPos();
    fSlope = dy / dx;
  } else
    fSlope = 1.0;

  CalcDist();
  fLocalSlope = fSlope;

  fFitOK = true;
}

//_____________________________________________________________________________
void OldVDCCluster::ConvertTimeToDist()
{
  // Convert TDC Times in wires to drift distances

  //Do conversion for each hit in cluster
  for (int i = 0; i < fSize; i++)
    fHits[i]->ConvertTimeToDist(fSlope);
}

//_____________________________________________________________________________
void OldVDCCluster::CalcDist()
{
  // Calculate and store the distance of the fitted-track to the wires.
  //  We can then inspect the quality of the fits
  for (int j = 0; j < fSize; j++) {
//    Double_t x = fHits[j]->GetDist() + fTimeCorrection;
    Double_t y = fHits[j]->GetPos();

//    Double_t Y = fSlope * x + fInt;
    if (fSlope!=0.) {
      Double_t X = (y-fInt)/fSlope;
      fHits[j]->SetFitDist(TMath::Abs(X));
    } else {
      fHits[j]->SetFitDist(kBig);
    }
  }
}
  
//_____________________________________________________________________________
void OldVDCCluster::FitTrack( EMode /* mode */ )
{
  // Fit track to drift distances. Supports three modes:
  // 
  // kSimple:  Linear fit, ignore t0 and multihits
  // kT0:      Fit t0, but ignore mulithits
  // kFull:    Analyze multihits and fit t0
  // 
  // FIXME: kT0 and kFull are not yet implemented. Identical to kSimple.

  FitSimpleTrack();
  //  FitSimpleTrackWgt();
  CalcDist();
}

//_____________________________________________________________________________
void OldVDCCluster::FitSimpleTrack()
{
  // Perform linear fit on drift times. Calculates slope, intercept, and errors.
  // Assume t0 = 0.

  // For this function,
  //   X = Drift Distance
  //   Y = Position of Wires

  fFitOK = false;
  if( fSize < 3 )
    return;  // Too few hits to get meaningful results
             // Do keep current values of slope and intercept

  Double_t N = fSize;  //Ensure that floating point calculations are used
  Double_t m, sigmaM;  // Slope, St. Dev. in slope
  Double_t b, sigmaB;  // Intercept, St. Dev in Intercept
  Double_t sigmaY;     // St Dev in delta Y values

  Double_t* xArr = new Double_t[fSize];
  Double_t* yArr = new Double_t[fSize];

  Double_t bestFit = 0.0;

  // Find the index of the pivot wire and copy distances into local arrays
  // Note that as the index of the hits is increasing, the position of the
  // wires is decreasing

  Int_t pivotNum = 0;
  for (int i = 0; i < fSize; i++) {
    if (fHits[i] == fPivot) {
      pivotNum = i;
    }
    xArr[i] = fHits[i]->GetDist() + fTimeCorrection;
    yArr[i] = fHits[i]->GetPos();
  }
  
  const Int_t nSignCombos = 2; //Number of different sign combinations
  for (int i = 0; i < nSignCombos; i++) {

    Double_t sumX  = 0.0;   //Drift distances
    Double_t sumXX = 0.0;
    Double_t sumY  = 0.0;   //Position of wires
    Double_t sumXY = 0.0;
    Double_t sumDY2 = 0.0;  // Sum of squares of delta Y values
    
    if (i == 0)
      for (int j = pivotNum+1; j < fSize; j++)
	xArr[j] = -xArr[j];
    else if (i == 1)
      xArr[pivotNum] = -xArr[pivotNum];

    for (int j = 0; j < fSize; j++) {
      Double_t x = xArr[j];  // Distance to wire
      Double_t y = yArr[j];   // Position of wire
      sumX  += x;
      sumXX += x * x;
      sumY  += y;
      sumXY += x * y;
    }

    // Standard formulae
    m  = (N * sumXY - sumX * sumY) / (N * sumXX - sumX * sumX);
    b  = (sumXX * sumY - sumX * sumXY) / (N * sumXX - sumX * sumX);

    // Calculate sum of delta y values
    for (int j = 0; j < fSize; j++) {
      Double_t y = yArr[j];
      Double_t Y = m * xArr[j] + b;
      sumDY2 += (y - Y) * (y - Y);
    } 
    
    sigmaY = TMath::Sqrt (sumDY2 / (N - 2));
    sigmaM = sigmaY * TMath::Sqrt ( N / ( N * sumXX - sumX * sumX) );
    sigmaB = sigmaY * TMath::Sqrt (sumXX / ( N * sumXX - sumX * sumX) );
    
    // Pick the best value
    if (i == 0 || sigmaY < bestFit) {
      bestFit = sigmaY;
      fSlope = m;
      fSigmaSlope = sigmaM;
      fInt = b;
      fSigmaInt = sigmaB;
    }
  }

  // calculate the best possible chi2 for the track given this slope and intercept
  Double_t chi2 = 0.;
  Int_t nhits = 0;
  
  CalcChisquare(chi2,nhits);
  fChi2 = chi2;
  fNDoF = nhits-2;
  
  fLocalSlope = fSlope;
  fFitOK = true;

  delete[] xArr;
  delete[] yArr;
}

//_____________________________________________________________________________
void OldVDCCluster::FitSimpleTrackWgt()
{
  // Perform linear fit on drift times. Calculates slope, intercept, and errors.
  // Does not assume the uncertainty is the same for all hits.
  //
  // Assume t0 = 0.

  // For this function, the final results is (m,b) in  Y = m X + b
  //   X = Drift Distance
  //   Y = Position of Wires


  fFitOK = false;
  if( fSize < 3 ) {
    fChi2 = kBig;
    fNDoF = fSize;
    return;  // Too few hits to get meaningful results
             // Do keep current values of slope and intercept
  }
  
  Double_t m, sigmaM;  // Slope, St. Dev. in slope
  Double_t b, sigmaB;  // Intercept, St. Dev in Intercept

  Double_t* xArr = new Double_t[fSize];
  Double_t* yArr = new Double_t[fSize];
  Double_t* wtArr= new Double_t[fSize];
  
  Double_t bestFit = 0.0;
  
  // Find the index of the pivot wire and copy distances into local arrays
  // Note that as the index of the hits is increasing, the position of the
  // wires is decreasing

  Int_t pivotNum = 0;
  for (int i = 0; i < fSize; i++) {

    // In order to take into account the varying uncertainty in the
    // drift distance, we will be working with the X' and Y', and
    //       Y' = F + G X'
    //  where Y' = X, and X' = Y  (swapping it around)
    if (fHits[i] == fPivot) {
      pivotNum = i;
    }

    xArr[i] = fHits[i]->GetPos();
    yArr[i] = fHits[i]->GetDist() + fTimeCorrection;
    wtArr[i]= fHits[i]->GetdDist();

    // the hit will be ignored if the uncertainty is < 0
    if (wtArr[i]>0) wtArr[i] = 1./(wtArr[i]*wtArr[i]); // sigma^-2 is the weight
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
      for (int j = pivotNum+1; j < fSize; j++)
	yArr[j] *= -1;
    else if (i == 1)
      yArr[pivotNum] *= -1;

    for (int j = 0; j < fSize; j++) {
      Double_t x = xArr[j];   // Position of wire
      Double_t y = yArr[j];   // Distance to wire
      Double_t w =wtArr[j];
      
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
    sigmaFG = ( sumXW * W * sumXX - sumXXW * W * sumX
		- WW * sumX * sumXX + sumXW * sumX * sumX ) / (Delta*Delta);

    m  =   1/G;
    b  = - F/G;

    sigmaM = m * m * TMath::Sqrt( sigmaG2 );
    sigmaB = TMath::Sqrt( sigmaF2/(G*G) + F*F/(G*G*G*G)*sigmaG2 - 2*F/(G*G*G)*sigmaFG);
    
    // calculate the best possible chi2 for the track given this slope and intercept
    Double_t chi2 = 0.;
    Int_t nhits = 0;
    
    Double_t oldslope = fSlope;
    Double_t oldint   = fInt;
    CalcChisquare(chi2,nhits);
    fSlope = oldslope;
    fInt   = oldint;
    
    // scale the uncertainty of the fit parameters based upon the
    // quality of the fit. This really should not be necessary if
    // we believe the drift-distance uncertainties
    sigmaM *= chi2/(nhits - 2);
    sigmaB *= chi2/(nhits - 2);
    
    // Pick the best value
    if (i == 0 || chi2 < bestFit) {
      bestFit = chi2;
      fChi2 = chi2;
      fNDoF = nhits-2;
      fSlope = m;
      fSigmaSlope = sigmaM;
      fInt = b;
      fSigmaInt = sigmaB;
    }
  }

  fLocalSlope = fSlope;
  fFitOK = true;

  delete[] xArr;
  delete[] yArr;
  delete[] wtArr;
}

//_____________________________________________________________________________
Int_t OldVDCCluster::GetPivotWireNum() const
{
  // Get wire number of cluster pivot (hit with smallest drift distance)

  return fPivot ? fPivot->GetWireNum() : -1;
}

//_____________________________________________________________________________
void OldVDCCluster::CalcChisquare(Double_t& chi2, Int_t& nhits ) const
{
  // given the parameters of the track (slope and intercept), calculate the
  // residual chi2 for the cluster

  Int_t pivotNum = 0;
  for (int j = 0; j < fSize; j++) {
    if (fHits[j] == fPivot) {
      pivotNum = j;
    }
  }
  
  for (int j = 0; j < fSize; j++) {
    Double_t x = fHits[j]->GetDist() + fTimeCorrection;
    if (j>pivotNum) x = -x;
    
    Double_t y = fHits[j]->GetPos();
    Double_t dx = fHits[j]->GetdDist();
    Double_t Y = fSlope * x + fInt;
    Double_t dY = fSlope * dx;
    if (dx <= 0) continue;
    
    if ( fHits[j] == fPivot ) {
      // test the other side of the pivot wire, take the 'best' choice
      Double_t ox = -x;
      Double_t oY =  fSlope * ox + fInt;
      if ( TMath::Abs(y-Y) > TMath::Abs(y-oY) ) {
	//	x = ox;
	Y = oY;
      }
    }
    chi2 += (Y - y)/dY * (Y - y)/dY;
    nhits++;
  } 
}


//_____________________________________________________________________________
void OldVDCCluster::Print( Option_t* ) const
{
  // Print contents of cluster

  if( fPlane )
    cout << "Plane: " << fPlane->GetPrefix() << endl;
  cout << "Size: " << fSize << endl;

  cout << "Wire numbers:";
  for( int i = 0; i < fSize; i++ ) {
    cout << " " << fHits[i]->GetWireNum();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire raw times:";
  for( int i = 0; i < fSize; i++ ) {
    cout << " " << fHits[i]->GetRawTime();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire times:";
  for( int i = 0; i < fSize; i++ ) {
    cout << " " << fHits[i]->GetTime();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire positions:";
  for( int i = 0; i < fSize; i++ ) {
    cout << " " << fHits[i]->GetPos();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire drifts:";
  for( int i = 0; i < fSize; i++ ) {
    cout << " " << fHits[i]->GetDist();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;

  cout << "Int(err), local Slope(err), global slope, t0(err): " 
       << fInt   << " (" << fSigmaInt   << "), "
       << fLocalSlope << " (" << fSigmaSlope << "), "
       << fSlope << ", "
       << fT0    << " (" << fSigmaT0 << "), fit ok: " << fFitOK
       << endl;

}

///////////////////////////////////////////////////////////////////////////////
ClassImp(OldVDCCluster)

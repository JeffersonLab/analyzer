///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCCluster                                                             //
//                                                                           //
// A group of VDC hits and routines for linear fitting of drift distances.   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCCluster.h"
#include "THaVDCHit.h"
#include "THaVDCPlane.h"
#include "THaVDCUVTrack.h"
#include "THaTrack.h"
#include "TClass.h"

#include <iostream>

ClassImp(THaVDCCluster)

const Double_t THaVDCCluster::kBig = 1e307;  // Arbitrary large value

//_____________________________________________________________________________
THaVDCCluster::THaVDCCluster( const THaVDCCluster& rhs ) :
  THaCluster(rhs), fPlane(rhs.fPlane), fSlope(rhs.fSlope), 
  fSigmaSlope(rhs.fSigmaSlope), fInt(rhs.fInt), fSigmaInt(rhs.fSigmaInt), 
  fT0(rhs.fT0), fPivot(rhs.fPivot)
{
  // Copy constructor

  for( int i = 0; i < fSize; i++ )
    fHits[i] = rhs.fHits[i];
}

//_____________________________________________________________________________
THaVDCCluster& THaVDCCluster::operator=( const THaVDCCluster& rhs )
{
  // Assignment operator

  THaCluster::operator=( rhs );
  if( this != &rhs ) {
    fPlane      = rhs.fPlane;
    fSlope      = rhs.fSlope;
    fSigmaSlope = rhs.fSigmaSlope;
    fInt        = rhs.fInt;
    fSigmaInt   = rhs.fSigmaInt;
    fT0         = rhs.fT0;
    fPivot      = rhs.fPivot;

    for( int i = 0; i < fSize; i++ )
      fHits[i] = rhs.fHits[i];
  }
  return *this;
}

//_____________________________________________________________________________
void THaVDCCluster::AddHit(THaVDCHit * hit)
{
  //Add a hit to the cluster

  if (fSize < MAX_SIZE) {
    fHits[fSize++] = hit;
  } else if( fPlane && fPlane->GetDebug()>0 ) {
    Warning( "AddHit()", "Max cluster size reached.");
  }
}

//_____________________________________________________________________________
void THaVDCCluster::Clear( const Option_t* opt )
{
  // Clear the contents of the cluster

  THaCluster::Clear( opt );
  ClearFit();
  fPivot = NULL;
  fPlane = NULL;
//    fUVTrack = NULL;
//    fTrack = NULL;

}

//_____________________________________________________________________________
void THaVDCCluster::ClearFit()
{
  // Clear fit results only
  
  fSlope      = 0.0;
  fSigmaSlope = kBig;
  fInt        = 0.0;
  fSigmaInt   = kBig;
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

  // Find pivot
  Int_t time = 0, minTime = 0;  // Raw TDC times
  for (int i = 0; i < fSize; i++) {
    time = fHits[i]->GetRawTime();
    if (time > minTime) { // Higher TDC value means lower (raw) time
      minTime = time;
      fPivot = fHits[i];
    }
  }
  
  // Now set intercept
  fInt = fPivot->GetPos();

  // Now find the slope (this is a very coarse approximation)
  //   X = Drift Distance (m)
  //   Y = Position of Wires (m)

  Double_t conv = fPlane->GetDriftVel();  // m/s
  Double_t dx = conv * (fHits[0]->GetTime() + fHits[fSize-1]->GetTime());
  Double_t dy = fHits[0]->GetPos() - fHits[fSize-1]->GetPos();

  fSlope = dy / dx;
}

//_____________________________________________________________________________
void THaVDCCluster::ConvertTimeToDist()
{

  // Convert TDC Times in wires to drift distances

  for (int i = 0; i < fSize; i++)
    fHits[i]->ConvertTimeToDist(fSlope); //Do conversion for each hit in cluster
  
}

//_____________________________________________________________________________
void THaVDCCluster::FitTrack( EMode mode )
{
  // Fit track to drift distances. Supports three modes:
  // 
  // kSimple:  Linear fit, ignore t0 and multihits
  // kT0:      Fit t0, but ignore mulithits
  // kFull:    Analyze multihits and fit t0
  // 
  // kT0 and kFull are not yet implemented. Identical to kSimple.

  FitSimpleTrack();
}

//_____________________________________________________________________________
void THaVDCCluster::FitSimpleTrack()
{
  // Perform linear fit on drift times. Calculates slope, intercept, and errors.
  // Assume t0 = 0.

  // For this function,
  //   X = Drift Distance
  //   Y = Position of Wires

  if( fSize < 3 ) {
    ClearFit();
    return;  // Too few hits to get meaningful results
  }

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
    xArr[i] = fHits[i]->GetDist();
    yArr[i] = fHits[i]->GetPos();
  }
  
  Int_t nSignCombos = 2; //Number of different sign combinations
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
    
    sigmaY = sqrt (sumDY2 / (N - 2));
    sigmaM = sigmaY * sqrt ( N / ( N * sumXX - sumX * sumX) );
    sigmaB = sigmaY * sqrt (sumXX / ( N * sumXX - sumX * sumX) );
    
    // Pick the best value
    if (i == 0 || sigmaY < bestFit) {
      bestFit = sigmaY;
      fSlope = m;
      fSigmaSlope = sigmaM;
      fInt = b;
      fSigmaInt = sigmaB;
    }
  }
  
  delete[] xArr;
  delete[] yArr;

}

//_____________________________________________________________________________
Int_t THaVDCCluster::GetPivotWireNum() const
{
  // Get wire number of cluster pivot (hit with smallest drift distance)

  return fPivot ? fPivot->GetWireNum() : -1;
}

//_____________________________________________________________________________
void THaVDCCluster::Print( Option_t* opt ) const
{
  // Print contents of cluster

  THaCluster::Print( opt );
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

  cout << "Slope(err), Int(err), t0: " 
       << fSlope << " (" << fSigmaSlope << "), "
       << fInt   << " (" << fSigmaInt   << "), "
       << fT0
       << endl;

}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCCluster                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCCluster.h"
#include "THaVDCHit.h"
#include "THaVDCPlane.h"
#include "THaVDCUVTrack.h"
#include "THaTrack.h"

ClassImp(THaVDCCluster)


//______________________________________________________________________________
THaVDCCluster::THaVDCCluster( THaVDCPlane* owner )
{
  // Constructor
  Clear();
  fPlane = owner;
}


//______________________________________________________________________________
THaVDCCluster::~THaVDCCluster()
{
  // Destructor.
  Clear();
}

//______________________________________________________________________________
void THaVDCCluster::AddHit(THaVDCHit * hit)
{
  //Add a hit to the cluster
  if (fSize < MAX_SIZE) {
    fHits[fSize] = hit;
    fSize++;
//    } else 
//      printf("THaVDCCluster::AddHit - Max Cluster Size reached.\n");
  }

}

//______________________________________________________________________________
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
  //   X = Drift Distance
  //   Y = Position of Wires

  Double_t conv = fPlane->GetTDCRes() * fPlane->GetDriftVel();
  Double_t dx = conv * (fHits[0]->GetRawTime() + fHits[fSize-1]->GetPos());
  Double_t dy = fHits[0]->GetPos() - fHits[fSize-1]->GetPos();

  fSlope = dy / dx;
}
//______________________________________________________________________________
void THaVDCCluster::ConvertTimeToDist()
{

  // Convert TDC Times in wires to drift distances

  for (int i = 0; i < fSize; i++)
    fHits[i]->ConvertTimeToDist(fSlope); //Do conversion for each hit in cluster
  
}
//______________________________________________________________________________
void THaVDCCluster::FitTrack()
{
  // Calculate slope, intercept and errors

  // For this function,
  //   X = Drift Distance
  //   Y = Position of Wires

  if (fSize < 3)
    return;  // Too few hits to get meaningful results

  Double_t N = fSize;  //Ensure that floating point calculations are used
  Double_t m, sigmaM;  // Slope, St. Dev. in slope
  Double_t b, sigmaB;  // Intercept, St. Dev in Intercept
  Double_t sigmaY;     // St Dev in delta Y values

  Double_t * xArr = new Double_t[fSize];
  Double_t * yArr = new Double_t[fSize];

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
  
//    printf("Looking for best fit for:\n");
//    printf("X:");
//    for (int i = 0; i < fSize; i++)
//      printf(" %f", xArr[i]);
//    printf("\nY:");
//    for (int i = 0; i < fSize; i++)
//      printf(" %f", yArr[i]);
//    printf("\n");

//    printf("Coarse: y = %f * x + %f\n", fSlope, fInt);

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

    //These formulas should be available in any statistics book

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
    
    // Now have to pick the best value
    if (i == 0 || sigmaY < bestFit) {
      bestFit = sigmaY;
      fSlope = m;
      fSigmaSlope = sigmaM;
      fInt = b;
      fSigmaInt = sigmaB;
    }
//      printf("Found: y = %f * x + %f, sigmaY = %f\n", m, b, sigmaY);

  }
//    printf("Best Fit: y = %f * x + %f\n", fSlope, fInt);
  
  delete[] xArr;
  delete[] yArr;

}

//______________________________________________________________________________
void THaVDCCluster::Clear( const Option_t* opt )
{
  // Clears the contents of the cluster
  for (int i = 0; i < MAX_SIZE; i++)
    fHits[i] = NULL;
  fSize  = 0;                    // Size of cluster
  fSlope = fSigmaSlope = 0.0;    // Slope and error in slope of track
  fInt   = fSigmaInt =   0.0;    // Intercept and error of track
  fPivot = NULL;                 // Pivot
  fPlane = NULL;
//    fUVTrack = NULL;               // UVTrack the cluster belongs to
//    fTrack = NULL;                 // Track the cluster belongs to

}

////////////////////////////////////////////////////////////////////////////////


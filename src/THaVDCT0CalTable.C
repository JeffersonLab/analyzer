///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCT0CalTable                                                          //
//                                                                           //
// T0 Calibration Table - Determines timing offset for a given wire          //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCT0CalTable.h"

ClassImp(THaVDCT0CalTable)


//______________________________________________________________________________
THaVDCT0CalTable::THaVDCT0CalTable()
{
  // Constructor.
  fBinSize = 14;
  fMinTime = 800;
  fMaxTime = 2200;

}

//______________________________________________________________________________
THaVDCT0CalTable::~THaVDCT0CalTable()
{
  // Destructor. 
 
}

//______________________________________________________________________________
void THaVDCT0CalTable::AddToCalibration(Int_t time)
{
  // Increment the appropriate bin
  if (time < fMaxTime && time > fMinTime)
    fCalibrationTable[( time - fMinTime ) / fBinSize]++;
  //  else printf("Time: %d\n", time);
}

//______________________________________________________________________________
Double_t THaVDCT0CalTable::CalcTOffset()
{
  // Calculate TOffset, based on the contents of the Calibration Table
  // Start from left
  int maxBin = -1; //Bin containing the most counts
  int maxCnt = 0;  //Number of counts in maxBin
  for (int i = 98; i >= 0; i--) {
    if (fCalibrationTable[i] > maxCnt) { 
      // New max bin
      maxCnt = fCalibrationTable[i];
      maxBin = i;
    }

  }
  if (maxBin == -1) {
    printf ("T0 calculation failed.\n");
    return 0;
  }
  int leftBin, rightBin;
  int leftCnt, rightCnt;
  for (int i = maxBin; i < 99; i++) {
    // Now find bins for fit
    if (fCalibrationTable[i] > 0.9 * maxCnt) {
      leftBin = i;
      leftCnt = fCalibrationTable[i];
    }
    if (fCalibrationTable[i] < 0.1 * maxCnt) {
      rightBin = i;
      rightCnt = fCalibrationTable[i];
      break;
  
    }
  }
  // Now fit a line to the bins between left and right

  Double_t sumX  = 0.0;   //Counts
  Double_t sumXX = 0.0;
  Double_t sumY  = 0.0;   //Bins
  Double_t sumXY = 0.0;

  for (int i = leftBin; i <= rightBin; i++) {
    Double_t x = fCalibrationTable[i];
    Double_t y = i;
    sumX  += x;
    sumXX += x * x;
    sumY  += y;
    sumXY += x * y;
  }
    
  //These formulas should be available in any statistics book
  
  Double_t N = rightBin - leftBin + 1;
  Double_t yInt = (sumXX * sumY - sumX * sumXY) / (N * sumXX - sumX * sumX);
  fTOffset = yInt * fBinSize + fMinTime;
  
  return fTOffset;

}

////////////////////////////////////////////////////////////////////////////////

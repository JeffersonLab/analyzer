//////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCLookupTTDConv                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCLookupTTDConv.h"

#include <iostream>

ClassImp(THaVDCLookupTTDConv)


//______________________________________________________________________________
THaVDCLookupTTDConv::THaVDCLookupTTDConv()
{
  //Normal constructor
  fLongestDist = 0.01512;
  fTimeRes = 0.5e-9;

  // TODO: This should be read from database!!
  fA1tdcCor[0] = 2.12e-3;
  fA1tdcCor[1] = 0.0;
  fA1tdcCor[2] = 0.0;
  fA1tdcCor[3] = 0.0;
  fA2tdcCor[0] = -4.20e-4;
  fA2tdcCor[1] =  1.3e-3;
  fA2tdcCor[2] = 1.06e-4;
  fA2tdcCor[3] = 0.0;
}


//______________________________________________________________________________
THaVDCLookupTTDConv::~THaVDCLookupTTDConv()
{
  // Destructor. Remove variables from global list.

}

//______________________________________________________________________________
Double_t THaVDCLookupTTDConv::ConvertTimeToDist(Double_t time, 
						Double_t tanTheta)
{
  int binned_time;
  Double_t dist;

  // initial culling
  if(time < fT0) {
    //cout<<"culled time of "<<time<<" against a t0 of "<<fT0<<endl;
    return 0.0;
  }

  /***/
  /*
  Double_t a1 = 0.0, a2 = 0.0;
  // Find the values of a1 and a2 by evaluating the proper polynomials
  // a = A_3 * x^3 + A_2 * x^2 + A_1 * x + A_0

  tanTheta = 1.0 / tanTheta;  // I assume this has to do w/ making the
                              // polynomial have the proper variable...

  for (Int_t i = 3; i >= 1; i--) {
    a1 = tanTheta * (a1 + fA1tdcCor[i]);
    a2 = tanTheta * (a2 + fA2tdcCor[i]);
  }
  a1 += fA1tdcCor[0];
  a2 += fA2tdcCor[0];
  */
//    printf("a1(%e) = %e\n", tanTheta, a1);
//    printf("a2(%e) = %e\n", tanTheta, a2);
  /***/

  // bin the given time
  binned_time = static_cast<int>((time - fT0) / fTimeRes);
  //cout<<binned_time<<": "<<time<<" "<<fT0<<" "<<fNumBins<<endl;
  //cout<<binned_time<<endl;

  // make sure it's in the correct range
  if(binned_time >= fNumBins)
    return fLongestDist;

  // figure out time
  dist = fTable[binned_time];


  //if((dist == 0.0) && (fT0 == 0.0))
  //  cout<<"got distance of zero with time "<<time<<" against a t0 of "<<fT0<<endl;


  // do angle adjustments?
  /***/
  /*
  if (dist < a1 ) { 
    //    dist = dist * (1 + 1 / (a1/a2 + 1));
    dist *= ( 1 + a2 / a1);
  }
  else {
    dist +=  a2;
  }
  */
  /***/

  return dist;
}


////////////////////////////////////////////////////////////////////////////////

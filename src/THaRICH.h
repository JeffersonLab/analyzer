#ifndef ROOT_THaRICH
#define ROOT_THaRICH

//////////////////////////////////////////////////////////////////////////
//
// THaRICH
//
// The Hall A RICH
//
//////////////////////////////////////////////////////////////////////////

#include "THaPidDetector.h"
#include "THaRICHClusterList.h"
#include "TVector3.h"
#include "THaMatrix.h"

#include <stdio.h>
#include <stdlib.h>

class TClonesArray;
class THaTrack;
class TDatime;

class THaRICH : public THaPidDetector {
  
public:

  static const int kTOTAL_PADS = 1200; //***IMPORTANT***
                                 //the number of total CsI-pads on whole plane.

  THaRICH( const char* name, const char* description = "",
	   THaApparatus* a = NULL );
  virtual ~THaRICH();
  
  virtual Int_t        Decode( const THaEvData& );
  virtual Int_t        CoarseProcess( TClonesArray& tracks );
  virtual Int_t        FineProcess( TClonesArray& tracks );
  virtual EStatus      Init( const TDatime& run_time );

  Int_t                ReadData( FILE *infile );

protected:

  THaRICHHit        fHits[ kTOTAL_PADS ];    //Array of hits for each event
  Int_t             fNHits;                  //Number of hits in the array

  THaRICHCluster    fClusters[ kTOTAL_PADS/3+1 ];  //Array of clusters of hits
  Int_t             fNClusters;

  THaRICHCluster** fMIP;          //Array of MIP clusters

  //RICH parameters ... currently read from 'rich.dat'.

  Float_t L_RAD,l_quartz,l_gap;    //length of radiator,quartz,proxiity gap
  Float_t l_emission;              //photon emission depth in the radiator.
  Float_t n_radiator,n_quartz,n_gap; //the refraction indices 
  Float_t PAD_SIZE_X;              //dimension of a pad (mm). 
  Float_t PAD_SIZE_Y;              //dimension of a pad (mm).
  Int_t PADS_X,PADS_Y;             //the number of the pads 
                                   // in a row and a column 
                                   // associated with ONE ADC.

  Float_t fMaxdist2;               // Search radius for MIP finding

  // Geometry ... currently hardcoded in Init()

  TVector3  fXax;                  // X axis of the RICH plane
  TVector3  fYax;                  // Y axis of the RICH plane
  TVector3  fZax;                  // Normal to the RICH plane
  THaMatrix fDenom;                // Denominator matrix for intercept calc
  THaMatrix fNom;                  // Nominator matrix for intercept calc


  Int_t   GetPadX( Int_t pad );    //Get x coord corresponding to pad number
  Int_t   GetPadY( Int_t pad );    //Get y coord corresponding to pad number

  void    ReadRICH();

  void    Padn2xy(Int_t, Int_t, Float_t);
  void    Padn2xy(Int_t, Int_t );
  void    DeleteClusters();
  Int_t   FindClusters();
  Int_t   FindMIP( const TClonesArray& tracks );

  Float_t Get_phi_photon( Float_t x_photon, Float_t y_photon,
			  Float_t x_mip, Float_t y_mip,
			  Float_t theta_mip, Float_t phi_mip );

  Float_t Get_a( Float_t theta_mip );

  Float_t Get_b( Float_t x_photon, Float_t y_photon,
		 Float_t x_mip, Float_t y_mip,
		 Float_t theta_mip, Float_t phi_mip);

  Float_t Get_theta_photon(Float_t x_photon, Float_t y_photon,
			   Float_t x_mip, Float_t y_mip,
			   Float_t theta_mip, Float_t phi_mip);

  Float_t RecoAng( Float_t x_photon, Float_t y_photon,
		   Float_t x_mip, Float_t y_mip,
		   Float_t theta_mip, Float_t phi_mip) ;

  // Prevent default construction, copying, assignment
  THaRICH() {}
  THaRICH( const THaRICH& ) {}
  THaRICH& operator=( const THaRICH& ) { return *this; }

private:

  ClassDef(THaRICH,0)   //The Hall A RICH
};

//______________ inline functions _____________________________________________
inline 
Int_t THaRICH::GetPadX( Int_t pad )           
{ 
  //Get the x-coordinate coresponding to a pad number

  div_t dt = div( pad-1, PADS_X*PADS_Y );
  return dt.quot*PADS_X + dt.rem%PADS_X + 1;
}

//_____________________________________________________________________________
inline 
Int_t THaRICH::GetPadY( Int_t pad )
{
  //Get the y-coordinate coresponding to a pad number

  return (pad % (PADS_X*PADS_Y)) / PADS_X + 1;
}

#endif


//*-- Author :    Guido Maria Urciuoli   12 March 2001

//////////////////////////////////////////////////////////////////////////
//
// THaRICH
//
// The RICH detector
// Written by Guido Maria Urciuoli, INFN
// Adapted for Hall A Analyzer by Ole Hansen, JLab
//
//////////////////////////////////////////////////////////////////////////


#include "TClonesArray.h"
#include "THaRICH.h"
#include "THaTrack.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "THaMatrix.h"
#include "TDatime.h"

#include <fstream>
#include <strstream>
#include <cstring>
#include <cmath>

ClassImp(THaRICH)


const int THaRICH::kTOTAL_PADS;

//_____________________________________________________________________________
THaRICH::THaRICH( const char* name, const char* description,
		  THaApparatus* apparatus ) 
  : THaPidDetector(name,description,apparatus), fMIP(NULL)
{
  // Constructor.

  //FIXME: Read fNelem from database and allocate arrays dynamically
  fNelem = kTOTAL_PADS;
}

//_____________________________________________________________________________
THaDetectorBase::EStatus THaRICH::Init( const TDatime& run_time )
{
  // Initialize detector parameters and set up globally visible symbolic 
  // variables.

  // Set up the detector map for this detector
  // These data will come from a database in the future.

  MakePrefix();

  for (int i = 1; i < 17; i++) {
    if( fDetMap->AddModule(16,i,0,480) < 0 ) {
      Error("Init()", "Detector map is full. RICH not initialized.");
      return fStatus = kInitError;
    }
  }

  ReadRICH();

  // Search radius (maximum distance a cluster can be away 
  // from a track to be a candidate for a MIP).
  // Note that this is the SQUARE of the distance.

  fMaxdist2 = 10.0*(PAD_SIZE_X+PAD_SIZE_Y);
  fMaxdist2 *= fMaxdist2;

  fNClusters = 0; 
  fNHits = 0;

  // Geometry parameters. THESE ARE IMPORTANT. 
  // FIXME: Get from database!  
  // FIXME: These numbers are just guesses, must replace with real data

  fOrigin.SetXYZ( 0.0, 0.0, 100.0 );     // position of RICH plane (cm)
  fXax.SetXYZ( 1.0, 0.0, 0.0 );          // unit vector along x-axis 
                                         //  of RICH plane
  fYax.SetXYZ( 0.0, 1.0, 0.0 );          // unit vector along y-axis
  fZax = fXax.Cross(fYax);               // z-axis, perpendicular to RICH
                                         //  plane, pointing along beam

  // Matrices for the track intercept calculation. 
  // These have been made member variables for efficiency. 
  // The following lines should not be modified. 
  // Make all required changes above (i.e. in the database).

  fDenom.ResizeTo( 3, 3 );
  fNom.ResizeTo( 3, 3 );
  fDenom.SetColumn( fXax, 0 );
  fDenom.SetColumn( fYax, 1 );
  fNom.SetColumn( fXax, 0 );
  fNom.SetColumn( fYax, 1 );

  // Register variables in global list
  // FIXME: Define useful RICH variables here

  return fStatus = kOK;
}

//_____________________________________________________________________________
THaRICH::~THaRICH()
{
  // Destructor. Remove variables from global list.

  delete [] fMIP;
  RemoveVariables();
}

//_____________________________________________________________________________
Int_t THaRICH::Decode( const THaEvData& evdata )
{
  //Decode RICH data and fill hit array

  if( !evdata.IsPhysicsTrigger() ) return 0;

  fNHits = 0;     //Clear previous hits

  // Loop over all modules defined for this detector

  for( UShort_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );

    // Loop over all channels that have a hit.

    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan > d->hi || chan < d->lo ) continue;   // Not one of my channels

      // Get the data.

      Int_t ADC = d->slot - 1;

      Int_t nhit = evdata.GetNumHits( d->crate, d->slot, chan );
      if( fNHits+nhit >= kTOTAL_PADS ) {
	Warning("Decode()", "Too many hits! Event skipped.");
	fNHits = 0;
	return 0;
      }
      for (int hit = 0; hit < nhit; hit++) {
	Int_t istartx = 1 - ADC/8;
	Int_t istarty = ADC/8;
	Int_t initialvalue = chan/6;
	Int_t ix, iy;
			  
	if( (ADC % 2) == 0 ) {
	  ix = initialvalue + 1 + 80*istartx;
	  iy = chan - initialvalue*6 + 1 + (ADC - 8*istarty)*6;
	} else {
	  ix = 80 - initialvalue  + 80*istartx;
	  iy = 6 - (chan - initialvalue*6) + (ADC-8*istarty)*6;
	}

	// Fill hit array

	Int_t data = evdata.GetData(d->crate,d->slot,chan,hit);
	Padn2xy( ix, iy, static_cast<Float_t>(data) );
      }
    }
  }

  return fNHits;
}


//_____________________________________________________________________________
//****************************************************************************
// ReadRICH()
//   The function to read the basic RICH parameters from the file "rich.dat"
//   These data will come from a database in the future.
//****************************************************************************
void THaRICH::ReadRICH() {

  //FIXME: dummies, not used:
  int TOTAL_PADS;
  int TOTAL_PADS_X,TOTAL_PADS_Y;   //the total number of the pads 
                                   //in a row and a column

  // FIXME: Replace the following with code that reads from database

  ifstream infile( "rich.dat" );
  char line[100];
  int i=1;
  while( infile.getline(line,100) ) {
    istrstream sline(line,infile.gcount());
    while ( sline.good() ) {
      switch(i) {
      case 1:  sline >> L_RAD;        break;
      case 2:  sline >> l_quartz;     break;
      case 3:  sline >> l_gap;        break;
      case 4:  sline >> l_emission;   break;
      case 5:  sline >> n_radiator;   break;
      case 6:  sline >> n_quartz;     break;
      case 7:  sline >> n_gap;        break;
      case 8:  sline >> TOTAL_PADS;   break;  //not used - hardcoded in header
      case 9:  sline >> TOTAL_PADS_X; break;  //not used
      case 10: sline >> TOTAL_PADS_Y; break;  //not used
      case 11: sline >> PADS_X;       break;
      case 12: sline >> PADS_Y;       break;
      case 13: sline >> PAD_SIZE_X;   break;
      case 14: sline >> PAD_SIZE_Y;   break;
      default: break;
      }
      if( sline.good() ) i++;
      if( i>14 ) goto exit;
    }
  }
 exit:
  infile.close();
}

//_____________________________________________________________________________
//***************************************************************************
// void ReadData()
//    ... read the certain number of data set of pad-number and charge
//   for ONE event.
// 
//**************************************************************************
// THIS FUNCTION IS FOR TEST PURPOSES ONLY.  FOR REAL DATA, USE Decode()
//
Int_t THaRICH::ReadData( FILE* infile )
{
  //Read a single event from input file 'infile' into internal hit array.
  //Clear all previous hit information.

  char line[100];
  int count = 0;
  fNHits = 0;             //Clear previous hits.

  fgets(line,100,infile);
  istrstream sline(line,100);
  sline >> count;
  if( sline.good() ) {
    if( fDebug > 0 ) printf( "*****Hit count: %5d\n", count );
  } else {
    return 0;
  }
  
  if( count >= kTOTAL_PADS ) {
    Warning("ReadData()", "Too many hits! Event skipped.");
    fNHits = 0;
    return 0;
  }

  for( int i=0; i<count; i++ ) {
    int i_pad, j_pad; 
    float charg;
    fgets(line,100,infile);
    sline.clear(); sline.seekg(0);
    sline >> i_pad >> j_pad >> charg;
    if( !sline.good() ) break;
    Padn2xy( i_pad, j_pad, charg );
    if( fDebug > 0 ) printf("%5d %5d %5f\n",i_pad,j_pad,charg);
  }

  return fNHits;
}

//__________________________________________________________________________
//**************************************************************************
// Void Padn2xy( int i, int pad, int charg )
//   Define the ith fHits (X and Y coordinate, I and J (serial number  
//   in X and in Y raw respectively, charge, serial number from the pad  
//   number (pad) and the charge collected (charg).
//      
//
//  This function calls the functions "GetPadX" and "GetPadY".
//  
//      
//**************************************************************************
void THaRICH::Padn2xy( Int_t pad, Int_t charg ) 
{
  //Add a hit to the internal hit array, given pad number and charge.

  Int_t i = fNHits;
  fHits[i].SetI( GetPadX(pad) );
  fHits[i].SetJ( GetPadY(pad) );
  fHits[i].SetX(( GetPadX(pad)-1)*PAD_SIZE_X + PAD_SIZE_X/2 );
  fHits[i].SetY(( GetPadY(pad)-1)*PAD_SIZE_Y + PAD_SIZE_Y/2 );
  fHits[i].SetADC( charg );
  fHits[i].SetNumber( i );
  fHits[i].SetFlag( 0 );
  fNHits++;
}  
  
//__________________________________________________________________________
//**************************************************************************
// void Padn2xy( int i, int i_pad, int j_pad, float charg )
//   Define the ith fHits (X and Y coordinate, I and J (serial number  
//   in X and in Y raw respectively, charge, serial number from the pad  
//   number (pad) and the charge collected (charg).
//      
//
//**************************************************************************
void THaRICH::Padn2xy( Int_t i_pad, Int_t j_pad, Float_t charg ) 
{
  //Add a hit to the internal hit array, given i and j indices.

  Int_t i = fNHits;
  fHits[i].SetI( i_pad );
  fHits[i].SetJ( j_pad );
  fHits[i].SetX( (i_pad-1)*PAD_SIZE_X + PAD_SIZE_X/2 );
  fHits[i].SetY( (j_pad-1)*PAD_SIZE_Y + PAD_SIZE_Y/2 );
  fHits[i].SetADC( static_cast<Int_t>( charg ));
  fHits[i].SetNumber( i );
  fHits[i].SetFlag( 0 );
  fNHits++;
}  

//__________________________________________________________________________
void THaRICH::DeleteClusters()
{
  //Delete all clusters

  THaRICHCluster* theCluster = fClusters;
  for( int k=0; k<fNClusters; k++, theCluster++ ) {
    theCluster->Clear();           //Clear the list of hits for each cluster
  }
  fNClusters = 0;
  return;
}

//__________________________________________________________________________
Int_t THaRICH::FindClusters()
{
  // Group the hits that are currently in the array into clusters.
  // Return number of clusters found.
 
  // Delete existing clusters

  DeleteClusters();

  // minimum distance between two pads.
  const float par1 = 2.0; 

  // maximum distance in X between two fired pads to be in the same cluster.
  float par2 = PAD_SIZE_X+0.1;  

  // maximum distance in Y between two fired pads to be in the same cluster.
  float par3 = PAD_SIZE_Y+0.1;  

  THaRICHHit*     theHit      = fHits;
  THaRICHCluster* theCluster  = fClusters;

  for( int k=0; k<fNHits; k++, theHit++ ) {

    // HitFlag not equal 0: The Hit was alredy processed
    // HitFlag equal 0: insert the Hit as first element of the cluster

    if( theHit->GetFlag() == 0 ) {

      theHit->SetFlag(1);
      theCluster->Insert( theHit );
      
      //---Scanning all the hit pads

      int flag = 0;
      while( flag==0 ) {
	flag = 1;
	THaRICHHit* hit = fHits;
	for( int i=0; i<fNHits; i++, hit++ ) {
	  if( hit->GetFlag() == 0 && 
	      theCluster->Test( hit, par1, par2, par3 )) {
	      
	    // the hit belongs to the Cluster

	    theCluster->Insert( hit );

	    //tagging to avoid double processing
	    hit->SetFlag(1);  

	    flag = 0;
	    // at least one new element in the Cluster; check again.
	    // FIXME: oi wei ... this takes TIME! Here's room for improvement.
	  }
	}
      }
      fNClusters++;
      theCluster++;
    }
  }
  return fNClusters;
}

//_____________________________________________________________________________
Int_t THaRICH::FindMIP( const TClonesArray& tracks )
{
  // Find the MIPs among the clusters. A MIP is a cluster generated 
  // by a track rather than by Cherenkov photons. Normally every track 
  // crossing the RICH plane is associated with a MIP.
 
  // Clear the list of MIPs.

  delete [] fMIP; fMIP = 0;

  // Quit if no tracks or no clusters. Nothing to do.

  Int_t ntracks = tracks.GetLast()+1;
  if( ntracks == 0 || fNClusters == 0 ) return 0;

  // closeClusters[] contains pointers to clusters within the search radius
  // around a given track.

  THaRICHCluster** closeClusters = new THaRICHCluster*[ fNClusters ];
  fMIP  = new THaRICHCluster* [ ntracks ];
  Int_t nfound = 0;

  // Find the closeClusters for each track.

  for( Int_t k=0; k<ntracks; k++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks.At(k) );
    if( theTrack == NULL ) continue;

    // Find the coordinates of the track crossing point in the RICH plane.
    // This is accomplished using elementary vector algebra. Variables:
    // t0:       Track origin
    // tv:       Unit vector along track direction
    // fDenom:   Denominator matrix. Columns 0 & 1 initialized in Init()
    // fNom:     Nominator matrix. Dto.
    // det:      Determinant of denominator matrix. Zero means no solution
    //           (i.e. track parallel to plane).
    // t:        Parameter indicating location of crossing point along track.
    // v:        Vector from RICH plane origin to track crossing point
    // trackX/Y: Coordinates of the crossing point in the RICH system.

    TVector3 t0( theTrack->GetX(), theTrack->GetY(), 0.0 );
    TVector3 tv( theTrack->GetPx(), theTrack->GetPy(), theTrack->GetPz() );
    fDenom.SetColumn( -tv, 2 );
    fNom.SetColumn( t0-fOrigin, 2 );
    Float_t det = fDenom.Determinant();
    if( fabs(det) < 1e-5 ) continue;  // No useful solution for this track
    Float_t t = fNom.Determinant() / det;
    TVector3 v = t0 + t*tv - fOrigin;
    Float_t trackX = v.Dot(fXax);
    Float_t trackY = v.Dot(fYax);
    
    THaRICHCluster* theCluster     = fClusters;
    THaRICHCluster* minDistClust   = NULL;
    THaRICHCluster* maxChargeClust = NULL;
    THaRICHCluster* theMIP         = NULL;
    Float_t          maxcharge      = 0.0;
    Float_t          mindist        = 1e10;
    Int_t            ncloseClusters = 0;

    for( int j=0; j<fNClusters; j++, theCluster++) {

      // For any given track, find the cluster(s) within the
      // search radius fMaxdist around the point where the
      // track crosses the RICH plane.

      Float_t dx    = theCluster->GetXcenter() - trackX;
      Float_t dy    = theCluster->GetYcenter() - trackY;
      Float_t dist2 = dx*dx + dy*dy;
      if( dist2 < fMaxdist2 ) {
	closeClusters[ ncloseClusters++ ] = theCluster;
	Float_t charge  = theCluster->GetCharge();
	if( charge > maxcharge ) {
	  maxChargeClust = theCluster;
	  maxcharge      = charge;
	}
	if( dist2 < mindist ) {
	  minDistClust = theCluster;
	  mindist      = dist2;
	}	  
      }
    }//end loop over clusters

    // If there is only one closeCluster, it is the MIP

    if( ncloseClusters == 1 ) {
      theMIP = closeClusters[0];
    } 

    // otherwise choose the closeCluster with the largest charge

    else if( ncloseClusters > 1 ) {
      theMIP = maxChargeClust;
    }

    // Save the MIP found for this track, if any

    fMIP[k] = theMIP;
    if( theMIP ) {
      nfound++;
      theMIP->MakeMIP();
    }	

  } //end loop over tracks

  delete [] closeClusters;
  return nfound;
}

//**************************************************************************
// The function for calculating "phi_photon";the azimuthal angle of 
// the C-photon.
//**************************************************************************
Float_t THaRICH::Get_phi_photon( Float_t x_photon, Float_t y_photon,
				 Float_t x_mip, Float_t y_mip,
				 Float_t theta_mip, Float_t phi_mip )
{
  //calculation for "phi_photon"
  Float_t answer = 0.0;
  float term1  = tan(theta_mip);
  float term2  = sin(phi_mip);
  float term3  = cos(phi_mip);
  float length = L_RAD+l_quartz+l_gap;

  float x_origin = x_mip - length*term1*term3;  //the entrance point of the MIP
  float y_origin = y_mip - length*term1*term2;

  float ypart = y_photon - y_origin - l_emission*term1*term2;
  float xpart = x_photon - x_origin - l_emission*term1*term3;
  float hyp   = sqrt( xpart*xpart + ypart*ypart );
  if(fabs(xpart) > hyp) {
    // it should happen only for ypart << xpart because of calculation rounding
    float ratio = xpart/hyp;
    cout << "WARNING xpart/hyp = " << ratio << 
      ", (its module should be < 1)" << endl;
    answer = 0.0;
  } else {
    answer = acos(xpart/hyp);
  }
  if(ypart<0 && xpart>0) answer = - answer + 2.*M_PI;
  if(ypart<0 && xpart<0) answer = - answer + 2.*M_PI;
  //***These 2 lines above are to get around the problem with "arccos".
  //If these are missing, phi_photon calculation will return wrong result. 

  if( fDebug > 2 ) {
    printf("********************************************\n");
    printf("* CHERENKOV ANGLE RECONSTRUCTION           *\n");
    printf("********************************************\n");
    printf("***phi_photon***\n");
    printf("   phi_photon = atan((%f - %f*%f*%f) \n",
	   y_photon,l_emission,term1,term2);
    printf("           / (%f - %f*%f*%f)\n",x_photon,l_emission,term1,term3); 
    printf("   phi_photon = %f DEG \n",answer/M_PI*180);
  }

  return answer;
}
//___________________________________________________________________________

//***************************************************************************
//the function for computing "dist_a";the distance between the MIP impact pad
//and the origin.
//***************************************************************************
Float_t THaRICH::Get_a( Float_t theta_mip ) 
{
  float answer = (L_RAD - l_emission + l_quartz + l_gap) * tan(theta_mip);

  if( fDebug > 2 ) {
    printf("***dist_a : MIP-origin***\n");
    printf("   dist_a = (%f - %f + %f + %f)*tan(%f)\n"
	   ,L_RAD,l_emission,l_quartz,l_gap,theta_mip/M_PI*180);
    printf("   dist_a = %f mm\n",answer);
  }

  return answer;
}

//___________________________________________________________________________
//***************************************************************************
//The function for computing "dist_b";distance between the photon pad and 
//the origin on the pad plane that is refered as "b" in [1] and
//for computing "dist_R";the distance between the photon pad 
//and the MIP impact pad.

//This function calls : Get_a
//                      Get_phi_photon
//***************************************************************************
Float_t THaRICH::Get_b( Float_t x_photon, Float_t y_photon,
			Float_t x_mip, Float_t y_mip,
			Float_t theta_mip, Float_t phi_mip)
{
  float dist_a = Get_a(theta_mip);

  float term1  = x_photon - x_mip;
  float term2  = y_photon - y_mip;
  float dist_R = sqrt( term1*term1 + term2*term2 );

  float phi_photon = Get_phi_photon
    (x_photon,y_photon,x_mip,y_mip,theta_mip,phi_mip);

  if( fDebug > 0 ) 
    printf("dist_a = %f , dist_R = %f , phi_photon = %f\n",dist_a,
	   dist_R, phi_photon);

  //Computing "dist_b(answer)"

  float term3 = 0.0;
  float term4 = 0.0; 
  //cout << "phi_photon " << phi_photon << endl;
    
  if(fabs(cos(phi_photon)) > 0.0003) 
    term3 = (term1 + dist_a*cos(phi_mip))/cos(phi_photon);
  if(fabs(sin(phi_photon)) > 0.0003) {
    term4 = (term2 + dist_a*sin(phi_mip))/sin(phi_photon);    
    if(term3 == 0) {
      term3 = term4; // phi = 90 degree.
    }
  } else
    term4 = term3; // phi = 0 degree.

  Float_t answer = (term3 + term4)/2.0;

  // average between the two (in principle identical) values.

  if( fDebug > 0 ) {
    printf("term3 term4 ");
    printf("%4f,%4f",term3,term4);
    if( fDebug > 2 ) {
      printf("***dist_R: photon-MIP ***\n");
      printf("dist_R = sqrt(%f^2 + %f^2)\n",term1,term2);
      printf("dist_R = %f mm\n",answer);
      printf("***dist_b: photon-origin***\n");
      printf("   dist_b = %f mm\n",answer);
    }
  }

  return answer;
}

//___________________________________________________________________________
//***************************************************************************
//The function for computing the "theta_photon"; the polar angle of the photon
//with respect NOT to the MIP, but to the z-axis that is parpendicular to the
//pad plane.
//This function calls :  Get_b()
//***************************************************************************
Float_t THaRICH::Get_theta_photon(Float_t x_photon, Float_t y_photon,
				  Float_t x_mip, Float_t y_mip,
				  Float_t theta_mip, Float_t phi_mip) 
{
  float TOLERANCE=0.0001;  //tolerance in the precision of calculating 
  float term1=0.0,term2=0.0,term3=0.0;  //for clarity of calculation
  float sq1,sq2;  //terms in square root
  float nr2 = n_radiator*n_radiator;
  float nq2 = n_quartz*n_quartz;
  float ng2 = n_gap*n_gap;

  //INITIALIZING
  //used in computing the "theta_photon" as a candidate for 
  //the "dist_b"
  float trial = 0.0;

  //arbitrary
  float theta_photon = 45./180.*M_PI;   
  
  //upper and lower value of BRACKETING(calculating)
  float upper = M_PI/2.;
  float lower = 0.0;

  float dist_b = Get_b(x_photon, y_photon, x_mip, y_mip, theta_mip, phi_mip);
  
  //Loop for computing "theta_photon" solving the eq. for the "dist_b"

  //cout << dist_b << endl;
  if(dist_b > 300.) 
    {TOLERANCE = 0.0002;}
  if(dist_b > 400.)
    { TOLERANCE = 0.0003;}
  if(dist_b > 500.)
    {TOLERANCE = 0.0004;}
  if(dist_b > 550.)
    {TOLERANCE = 0.0005;}
  if(dist_b > 600.)
    {TOLERANCE = 0.0007;}
  int IterationNumber = 0;

  // due to precision problems in solving the equation for theta_photon 
  // (see below), increase TOLERANCE when dist_b is big otherwise the program 
  //will loop without an end !!

  while((dist_b-trial)>TOLERANCE | (trial-dist_b)>TOLERANCE ) {
    IterationNumber ++;
    //pre-calculation
    sq1=nq2-nr2*sin(theta_photon)*sin(theta_photon);
    sq2=ng2-nr2*sin(theta_photon)*sin(theta_photon);
    if (sq1 < 0.0001 || sq2 < 0.0001) {
      //avoid doing sqrt(negative) or sq1/sq2 equal to zero.
      while(sq1 < 0.0001 || sq2 < 0.0001) {
	theta_photon = theta_photon - TOLERANCE;
	sq1=nq2-nr2*sin(theta_photon)*sin(theta_photon);
	sq2=ng2-nr2*sin(theta_photon)*sin(theta_photon);
      }
    }
    term1 = (L_RAD-l_emission)*tan(theta_photon);
    term2 = l_quartz*n_radiator*sin(theta_photon)/sqrt(sq1);
    term3 = l_gap*n_radiator*sin(theta_photon)/sqrt(sq2);
    if( fDebug > 1 ) {
      printf("term2=%f term3=%f\n",term2,term3);
    }
    trial =  term1 + term2 + term3;
 
    if(trial > dist_b) {
      upper = theta_photon;
      theta_photon = theta_photon - (upper - lower)/2;
    } else {
      lower = theta_photon;
      theta_photon = theta_photon + (upper - lower)/2;
    }
    if(IterationNumber > 1000.) {
      cout << "LOOP ! Trial = " << trial << ", dist_b = " << dist_b << endl;
      cout <<  "upper = " << upper << ", lower = " << lower << endl;
      break;
    }
  }

  if( fDebug > 0 )
    printf(" theta photon = %5f ",theta_photon);

  if( fDebug > 2 ) {
    printf("***theta_photon***\n");
    printf("   theta_photon = %f DEG\n",theta_photon/M_PI*180);
    printf("  'dist_b' calculated with theta_photon obtained above is...\n");
    printf("   %f + %f + %f = %f : should be as close as original 'dist_b:%f'\n"
	   ,term1,term2,term3,trial,dist_b);
  }

  return theta_photon;
}


//_____________________________________________________________________________
//**************************************************************************
//float RecoAng(v1,v2,v3,v4,v5,v6)
//    float  v1: x-coordinate of the C-photon hit-pad (mm)
//    flaot  v2: y-coordinate of the C-photon hit-pad (mm)
//    float  v3: x- of the MIP hit-pad (mm)
//    float  v4: y- of the MIP hit-pad (mm)
//    float  v5: the polar angle of the MIP (rad)
//    float  v6:the azimuthal angle of the MIP (rad)
//  NOTE: Mind the unit of each parameter is correct.
//              
// This function performe a series of calculation to return the reconstructed
// Cherenkov angle(deg). 
//**************************************************************************

Float_t THaRICH::RecoAng( Float_t x_photon, Float_t y_photon,
			  Float_t x_mip, Float_t y_mip,
			  Float_t theta_mip, Float_t phi_mip) 
{
  Float_t ReturnValue = -100.0;

  Float_t theta_photon = Get_theta_photon(x_photon,y_photon,x_mip,y_mip,
					  theta_mip,phi_mip);
  Float_t phi_photon = Get_phi_photon
    (x_photon,y_photon,x_mip,y_mip,theta_mip,phi_mip);
  //NOTE: the variable "phi_photon" required in the next is defined 
  //      during the procedure for theta_photon.

  //The last calculation to get the reconstructed angle.

  Float_t term1 = sin(theta_mip)*sin(theta_photon);
  Float_t term2 = cos(phi_photon-phi_mip);
  Float_t term3 = cos(theta_mip);
  Float_t term4 = cos(theta_photon);
  Float_t term5 = term1*term2 + term3*term4;
  
  if( fabs(term5)<=1.0 ) 
    ReturnValue = acos(term5);

  if( fDebug>0 ) {
    printf(" cherenkov angle = %5f ",ReturnValue);
    printf("  %5f ",phi_photon);
    printf("  %5f ",ReturnValue);
    //printf("\n");
  }

  return ReturnValue;
}

//_____________________________________________________________________________
Int_t THaRICH::CoarseProcess( TClonesArray& tracks )
{
  // Coarse processing of the RICH. We do nothing here since we
  // need precise tracking to be able to do anything useful.

  return 0;
}

//_____________________________________________________________________________
Int_t THaRICH::FineProcess( TClonesArray& tracks )
{
  // The main RICH processing method. Here we
  //  
  // - Identify clusters
  // - Find the MIP(s) among the clusters
  // - Calculate photon angles
  // - Calculate particle probabilities

  if( FindClusters() == 0 )
    return -1;
  if( FindMIP( tracks ) == 0 ) 
    return -2;

  // For each cluster, find the associated MIP.
  // This is equivalent to identifying the Cherenkov rings.
  // FIXME: Here we make a crude approximation: for each cluster,
  // find the *closest* MIP.  This is correct only in the case of 
  // non-overlapping rings.

  Int_t last_imip = -1;
  Float_t x_mip = 0.0, y_mip = 0.0, p_mip = 0.0;
  Float_t theta_mip = 0.0, phi_mip = 0.0;
  THaTrack* MIPtrack = NULL;

  THaRICHCluster* theCluster = fClusters;
  for( int k = 0; k < fNClusters; k++, theCluster++ ) {

    if( theCluster->IsMIP() ) continue;  // ignore MIPs

    Float_t mindist         = 1e10;
    THaRICHCluster* theMIP  = NULL;
    Int_t imip              = 0;
    for( int i = 0; i < tracks.GetLast()+1; i++ ) {

      if( !fMIP[i] ) continue;
      Float_t dist = theCluster->Dist( fMIP[i] );
      if( dist < mindist ) {
	mindist = dist;
	theMIP = fMIP[i];
	imip = i;
      }
    }
    theCluster->SetMIP( theMIP );
    if( !theMIP ) continue;

    if( imip != last_imip ) {
      last_imip = imip;
      x_mip = theMIP->GetXcenter();
      y_mip = theMIP->GetYcenter();

      // Get momentum and angle of the track associated with each MIP

      MIPtrack = static_cast<THaTrack*>( tracks.At(imip) );
      TVector3 tv( MIPtrack->GetPx(), 
		   MIPtrack->GetPy(), 
		   MIPtrack->GetPz() );
      p_mip = MIPtrack->GetP();
      if( p_mip == 0.0 ) continue;  // Urgh
      tv *= 1.0/p_mip;   // Unit vector along track direction
    
      // MIP angles are in spherical coordinates in RICH system, units rad

      theta_mip = acos( fZax.Dot(tv) );
      phi_mip   = acos( (tv-fZax.Dot(tv)*fZax).Dot(fXax) );
    }      
    theCluster->SetTrack( MIPtrack );

    Float_t  x_photon = theCluster->GetXcenter();
    Float_t  y_photon = theCluster->GetYcenter();
    Float_t  angle = RecoAng( x_photon, y_photon,
			      x_mip, y_mip, theta_mip, phi_mip );
    theCluster->SetAngle( angle );
  }

  // FIXME: fill a PID structure here and associate it with MIPtrack...

  return 0;
}



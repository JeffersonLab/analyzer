// Modified version of hsimple.cxx for VDC simulation
// Ken Rossato 6/3/04
// Modified by Amy Orsborn 6/7/05

// UNITS: distances: m
//        times: ns
//        angles: tan angle
//        mass: 1 (momentum == velocity)

#include "THaVDCSim.h"

#ifndef __CINT__
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TNtuple.h"
#include "TRandom.h"
#include "TVector3.h"
#include "TString.h"
#include "TMath.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <fstream>

using namespace std;

int vdcsimgen( THaVDCSimConditions* s );
void getargs( int argc, char*argv[], THaVDCSimConditions* s );
void usage();

static bool verbose = false;

//______________________________________________________________________________
int main(int argc, char *argv[])
{
  //declare simulation conditions class
  THaVDCSimConditions *s = new THaVDCSimConditions;

  getargs( argc, argv, s );
  int ret = vdcsimgen( s );

  delete s;
  return ret;
}
#endif

void usage()
{
  printf("%s \n", "Usage: %s");
  puts(" -s <Output File Name> Default is 'vcdtracks.root'");
  puts(" -f <Textfile Name> Must be of form name.data. Default is 'trackInfo.data'");
  puts(" -a <Number of trials> Controls number of events generated. Default is 10,000");
  puts(" -d <Database file name> Default is '/u/home/orsborn/vdcsim/DB/20030415/db_L.vdc.dat' ");
  puts(" -n <Noise> Chamber drift time resolution (ns) (sigma). Default is 4.5");
  puts(" -e <Wire Efficiency> (In decimal form). Default is 0.999");
  puts(" -r <emission rate> Target's Emission Rate (IN kHz). Default is 2 kHz");
  puts(" -c <Chamber Noise> Probability of random wires firing within the chamber. Default is 0.0");
  puts(" -t <tdc time window> Length of time the TDCs can collect data (in ns). Default is 900");
  puts(" -v Verbose mode, print hit and track info for each plane");
  exit(1);
}

//______________________________________________________________________________
void getargs( int argc, char **argv, THaVDCSimConditions* s )
{
  while (argc-- >1){
    char *opt = *++argv;
    if(*opt == '-'){
      while (*++opt != '\0'){
	switch (*opt){
	case 's':
	   if(!*++opt){
	    if (argc-- < 1)
	      usage();
	    opt = *++argv;
	   }
	  s->filename = opt;
	  opt = "?";
	  break;
	case 'f':
	  if(!*++opt){
	    if (argc-- < 1)
	      usage();
	    opt = *++argv;
	  }
	  s->textFile = opt;
	  opt = "?";
	  break;
	case 'a':
	  if(!*++opt){
	    if (argc-- < 1)
	      usage();
	    opt = *++argv;
	  }
	  s->numTrials = atoi(opt);
	  if ( s->numTrials < 0.0 )
	    usage();
	  opt = "?";
	  break;
	case 'd':
	  if(!*++opt){
	    if (argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->databaseFile = opt;
	  opt = "?";
	  break;
	case 'n':
	  if(!*++opt){
	    if(argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->noiseSigma = atof(opt);
	  if( s->noiseSigma < 0.0 )
	    usage();
	  opt = "?";
	  break;
	case 'e':
	  if(!*++opt){
	    if(argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->wireEff = atof(opt);
	  if( s->wireEff > 1.0)
	    usage();
	  opt = "?";
	  break;
	case 'r':
	  if(!*++opt){
	    if(argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->emissionRate = atof(opt);
	  s->emissionRate /= 1000000.0;   //convert from kHz to 1/ns
	  if( s->emissionRate < 0.0)
	    usage();
	  opt = "?";
	  break;
	case 'c':
	  if(!*++opt){
	    if(argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->probWireNoise = atof(opt);
	  if( s->probWireNoise > 1.0 || s->probWireNoise < 0.0)
	    usage();
	  opt = "?";
	  break;
	case 't':
	  if(!*++opt){
	    if(argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->tdcTimeLimit = atof(opt);
	  if( s->tdcTimeLimit < 0.0)
	    usage();
	  opt = "?";
	  break;
	case 'v':
	  verbose = true;
	  break;
	default:
	  usage();
	}
      }
    }
  }
}

//_____________________________________________________________________________
bool IntersectPlaneWithRay( const TVector3& xax, const TVector3& yax,
			    const TVector3& org, const TVector3& ray_start,
			    const TVector3& ray_vect, Double_t& length,
			    TVector3& intersect )
{
  // Find intersection point of plane (given by 'xax', 'yax', 'org') with
  // ray (given by 'ray_start', 'ray_vect'). 
  // Returns true if intersection found, else false (ray parallel to plane).
  // Output is in 'length' and 'intersect', where
  //   intersect = ray_start + length*ray_vect
  // 'length' and 'intersect' must be provided by the caller.

  Double_t nom[9], den[9];
  nom[0] = den[0] = xax.X();
  nom[3] = den[3] = xax.Y();
  nom[6] = den[6] = xax.Z();
  nom[1] = den[1] = yax.X();
  nom[4] = den[4] = yax.Y();
  nom[7] = den[7] = yax.Z();
  den[2] = -ray_vect.X();
  den[5] = -ray_vect.Y();
  den[8] = -ray_vect.Z();

  Double_t det1 = den[0]*(den[4]*den[8]-den[7]*den[5])
    -den[3]*(den[1]*den[8]-den[7]*den[2])
    +den[6]*(den[1]*den[5]-den[4]*den[2]);
  if( fabs(det1) < 1e-5 )
    return false;

  nom[2] = ray_start.X()-org.X();
  nom[5] = ray_start.Y()-org.Y();
  nom[8] = ray_start.Z()-org.Z();
  Double_t det2 = nom[0]*(nom[4]*nom[8]-nom[7]*nom[5])
    -nom[3]*(nom[1]*nom[8]-nom[7]*nom[2])
    +nom[6]*(nom[1]*nom[5]-nom[4]*nom[2]);

  length = det2/det1;
  intersect = ray_start + length*ray_vect;
  return true;
}

//______________________________________________________________________________
int vdcsimgen( THaVDCSimConditions* s )
{
  // Create a new ROOT binary machine independent file.
  // This file is now becoming the current directory.
  TFile hfile(s->filename,"RECREATE","ROOT file for VDC simulation");

  // Define the z-positions of the wire planes
  s->set(s->wireHeight, 0.0, 0.026, 0.3348, 0.3609);
  
  //calculate probability using Poisson distribution
  Double_t probability = 1.0 - exp(-s->emissionRate*s->tdcTimeLimit);

  //hardcode prefixes for wire planes.
  s->Prefixes[0] = "L.vdc.u1";
  s->Prefixes[1] = "L.vdc.v1";
  s->Prefixes[2] = "L.vdc.u2";
  s->Prefixes[3] = "L.vdc.v2";

  Double_t* timeOffsets = new Double_t[4*s->numWires];
 
  //get time offsets and drift velocities from the file (default is 20030415/L.vdc.dat)
  Int_t getoffset = s->ReadDatabase(timeOffsets); 
  if(getoffset == 1) {
    cout << "Error Reading Database file: " << s->databaseFile << endl; 
    exit(1);
  }

  // Various constants. FIXME: should be in a database

  // Wire rotation angles wrt detector coordinates - this is the angle about the z-axis
  // that the u axis is rotated wrt to the x-axis.
  const Double_t wireAngle[4] = { -TMath::Pi()/4.0, TMath::Pi()/4.0,
				  -TMath::Pi()/4.0, TMath::Pi()/4.0 };

  // Wire coordinate offsets and spacings (meters). The offset is the u (v)
  // position of wire #0 of each plane.
  // NOTE: The wires in the Hall A VDCs count backwards. Wires with higher
  // numbers have smaller u (v) positions than wire #0. We account for this
  // by using a negative wire spacing.

  const Double_t wireOffset[4]  = { 0.77852, 0.77852, 1.02718, 1.02718 };
  const Double_t wireSpacing[4] = { -s->cellWidth, -s->cellWidth,
				    -s->cellWidth, -s->cellWidth };

  // Polynomial coefficients used in time-to-distance conversion
  const Double_t fA1tdcCor[4] = { 2.12e-3, 0.0, 0.0, 0.0 };
  const Double_t fA2tdcCor[4] = { -4.20e-4, 1.3e-3, 1.06e-4, 0.0 };

  // Define orientation of the VDC planes. 
  // Allow an arbitrary rotation about the lab y-axis.
  const Double_t rot = 0.0; // Assume VDC exactly horizontal in lab
  const TVector3 vdcx( TMath::Cos(rot), 0.0, TMath::Sin(rot) );
  const TVector3 vdcy( 0.0, 1.0, 0.0 );
  const TVector3 vdcz = vdcx.Cross(vdcy);
  TVector3 planeOrigin[4];
  for( int j=0; j<4; j++ )
    planeOrigin[j] += vdcz * s->wireHeight[j];

  // Create some histograms, a profile histogram and an ntuple
  //TH1F *hwire   = new TH1F("hwire","Wire Hit Map",100,-5,400);
  TH1F *hwireU1 = new TH1F("hwireU1", "Wire Hit Map U1 Plane", 100, -5, 400);
  TH1F *hwireV1 = new TH1F("hwireV1", "Wire Hit Map V1 Plane", 100, -5, 400);
  TH1F *hwireU2 = new TH1F("hwireU2", "Wire Hit Map U2 Plane", 100, -5, 400);
  TH1F *hwireV2 = new TH1F("hwireV2", "Wire Hit Map V2 Plane", 100, -5, 400);
  TH1F *hnumwiresU1 = new TH1F("hnumwiresU1", "Number of wires triggered in U1 Plane", 10, 0, 10);
  TH1F *hnumwiresV1 = new TH1F("hnumwiresV1", "Number of wires triggered in V1 Plane", 10, 0, 10);
  TH1F *hnumwiresU2 = new TH1F("hnumwiresU2", "Number of wires triggered in U2 Plane", 10, 0, 10);
  TH1F *hnumwiresV2 = new TH1F("hnumwiresV2", "Number of wires triggered in V2 Plane", 10, 0, 10);
  TH1F *htimeV1 = new TH1F("htimeV1", "Drift Times for V1 Plane", 200, 0, 0.0000005);
  TH1F *hdistV1 = new TH1F("hdistV1", "Drift Distances for V1 Plane",200, 0, 0.02); 
  TH2F *horigin = new TH2F("horigin","Origin Y vs. X",100,-1.2,1.2,100,-.1,.1);
  TH1F *hdrift = new TH1F("hdrift","Drift Time", 100, 1000, 2000);
  TH1F *hdriftNoise = new TH1F ("hdriftNoise", "Drift Time (With Noise)", 100, 1000, 2000);
  TH2F *hdrift2 = new TH2F("hdrift2", "Drift Time vs Num Wires in Cluster",
			   6, 3, 9, 100, 1000, 2000);
  TH1F *hdeltaT4 = new TH1F ("hdeltaT4", "Relative Time (4 hits)", 200, -100, 100);
  TH1F *hdeltaTNoise4 = new TH1F ("hdeltaTNoise4", "Relative Time (4 hits w/ Noise)", 200, -100, 100);
  TH1F *hdeltaT5 = new TH1F ("hdeltaT5", "Relative Time (5 hits)", 200, -100, 100);
  TH1F *hdeltaTNoise5 = new TH1F ("hdeltaTNoise5", "Relative Time (5 hits w/ Noise)", 200, -100, 100); 
  TH1F *hdeltaT6 = new TH1F ("hdeltaT6", "Relative Time (6 hits)", 200, -100, 100);
  TH1F *hdeltaTNoise6 = new TH1F ("hdeltaTNoise6", "Relative Time (6 hits w/ Noise)", 200, -100, 100);
  TH1F *hnumtracks = new TH1F ( "hnumtracks", "Number of Tracks Per Event", 5, 0, 5);

  THaVDCSimEvent *event = new THaVDCSimEvent;
  TTree *tree = new TTree("tree","VDC Track info");
  //TBranch *eventBranch = tree->Branch("event", "THaVDCSimEvent", &event);
  // avoid unused variable warning ;)
  tree->Branch("event", "THaVDCSimEvent", &event);

  //open text file to put track info into
  bool do_text = !(s->textFile.IsNull());
  ofstream textFile;
  if( do_text ) {
    textFile.open(s->textFile, ios::out);
    if(!textFile.is_open())
      cout << "Error Opening Text File\n";

    //write important sim conditions to text file
    textFile << "******Simulation Conditions******\n"
	     << "Output File Name = " << s->filename << endl
	     << "Database File Read = " << s->databaseFile << endl
	     << "Number of Trials = " << s->numTrials << endl
	     << "Noise Sigma = " << s->noiseSigma << " ns\n"
	     << "Wire Efficiency = " << s->wireEff << endl
	     << "Emission Rate = " << s->emissionRate << " particle/ns\n"
	     << "TDC Time Window = " << s->tdcTimeLimit << " ns\n"
	     << "Probability of Random Wire Firing = " << s->probWireNoise << "\n\n";
  }

  //  Int_t numSecondTracks = 0, numThirdTracks = 0;
  Int_t numRandomWires = 0;
  //create numTrials simulated tracks
  for ( Int_t i=0; i< s->numTrials; i++) {

    //set track type, track number, and event number each time loop executes
    Int_t tracktype = 0, tracknum = 0;
    event->event_num = i + 1;
    if( do_text )
      textFile << "\nEvent: " << event->event_num << endl 
	       << "***************************************\n";

  newtrk:
    THaVDCSimTrack *track = new THaVDCSimTrack(tracktype,tracknum);

    // create randomized origin and momentum vectors for trigger and coincident tracks
    if(track->type >= 0 && track->type <= 2){
      track->origin.SetXYZ(gRandom->Rndm(1)*(s->x2-s->x1) + s->x1, 
			   gRandom->Gaus(s->ymean, s->ysigma), s->z0);
      track->momentum.SetXYZ(0,0,s->pmag0);
      track->momentum.SetTheta(gRandom->Gaus(s->pthetamean, s->pthetasigma));
      track->momentum.SetPhi(gRandom->Gaus(s->pphimean, s->pphisigma));
    }

    //write track info to text file
    if( do_text )
      textFile << "Track number = " << tracknum << ", type = " << tracktype << endl
	       << "Origin = ( " << track->origin.X() << ", " << track->origin.Y() << ", " 
	       << track->origin.Z() << " )" << endl
	       << "Momentum = ( " << track->momentum.X() << ", " 
	       << track->momentum.Y() <<", " << track->momentum.Z() << " )\n";

    //Fill histogram with origin position
    horigin->Fill(track->X(), track->Y());

    //generate a time offset for the track
    if(track->type == 0)
      track->timeOffset = 0.0;
    //FIXME!! the linear approximation isn't correct!
    else if(track->type == 1)
      track->timeOffset = gRandom->Rndm(1)*s->tdcTimeLimit; 
    //if it's a third track, increase the time offset by random number
    //(i.e. must be after 2nd track)
    else if(track->type == 2)
      track->timeOffset += gRandom->Rndm(1)*s->tdcTimeLimit;
    //****Note******* 
    //Here we use a linear approximation of the exponential 
    //probability of s->timeOffset. 
    //Assuming low emission rates (< 10kHz) and time windows of ~100 ms, this is acceptable.

    //fill track information
    event->tracks.Add(track);

    //run through each plane (u1, v1, u2, v2)
    for (Int_t j=0; j < 4; j++) 
    {
//       //calculate time it takes to hit wire plane and the position at which it hits
//       Double_t travelTime = (s->wireHeight[j]-track->origin.Z())/track->momentum.Z();
//       TVector3 position = track->origin + travelTime*track->momentum;

      // Calculate intersection point of track with current wire plane

      Double_t pathlen;
      TVector3 position;
      if( !IntersectPlaneWithRay( vdcx, vdcy, planeOrigin[j],
				  track->origin, track->momentum,
				  pathlen, position ))	continue;

      //Calculate TRANSPORT coordinates of the track w.r.t. the u1 plane coordinates
      if(j == 0){
	TVector3 v(position);
	v -= planeOrigin[j];
	track->ray[0] = v.Dot(vdcx);
	track->ray[1] = v.Dot(vdcy);
	if(track->momentum.Dot(vdcz) != 0.0){
	  track->ray[2] = track->momentum.Dot(vdcx)/track->momentum.Dot(vdcz);
	  track->ray[3] = track->momentum.Dot(vdcy)/track->momentum.Dot(vdcz);
	} else{
	  track->ray[2] = 0.0;
	  track->ray[3] = 0.0;
	}	
	track->ray[4] = 0.0;  //By definition - we take the u1 plane as reference
	if( do_text )
	  textFile << "Position (x, y, Theta, Phi, z) = \t( "
		   << track->ray[0] << ",\t" << track->ray[1] << ",\t" 
		   << track->ray[2] << ",\t" << track->ray[3] <<", \t" 
		   << track->ray[4] << " )\n";
      }
      if( do_text )
	textFile << "\nPlane: " << j << endl;

      // compute tanThetaPrime - the angle between the VDC u(v)-axis and the
      // projection of the track into the u(v)-z plane
      // NB: this value can be different for each plane, so we need to recompute
      // it here, inside the loop.
      // FIXME: this assumes that wireAngle = +/- 45 deg
      Double_t tanThetaPrime;
      if(j%2 == 0)
  	tanThetaPrime = sqrt(2.0)/(track->ray[2] - track->ray[3]);
      else
	tanThetaPrime = sqrt(2.0)/(track->ray[2] + track->ray[3]);

      // The cluster slopes are defined wrt the z-axis:
      track->slope[j] = 1.0/tanThetaPrime;

      // Get position into (u,v,z) coordinates for u plane
      // (v,-u,z) coordinates for v plane - in the VDC coordinates, NOT lab
      TVector3 uvpos( (position-planeOrigin[j]).Dot(vdcx),
		      (position-planeOrigin[j]).Dot(vdcy),
		      s->wireHeight[j] );
      uvpos.RotateZ( -wireAngle[j] );
     
      // Find the range of wire numbers that this track hit
      
      // position of intercept wrt to u(v)-origin in terms of wire spacings
      Double_t x = (uvpos.X() - wireOffset[j]) / wireSpacing[j];
      track->xover[j] = uvpos.X();

      // First and last wire number hit. Must round to the nearest integer.
      Int_t wirehitFirst = 
	static_cast<Int_t>(x - s->cellHeight/2.0/tanThetaPrime/
			   TMath::Abs(wireSpacing[j]) + 0.5);
      Int_t wirehitLast = 
	static_cast<Int_t>(x + s->cellHeight/2.0/tanThetaPrime/
			   TMath::Abs(wireSpacing[j]) + 0.5);
      
      // Parameters for time-to-distance conversion
      Double_t a1 = 0.0, a2 = 0.0;
      
      // Find the values of a1 and a2 by evaluating the proper polynomials
      // a = A_3 * x^3 + A_2 * x^2 + A_1 * x + A_0
      for (Int_t i = 3; i >= 1; i--) {
	a1 = tanThetaPrime * (a1 + fA1tdcCor[i]);
	a2 = tanThetaPrime * (a2 + fA2tdcCor[i]);
      }
      a1 += fA1tdcCor[0];
      a2 += fA2tdcCor[0];
      
      if( do_text )
	textFile << "Plane " << j << ": \n";
      
      Int_t numwires = wirehitLast - wirehitFirst + 1;

      bool aWireFailed = false;
      bool hitOutOfBounds = false;
      bool negativeTDC = false;
      Int_t times[numwires];
      Int_t noiseTimes[numwires];         //arrays to hold times (to calc. relative time)
      Double_t distances[numwires];  //array to hold distances
      Int_t counter = -1;
      //initialize arrays to zero
      for(int m = 0; m < numwires ; m++){
	times[m] = 0;
	noiseTimes[m] = 0;
	distances[m] = 0.0;
      }

      //write actually hit wires to text file
      if( do_text ) {
	textFile << "\n\nReal Hits:\nWires Hit: \t\t";
	for(int m = wirehitFirst; m <= wirehitLast; m++)
	  textFile << m << "\t\t";
	textFile << endl << "Hit Times (ns) = \t";
      }

      //loop through each hit wire
      for (Int_t k=wirehitFirst; k<=wirehitLast; k++) 
      {
	// create a new hit
	THaVDCSimWireHit *hit = new THaVDCSimWireHit;

	hit->wirenum = k;
	hit->pos = wireOffset[j] + k*wireSpacing[j];
	counter++;

	// distance from crossing point to wire "k"
	Double_t d = (x - k) * wireSpacing[j];
	// Actual drift distance 
	// - perpendicular distance from wire center to track (meters)
	Double_t d0 = TMath::Abs(d)*tanThetaPrime;
	hit->distance = d0;
	
	// Apply angle-dependent time correction to account for the
	// non-linear dependence of drift time on the drift distance.
	// Inversion of THaVDCAnalyticTTDConv::ConvertTimeToDist
	if (d0 > a1 + a2) 
	  d0 -= a2;
	else 
	  d0 /= (1+a2/a1);
	
	hit->rawTime = d0/s->driftVelocities[j];
	  
	if( do_text )
	  textFile << hit->rawTime << "\t\t";

	//convert rawTime to tdctime
	hit->rawTDCtime = 
	  static_cast<Int_t>( timeOffsets[k+j*s->numWires] - 
			      s->tdcConvertFactor*(hit->rawTime + track->timeOffset) );

	// Correction for the velocity of the track
	// This correction is negligible, because of high momentum
	
	// Ignoring this correction permits momentum to have any
	// magnitude with no effect on the results
	//+ d*sqrt(pow(tanThetaPrime,2)+1)/track->momentum.Mag()
	
	// Correction for ionization drift velocities
	
	// reject data that the noise filter would
	// This shouldn't affect our data at all
	/*
	  if (d0 < -.5 * .013 ||
	  d0 > 1.5 * .013) {
	  delete hit;
	  continue;
	  }
	*/
	
	//time with additional noise to account for random walk nature of ions
	hit->time =
	  static_cast <Int_t>( hit->rawTDCtime + 
			       s->tdcConvertFactor*(gRandom->Gaus(s->noiseMean, s->noiseSigma)) ); 

	//check to assure hit is within bounds	
	if (hit->wirenum < 0 || hit->wirenum > s->numWires - 1){
	  delete hit;
	  hitOutOfBounds = true;
	  continue;
	}
	
	//make sure hit time falls within time tdc is open
	if (hit->time < 0){
	  delete hit;
	  negativeTDC = true;
	  continue;
	}
	
	//simulate efficiency of wires.
	double workFail = gRandom->Rndm(1);
	if(workFail > s->wireEff){
	  delete hit;
	  hit->wireFail = true;
	  aWireFailed = true;
	  continue;
	}
	
	//fill arrays for times if hit still exists
	//note: if hit doesn't exist, value will be default of 0
	times[counter] = hit->rawTDCtime;
	noiseTimes[counter] = hit->time;
	distances[counter] = hit->distance;

	//fill wire hit histograms for each plane
	if(j == 0)
	  hwireU1->Fill(k);
	else if(j == 2)
	  hwireU2->Fill(k);
	else if(j == 3)
	  hwireV2->Fill(k);

	//fill histograms for v1 plane
	if (j == 1) 
	{
	  htimeV1->Fill(hit->rawTime/1000000000.0);
	  hdrift->Fill((hit->rawTDCtime));
	  hdriftNoise->Fill((hit->time));
	  hdrift2->Fill(wirehitLast-wirehitFirst+1, (hit->time));
	  hwireV1->Fill(k);
	  hdistV1->Fill(hit->distance);
	}

	//fill wirehit information
	event->wirehits[j].Add(hit);
	track->hits[j].Add(hit);

      } //closes loop going through each hit wire

      //check for random noise hits and write info to text file
      //only do it if there is actually a probability of wire noise
      if(s->probWireNoise != 0.0){

	if( do_text )
	  textFile << "\nRandom Noise Hits: \nWires Hit: \t\t";
	
	Int_t noiseHitTDCs[static_cast<Int_t>(s->probWireNoise*s->numWires)+1]; 
	Int_t counter = -1;
	for(int n = 0; n < s->numWires; n++){
	  //roll die to see if wire fires
	  double randomFire = gRandom->Rndm(1);
	  if(randomFire <= s->probWireNoise){
	    counter++;
	    
	    //declare new hit class
	    THaVDCSimWireHit *hit = new THaVDCSimWireHit;
	    numRandomWires++;
	    
	    hit->wirenum = n;
	    hit->pos = wireOffset[j] + n*wireSpacing[j];
	    if( do_text )
	      textFile << n << "\t\t";
	    hit->type = 1;
	    
	    //set random tdc time within time window
	    hit->time = static_cast<Int_t>(gRandom->Rndm(1)*s->tdcTimeLimit); 
	    noiseHitTDCs[counter] = hit->time;
	    //NOTE: distance, rawTime, and rawTDCtime will stay as default 0 for these hits
	  
	    //fill wire hit histograms for each plane
	    if(j == 0)
	      hwireU1->Fill(n);
	    else if(j == 2)
	      hwireU2->Fill(n);
	    else if(j == 3)
	      hwireV2->Fill(n);
	    
	    //fill histograms for v1 plane
	    if (j == 1) {
	      hdrift->Fill((hit->time));
	      hdriftNoise->Fill((hit->time));
	      hwireV1->Fill(n);
	    }
	    
	    //fill wirehit information
	    event->wirehits[j].Add(hit);
	    track->hits[j].Add(hit);
	  }//closes if(random wire fires) loop
	  
	}//closes loop running through each wire

	if( do_text ) {
	  textFile << "\nTDC Times: \t\t";
	  for(Int_t n = 0; n <= counter; n++)
	    textFile << noiseHitTDCs[n] << "\t\t";
	}
      }

      // Get number of good wires in this plane for this event
      TList* hitlist = &event->wirehits[j];
      numwires = hitlist->GetSize();

      if( verbose ) {
	cout << "Plane " << j << endl;
	hitlist->Print();
      }

      // go through tlist and get data
      if( do_text ) {
	TIter next(hitlist);
	THaVDCSimWireHit* hit;
	textFile << "Wires Hit = \t\t";
	while( (hit = (THaVDCSimWireHit*)next()) )
	  textFile << hit->wirenum << "\t\t";
	next.Reset();
	textFile << endl << "Hit Times (ns) = \t";
	while( (hit = (THaVDCSimWireHit*)next()) )
	  textFile << hit->rawTime << "\t\t";
	next.Reset();
	textFile << endl << "Raw TDC Times = \t";
	while( (hit = (THaVDCSimWireHit*)next()) )
	  textFile << hit->rawTDCtime << "\t\t";
	next.Reset();
	textFile << endl << "Final TDC Times = \t";
	while( (hit = (THaVDCSimWireHit*)next()) )
	  textFile << hit->time << "\t\t";
	next.Reset();
	textFile << endl << "Drift Distances = \t";
	while( (hit = (THaVDCSimWireHit*)next()) ) {
	  textFile << hit->distance << "\t";
	  if(hit->distance == 0.0)
	    textFile << "\t";
	}

	// Print info about imperfect events
	textFile << endl;
	if(hitOutOfBounds)
	  textFile << "\tHit(s) Out of Bounds\n";
	if(negativeTDC)
	  textFile << "\tNegative TDC Time(s)\n";
	if(aWireFailed)
	  textFile << "\tWire failure(s)\n";

      }

      //set the number of wires that actually were hit 
      //(correcting for failed wires, out of bound hits, negative tdc time hits.)
      numwires = track->hits[j].GetSize();

      //fill number of wire histograms for each plane
      if(j == 0)      
	hnumwiresU1->Fill(numwires);
      else if(j == 1)
	hnumwiresV1->Fill(numwires);
      else if(j == 2)
	hnumwiresU2->Fill(numwires);
      else
	hnumwiresV2->Fill(numwires);

      // calculate relative time and store in histograms for 4, 5, and 6 hits
      // use 10^38 if a wire failed during the hit (since resolution would be completely lost!)
      if(aWireFailed){
	if(numwires == 4){
	  hdeltaTNoise4->Fill(1e38);
	  hdeltaT4->Fill(1e38);
	}
	else if(numwires == 5){
	  hdeltaTNoise5->Fill(1e38);
	  hdeltaT5->Fill(1e38);
	}
        else if(numwires == 6){
	  hdeltaTNoise6->Fill(1e38);
	  hdeltaT6->Fill(1e38);
	}
      }
       else{
	if(numwires == 4)
	  {
	    hdeltaTNoise4->Fill((noiseTimes[0] - noiseTimes[1]) - (noiseTimes[3] - noiseTimes[2]));
	    hdeltaT4->Fill((times[0] - times[1]) - (times[3] - times[2]));
	  }
	else if(numwires == 5)
	  {
	    hdeltaTNoise5->Fill((noiseTimes[0] - noiseTimes[1]) - (noiseTimes[4] - noiseTimes[3]));
	    hdeltaT5->Fill((times[0] - times[1]) - (times[4] - times[3]));
	  }
	else if(numwires == 6)
	  {
	    hdeltaTNoise6->Fill((noiseTimes[0] - noiseTimes[1]) - (noiseTimes[5] - noiseTimes[4]));
	    hdeltaT6->Fill((times[0] - times[1]) - (times[5] - times[4]));
	  }
       }

    }//closes loop going through each plane

    //if target track , decide if there will be coincident track
    if(track->type == 0){
       double secondTrack = gRandom->Rndm(1);
       if( secondTrack <= probability ){
	tracktype = 1;
	tracknum++;
	//numSecondTracks++;
	goto newtrk;  //go back to where new tracks are created, still within the same event

       }
    }
    //if it's a secondary track, decide if there will be tertiary track
    else if(track->type == 1){
      double thirdTrack = gRandom->Rndm(1);
      if( thirdTrack <= probability){
	tracktype = 2;
	tracknum++;
	//numThirdTracks++;
	goto newtrk;   //go back to where new tracks are created, still within same event
      }
    }

    //*******all tracks for the single event have now been generated*******
   
    if( verbose )
      event->tracks.Print();

    //fill histogram of number of tracks per event
    hnumtracks->Fill(tracknum + 1);
    
    //fill the tree with event information
    //    eventBranch->Fill();
    tree->Fill();

    //clean up
    //clear wirehits[j]
    event->Clear();

    //    cout << "I got here\t" << i << endl;
   } //closes loop going through all the trials

  //cout << numSecondTracks << " second tracks\n" << numThirdTracks << " third tracks\n";
  //  cout << numRandomWires << " random wires fired\n";

  //close text file
  if( do_text ) {
    textFile.close();
    if(textFile.is_open())
      cout << "Error Closing Text File\n";
  }

  // Save objects in this file
  tree->Write();
  s->Write("s");

  hwireU1->Write();
  hwireV1->Write();
  hwireU2->Write();
  hwireV2->Write();
  htimeV1->Write();
  hdistV1->Write();
  hnumwiresU1->Write();
  hnumwiresV1->Write();
  hnumwiresU2->Write();
  hnumwiresV2->Write();
  horigin->Write();
  hdrift->Write();
  hdrift2->Write();
  hdriftNoise->Write();
  hdeltaT4->Write();
  hdeltaTNoise4->Write();
  hdeltaT5->Write();
  hdeltaTNoise5->Write();
  hdeltaT6->Write();
  hdeltaTNoise6->Write();
  hnumtracks->Write();
 
  // Close the file.
  hfile.Close();
  delete [] timeOffsets;

  return 0;
}
//______________________________________________________________________________

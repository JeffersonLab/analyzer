///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDC                                                                    //
//                                                                           //
// Simple vertical drift chamber analysis class. It assumes that the chamber //
// consists of two pairs of U and V planes and finds tracks based on a very  //
// simple algorithm: Clusters of hits are identified in each plane, and the  //
// center of each cluster is taken to be the track crossing point. No        //
// fitting of drift times is performed, which greatly improves speed and     //
// simplifies the algorithm, but limits the achievable precision.            //
// This class always finds either zero tracks or exactly one track.          //
//                                                                           //
// Originally written by Armen Ketikyan in the summer of 2000.               //
// Heavily modified for Analyzer v0.60 by Ole Hansen.                        //
//                                                                           //
// This class will be obsoleted after version 0.60.                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDC.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "THaVarList.h"
#include "TDatime.h"
#include "TMath.h"

#include <cstring>
#include <algorithm>

ClassImp(THaVDC)

//FIXME: get rid of fixed-size arrays. max number of hits & clusters should be variable

const int THaVDC::MAXHIT;
const int THaVDC::MAXCLU;

//_____________________________________________________________________________
THaVDC::THaVDC( const char* name, const char* description,
		THaApparatus* apparatus )
  : THaTrackingDetector(name,description,apparatus), fFirstWire(NULL)
{
  // Constructor

  fNelem = NPLANES;  // Number of wire planes
}

//_____________________________________________________________________________
Int_t THaVDC::ReadDatabase( FILE* fi, const TDatime& date )
{
  // Set up data, map and variables for E-Arm VDC planes package.
  // These data come from personal database file "db_VDC.dat", if this file
  // exists in current directory. If database file does not exist in current
  // directory, then default values are set.

  // Read data from personal database file "db_VDC.dat":

  static const char* const here = "ReadDatabase()";
  const int LEN = 100;
  char buf[LEN];

  // Read detector map
  // We assume that there are exactly 4 modules per plane, and that the
  // sequence of planes is U1, V1, U2, V2.
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  int i = 0;
  delete [] fFirstWire;
  fFirstWire = new Int_t[ THaDetMap::kDetMapSize ];
  while (1) {
    Int_t crate, slot, first, last, first_wire;
    fscanf ( fi,"%d%d%d%d%d", &crate, &slot, &first, &last, &first_wire );
    fgets ( buf, LEN, fi );
    if( crate < 0 ) break;
    if( fDetMap->AddModule( crate, slot, first, last ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).", 
	    THaDetMap::kDetMapSize);
      delete [] fFirstWire;
      return kInitError;
    }
    fFirstWire[i++] = first_wire;
  }

  fgets ( buf, LEN, fi );
  fscanf ( fi, "%d", &fMaxgap );                     // Max gap in a cluster 
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%d%d", &fMintime, &fMaxtime );       // Min and Max time limit
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );

  double vdcangle, uwangle, vwangle;  // Angles of VDC plane and wires
  double u1z, v1z, u2z, v2z;          // z positions of planes in U1 cs

  fscanf ( fi, "%lf", &vdcangle );                   // Angle of VDC package
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%lf%lf", &uwangle, &vwangle );       // Angle of U and V wires
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%lf%lf%lf%lf", &u1z,&v1z,&u2z,&v2z); // Z-s with respect to U1
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf(fi,"%lf%lf%lf%lf",&u1wbeg,&v1wbeg,&u2wbeg,&v2wbeg); //1st wires posit
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf(fi,"%lf%lf%lf%lf",&u1wspac,&v1wspac,&u2wspac,&v2wspac);// Wires space

  // Compute some derived quantities for efficiency.

  du2u1 = u2z - u1z;
  dv2v1 = v2z - v1z;
  dv1u1 = v1z - u1z;

  const Double_t degrad = TMath::Pi()/180.0;
  tan_vdc = TMath::Tan(vdcangle*degrad);
  sin_vdc = TMath::Sin(vdcangle*degrad);
  cos_vdc = TMath::Cos(vdcangle*degrad);
  sin_uw  = TMath::Sin(uwangle*degrad);
  cos_uw  = TMath::Cos(uwangle*degrad);
  sin_vw  = TMath::Sin(vwangle*degrad);
  cos_vw  = TMath::Cos(vwangle*degrad);
  sin_uvw = TMath::Sin((vwangle-uwangle)*degrad);

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t THaVDC::SetupDetector( const TDatime& date )
{
  // Initialize global variables and decoder lookup table

  if( fIsSetup ) return kOK;
  fIsSetup = true;

  // Initialize decoder lookup table
  const DataDest tmp[NPLANES] = {
    { &fU1NHIT, fU1WIRE, fU1TIME }, { &fV1NHIT, fV1WIRE, fV1TIME },
    { &fU2NHIT, fU2WIRE, fU2TIME }, { &fV2NHIT, fV2WIRE, fV2TIME }
  };
  memcpy( fDataDest, tmp, NPLANES*sizeof(DataDest));

  // Register variables in global list:

  VarDef vars[] = {
    { "u1.nhit",   "Number of hits on U1 plane",    kInt,    0,      &fU1NHIT },
    { "u1.wire",   "Active wire numbers in U1",     kInt,    MAXHIT, fU1WIRE, &fU1NHIT },
    { "u1.time",   "TDC values of active U1 wires", kFloat,  MAXHIT, fU1TIME, &fU1NHIT },
    { "u1.nclust", "Number of clusters in U1",      kInt,    0,      &fU1NCLUST },
    { "u1.clpos",  "Cluster positions U1 (wire#)",  kFloat,  MAXCLU, fU1CLPOS, &fU1NCLUST },
    { "u1.clsiz",  "Cluster sizes U1 (wires)",      kInt,    MAXCLU, fU1CLSIZ, &fU1NCLUST },
    { "v1.nhit",   "Number of hits on V1 plane",    kInt,    0,      &fV1NHIT },
    { "v1.wire",   "Active wire numbers in V1",     kInt,    MAXHIT, fV1WIRE, &fV1NHIT },
    { "v1.time",   "TDC values of active V1 wires", kFloat,  MAXHIT, fV1TIME, &fV1NHIT },
    { "v1.nclust", "Number of clusters in V1",      kInt,    0,      &fV1NCLUST },
    { "v1.clpos",  "Cluster positions V1 (wire#)",  kFloat,  MAXCLU, fV1CLPOS, &fV1NCLUST },
    { "v1.clsiz",  "Cluster sizes V1 (wires)",      kInt,    MAXCLU, fV1CLSIZ, &fV1NCLUST },
    { "u2.nhit",   "Number of hits on U2 plane",    kInt,    0,      &fU2NHIT },
    { "u2.wire",   "Active wire numbers in U2",     kInt,    MAXHIT, fU2WIRE, &fU2NHIT },
    { "u2.time",   "TDC values of active U2 wires", kFloat,  MAXHIT, fU2TIME, &fU1NHIT },
    { "u2.nclust", "Number of clusters in U2",      kInt,    0,      &fU2NCLUST },
    { "u2.clpos",  "Cluster positions U2 (wire#)",  kFloat,  MAXCLU, fU2CLPOS, &fU2NCLUST },
    { "u2.clsiz",  "Cluster sizes U2 (wires)",      kInt,    MAXCLU, fU2CLSIZ, &fU2NCLUST },
    { "v2.nhit",   "Number of hits on V2 plane",    kInt,    0,      &fV2NHIT },
    { "v2.wire",   "Active wire numbers in V2",     kInt,    MAXHIT, fV2WIRE, &fV2NHIT },
    { "v2.time",   "TDC values of active V2 wires", kFloat,  MAXHIT, fV2TIME, &fV2NHIT },
    { "v2.nclust", "Number of clusters in V2",      kInt,    0,      &fV2NCLUST },
    { "v2.clpos",  "Cluster positions V2 (wire#)",  kFloat,  MAXCLU, fV2CLPOS, &fV2NCLUST },
    { "v2.clsiz",  "Cluster sizes V2 (wires)",      kInt,    MAXCLU, fV2CLSIZ, &fV2NCLUST },
    { "ntracks",   "Number of tracks (0 or 1)",     kInt,    0,      &fTRN },
    { "x",         "Track x coordinate (cm)",       kFloat,  0,      &fTRX },
    { "y",         "Track y coordinate (cm)",       kFloat,  0,      &fTRY },
    { "z",         "Track z coordinate (cm)",       kFloat,  0,      &fTRZ },
    { "theta",     "Tangent of track theta angle",  kFloat,  0,      &fTRTH },
    { "phi",       "Tangent of track phi angle",    kFloat,  0,      &fTRPH },
    { 0, 0, 0, 0, 0 }
  };
  DefineVariables( vars );

  return kOK;
}

//_____________________________________________________________________________
THaVDC::~THaVDC()
{
  // Destructor. Removes global variables.

  if( fIsSetup )
    RemoveVariables();
  delete [] fFirstWire;
}

//_____________________________________________________________________________
inline
void THaVDC::ClearEvent()
{
  // Reset all local data to prepare for next event.

  const int lh = MAXHIT*sizeof(Int_t);
  const int lf = MAXHIT*sizeof(Float_t);
  const int lc = MAXCLU*sizeof(Int_t);
  const int ld = MAXCLU*sizeof(Float_t);
  fU1NHIT = 0;                            // Number of hits on plane U1
  memset( fU1WIRE, 0, lh );               //   Array of wires numbers
  memset( fU1TIME, 0, lf );               //   Array of corresponding TDCs
  fU1NCLUST = 0;                          //   Number of clusters on U1
  memset( fU1CLPOS, 0, ld );              //   Array of clusters positions
  memset( fU1CLSIZ, 0, lc );              //   Array of clusters sizes
  fV1NHIT = 0;                            // Number of hits on plane V1
  memset( fV1WIRE, 0, lh );               //   Array of wires numbers
  memset( fV1TIME, 0, lf );               //   Array of corresponding TDCs
  fV1NCLUST = 0;                          //   Number of clusters on V1
  memset( fV1CLPOS, 0, ld );              //   Array of clusters positions
  memset( fV1CLSIZ, 0, lc );              //   Array of clusters sizes
  fU2NHIT = 0;                            // Number of hits on plane U2
  memset( fU2WIRE, 0, lh );               //   Array of wires numbers
  memset( fU2TIME, 0, lf );               //   Array of corresponding TDCs
  fU2NCLUST = 0;                          //   Number of clusters on U2
  memset( fU2CLPOS, 0, ld );              //   Array of clusters positions
  memset( fU2CLSIZ, 0, lc );              //   Array of clusters sizes
  fV2NHIT = 0;                            // Number of hits on plane V2
  memset( fV2WIRE, 0, lh );               //   Array of wires numbers
  memset( fV2TIME, 0, lf );               //   Array of corresponding TDCs
  fV2NCLUST = 0;                          //   Number of clusters on V2
  memset( fV2CLPOS, 0, ld );              //   Array of clusters positions
  memset( fV2CLSIZ, 0, lc );              //   Array of clusters sizes
  fTRN = 0;                               // Number of tracks (0 or 1)
  fTRX = 0.0;                             // Track X coord (in cm) in spect cs
  fTRY = 0.0;                             // Track Y coord (in cm) in spect cs
  fTRZ = 0.0;                             // Track Z coord (in cm) in spect cs
  fTRTH = 0.0;                            // Track Theta angle in spect cs
  fTRPH = 0.0;                            // Track Phi angle in spect cs
}

//_____________________________________________________________________________
Int_t THaVDC::Decode( const THaEvData& evdata )
{
  // Decode data from VDC planes, and copy the data into the following local 
  // data structure:
  //
  // fU1NHIT             -  Number of hits on the plane U1
  // fU1WIRE[0]...[49]   -  Array of hit wires numbers on plane U1
  // fU1TIME[0]...[49]   -  Array of corresponding TDCs times
  // fV1NHIT             -  Number of hits on the plane V1
  // fV1WIRE[0]...[49]   -  Array of hit wires numbers on plane V1
  // fV1TIME[0]...[49]   -  Array of corresponding TDCs times
  // fU2NHIT             -  Number of hits on the plane U2
  // fU2WIRE[0]...[49]   -  Array of hit wires numbers on plane U2
  // fU2TIME[0]...[49]   -  Array of corresponding TDCs times
  // fV1NHIT             -  Number of hits on the plane V2
  // fV2WIRE[0]...[49]   -  Array of hit wires numbers on plane V2
  // fV2TIME[0]...[49]   -  Array of corresponding TDCs times
  //
  // In case of multihits, only the last hit of a wire is kept.
  // Maximum 50 hits on a plane are considered (MAXHIT=50).
  // This is reasonable since the algorithm only considers wire numbers, 
  // not TDC times.

  static const char* const detnam[] = { "U1", "V1", "U2", "V2" };

  // Zero event-by-event data
  ClearEvent();

  Int_t nhit = 0, prev_k = 0;
  bool problem = false;

  // Loop over all modules defined for VDC planes package
  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {

    // k is the index of the wire chamber plane. We assume that there
    // are exactly 4 modules per plane in the sequence U1,V1,U2,V2.
    Int_t k = i/4;

    // Do nothing if MAXHIT exceeded for the current plane.
    if( problem ) {
      if( k != prev_k ) {
	problem = false;
	prev_k = k;
      } else 
	continue;
    }
    THaDetMap::Module* d = fDetMap->GetModule(i);
    DataDest* dest = fDataDest + k;	
    Int_t& n = *dest->nhit;

    // Loop over all channels that have a hit.
    Int_t j = 0, nchan = evdata.GetNumChan( d->crate, d->slot );
    while( j<nchan && !problem ) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      j++;
      if( chan > d->hi || chan < d->lo ) continue;     // Not one of my channels

      // Although the VDC is capable of recording multiple hits,
      // there is no point here in using them since we only need
      // wire numbers. So we record only the last hit in each channel
      // (since it is the one with the most reasonable TDC time).

      Int_t nr_hits = evdata.GetNumHits( d->crate, d->slot, chan );
      Int_t data = evdata.GetData( d->crate, d->slot, chan, nr_hits-1 );

      // Copy the data to the local variables.

//        cout << dec << "VDC: k,n,chan,data = " <<k<<","<<n<<",";
//        cout <<","<<chan<<","<<data << endl;

      if( n<MAXHIT ){
	dest->wire[n] = fFirstWire[i] + chan - d->lo;  // Detector channel
	dest->time[n] = static_cast<Float_t>( data );  // Time (raw)
	n++;                                           // Hits on plane
	nhit++;                                        // Hits on chamber
      } else {
#ifdef WITH_DEBUG
	if( fDebug>0 ) {
	  Warning( Here("Decode()"), "Too many hits on %s plane. "
		   "Extra data ignored.", detnam[k] );
	}
#endif
	problem = true;
      }

    }//while(chan)
  }//for(module)

  return nhit;
}

//_____________________________________________________________________________
Int_t THaVDC::CoarseTrack( TClonesArray& tracks )
{
  // Reconstruct clusters in the VDC planes, and copy the data into the 
  // following local data structure:
  //
  // fU1NCLUST           -  Number of clusters reconstructed in the plane U1
  // fU1CLPOS[0]...[9]   -  Positions, i.e. centers (in wires) of each cluster
  // fU1CLSIZ[0]...[9]   -  Sizes, i.e number of wires in each cluster
  // fV1NCLUST           -  Number of clusters reconstructed in the plane V1
  // fV1CLPOS[0]...[9]   -  Positions, i.e. centers (in wires) of each cluster
  // fV1CLSIZ[0]...[9]   -  Sizes, i.e number of wires in each cluster
  // fU2NCLUST           -  Number of clusters reconstructed in the plane U2
  // fU2CLPOS[0]...[9]   -  Positions, i.e. centers (in wires) of each cluster
  // fU2CLSIZ[0]...[9]   -  Sizes, i.e number of wires in each cluster
  // fV2NCLUST           -  Number of clusters reconstructed in the plane V2
  // fV2CLPOS[0]...[9]   -  Positions, i.e. centers (in wires) of each cluster
  // fV2CLSIZ[0]...[9]   -  Sizes, i.e number of wires in each cluster
  // fTRN                -  Number of reconstructed VDC tracks (0 or 1)
  // fTRX                -  X coordinate (in cm) of track in spect cs
  // fTRY                -  Y coordinate (in cm) of track in spect cs
  // fTRZ                -  Z coordinate (in cm) of track in spect cs
  // fTRTH               -  Tangent of Theta angle of track in spect cs
  // fTRPH               -  Tangent of Phi angle of track in spect cs
  //
  // Maximum 10 clusters on each of planes plane are considered ( MAXCLU=10 ).
  // Only 1 ineffective wire in a cluster allowed (fMaxgap=1 in "db_VDC.dat").
  // Only hits with TDC times inside interval 800-2200 are included to cluster
  // (fMintime=800 and fMaxtime=2200 in "db_VDC.dat").
  // Track is reconstructed when only one cluster on each of VDC plane is found.

  if ( fU1NHIT > 0 ) Detcluster( fU1NHIT, fU1WIRE, fU1TIME,
				 fU1NCLUST, fU1CLPOS, fU1CLSIZ );
  if ( fV1NHIT > 0 ) Detcluster( fV1NHIT, fV1WIRE, fV1TIME,
				 fV1NCLUST, fV1CLPOS, fV1CLSIZ );
  if ( fU2NHIT > 0 ) Detcluster( fU2NHIT, fU2WIRE, fU2TIME,
				 fU2NCLUST, fU2CLPOS, fU2CLSIZ );
  if ( fV2NHIT > 0 ) Detcluster( fV2NHIT, fV2WIRE, fV2TIME,
				 fV2NCLUST, fV2CLPOS, fV2CLSIZ );

  // Vertex reconstruction in spect coordinate system by using unique clusters
  // in each of VDC planes:
  if ( fU1NCLUST==1 && fV1NCLUST==1 && fU2NCLUST==1 && fV2NCLUST==1 ){

    // Vertex reconstruction in U1 wireplane coordinate system:
    double pu1 = u1wbeg + (fU1CLPOS[0]-1)*u1wspac;   // Point posit in U1 cs
    double pv1 = v1wbeg + (fV1CLPOS[0]-1)*v1wspac;   // Point posit in V1 cs
    double pu2 = u2wbeg + (fU2CLPOS[0]-1)*u2wspac;   // Point posit in U2 cs
    double pv2 = v2wbeg + (fV2CLPOS[0]-1)*v2wspac;   // Point posit in U2 cs
    double tan_u, tan_v;
    if (pu2 != pu1)  tan_u = ( pu2 - pu1 ) / du2u1;  else  tan_u = 1.0;
    if (pv2 != pv1)  tan_v = ( pv2 - pv1 ) / dv2v1;  else  tan_v = 1.0;
    double pu = pu1;
    double pv = pv1 - tan_v * dv1u1;
    double fpd_x = ( pu * sin_vw - pv * sin_uw ) / sin_uvw;
    double fpd_y = ( pu * cos_vw - pv * cos_uw ) / sin_uvw;
    //double fpd_z = 0;
    double fpd_th =   ( tan_u * sin_vw - tan_v * sin_uw ) / sin_uvw;
    double fpd_ph = - ( tan_v * cos_uw - tan_u * cos_vw ) / sin_uvw;

    // Vertex reconstruction in spectrometer coordinate system:
    double tmp = 1 - fpd_th * tan_vdc;
    fTRTH = ( fpd_th + tan_vdc ) / tmp;                 // tan(theta)  
    fTRPH = fpd_ph / tmp / cos_vdc;                     // tan(phi)
    fTRX  = fpd_x * cos_vdc * ( 1 + fTRTH * tan_vdc );  // X (cm)
    fTRY  = fpd_y + sin_vdc * fTRPH * fpd_x;            // Y (cm)
    fTRZ  = 0.0;                                        // Z (cm)
    fTRN  = 1;                                          // 1 track

    // Add the track to the array. Momentum is zero for now. No clusters.
    AddTrack( tracks, 0.0, fTRTH, fTRPH, fTRX, fTRY );
  }

  return 0;
}
//.............................................................................
Int_t THaVDC::Detcluster( Int_t nhit, Int_t theWire[], Float_t theTime[],
			  Int_t& nclust, Float_t clpos[], Int_t clsiz[])
{
  // Find clusters in a VDC plane and calculate their center positions.

  Int_t nclu = -1, pwire = -10, wire, ndif;
  for ( Int_t i=0; i<nhit; i++ ) {
    if ( fMintime<theTime[i] && theTime[i]<fMaxtime ) {
      wire = theWire[i];
      if ( wire != pwire ) {
	ndif = wire - pwire;
	pwire = wire;
	if ( ndif > fMaxgap+1 ) {
	  nclu++;                          // Begin next cluster.
	  if ( nclu == MAXCLU ) goto maxreach;
	  clsiz[nclu] = 1;                 // Size of the cluster.
	  clpos[nclu] = wire;              // Begining of the cluster.
	} else
	  clsiz[nclu] += ndif;             // Add number of wires in cluster.
      }
    }
  }
 maxreach:
  nclust = nclu+1;                         // Number of clusters.
  for ( Int_t i=0; i<nclust; i++ ) 
    clpos[i]=clpos[i]+static_cast<Float_t>(clsiz[i]-1)/2;   // Centers

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDC::FineTrack( TClonesArray& tracks )
{
  // Calculate exact track position and angle using drift time information.
  // Not yet implemented. Leaves tracks untouched, does nothing, returns 0.

  return 0;
}

////////////////////////////////////////////////////////////////////////////////


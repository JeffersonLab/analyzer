//*-- Author :    Chris  Behre   June 2000

//////////////////////////////////////////////////////////////////////////
//
// THaBpm
// 
// Decodes the BPM Data for BPM 4a and 4b
//
//////////////////////////////////////////////////////////////////////////

#include "THaBpm.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include "TMath.h"

#include <cstring>

ClassImp(THaBpm)

//_____________________________________________________________________________
THaBpm::THaBpm( const char* name, const char* description, 
		const char* device, THaApparatus* apparatus )
  : THaDetector(name,description,apparatus), fDevice("struck")
{
  // Constructor

  if( device && strlen(device) > 0 ) {
    fDevice = device;
    fDevice.ToLower();
  }
}

//_____________________________________________________________________________
THaBpm::~THaBpm()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//_____________________________________________________________________________
THaDetectorBase::EStatus THaBpm::Init( const TDatime& run_time )
{
  // Initialize detector.  "run_time" currently ignored.
  // Lot's of hardcoded constants here that need to come from a database
  // in the future.

  static const char* const here = "Init()";

  if( IsInit() ) {
    Warning( Here(here), "Detector already initialized. Doing nothing." );
    return fStatus;
  }

  MakePrefix();

  // Which BPM are we?

  int det = 0, bpm = 0;

  if( fDevice == "lecroy" ) 
    det = 1;
  else if( fDevice == "vmic" ) 
    det = 2;
  else if( fDevice == "struck" )
    det = 3;

  if( fName.Index("4a") != kNPOS )
    bpm = 1;
  else if( fName.Index("4b") != kNPOS )
    bpm = 2;
    
  if( !det || !bpm ) {
    Warning( Here(here), "No such device: %s. BPM initialization failed.",
	     fDevice.Data() ); 
    return fStatus;
  }
  
  // Initialize detector map

  const THaDetMap::Module m[6] = {
    { 14, 2, 0, 3, 0 },  { 14, 2, 4, 7, 0 },     // LeCroy
    { 14, 3, 8, 11, 0 }, { 14, 3, 12, 15, 0 },   // VMIC
    { 14, 4, 0, 3, 0 },  { 14, 4, 4, 7, 0 }      // Struck
  };
  int i = 2*(det-1)+bpm-1;
  fDetMap->AddModule( m[i].crate, m[i].slot, m[i].lo, m[i].hi );

  // Initialize parameters dependent on device.

  switch( i ) {
  case 0:
  case 2:
    // LeCroy or VMIC/4a
    kxp_ped  = -675;   // Pedestal for X-axis Plus ant. on BPM 4a (Lecroy Detector)
    kxm_ped  = -656;   // Pedastal for X-axis Minus ant. on BPM 4a
    kyp_ped  = -206;   // Pedastal for Y-axis Plus ant. on BPM 4a
    kym_ped  = -190;   // Pedastal for Y-axis Minus ant. on BPM 4a 
    kalpha_x = .9752;  // Gain Factor for X-axis set by Espace to 1.0
    kalpha_y = 1.1088; // Gain Factor for y-axis set by Espase to 1.0
    kkappa   = 18.87;  // Conversion Factor in millimeters 18.87
    kx_att   = 1.21;   // Attenuation Factors X-axis, BPM 4a
    ky_att   = 1.13;   // Attenuation Factors Y-axis, BPM 4a
    kx_off   = -1;     // Offset to X-axis im millimeters, BPM 4a 
    ky_off   = 1;      // Offset to Y-axis im millimeters, BPM 4a
    break;
  case 1:
  case 3:
    // LeCroy or VMIC/4b
    kxp_ped  = -519;   // Pedestal for X-axis Plus ant. on BPM 4b 
    kxm_ped  = -464;   // Pedastal for X-axis Minus ant. on BPM 4b
    kyp_ped  = -440;   // Pedastal for Y-axis Plus ant. on BPM 4b
    kym_ped  = -415;   // Pedastal for Y-axis Minus ant. on BPM 4b
    kalpha_x = .9829;  // Gain Factor for X-axis set by Espace to 1.0
    kalpha_y = 1.1191; // Gain Factor for Y-axis set by Espace to 1.0
    kkappa   = 18.87;  // Conversion Factor in millimeters   
    kx_att   = 1.01;   // Attenuation Factors X-axis, BPM 4b
    ky_att   = 1.01;   // Attenuation Factors Y-axis, BPM 4b
    kx_off   = -1;     // Offset to X-axis in millimeters, BPM 4b
    ky_off   = 1;      // Offset to X-axis in millimeters, BPM 4b
    break;
  case 4:
    // Struck/4a
    kxp_ped  = 2105;    // Pedestal for X-axis Plus ant. on BPM 4a 
    kxm_ped  = 2104;    // Pedastal for X-axis Minus ant. on BPM 4a
    kyp_ped  = 2098;    // Pedastal for Y-axis Plus ant. on BPM 4a
    kym_ped  = 2098;    // Pedastal for Y-axis Minus ant. on BPM 4a
    kalpha_x = 1;       // Gain Factor for X-axis set by Espace to 1.0
    kalpha_y = 1;       // Gain Factor for y-axis set by Espase to 1.0
    kkappa   = 18.87;   // Conversion Factor in millimeters 
    kx_att   = 1.21;    // Attenuation Factors X-axis, BPM 4a
    ky_att   = 1.13;    // Attenuation Factors Y-axis, BPM 4a
    kx_off   = -0.27;   // Offset to X-axis im millimeters, BPM 4a 
    ky_off   = 0.31;    // Offset to Y-axis im millimeters, BPM 4a
    break;
  case 5:
    // Struck/4b
    kxp_ped  = 2090;    // Pedestal for X-axis Plus ant. on BPM 4b 
    kxm_ped  = 2085;    // Pedastal for X-axis Minus ant. on BPM 4b
    kyp_ped  = 2085;    // Pedastal for Y-axis Plus ant. on BPM 4b
    kym_ped  = 2086;    // Pedastal for Y-axis Minus ant. on BPM 4b
    kalpha_x = 1;       // Gain Factor for X-axis set by Espace to 1.0
    kalpha_y = 1;       // Gain Factor for Y-axis set by Espace to 1.0
    kkappa   = 18.87;   // Conversion Factor in millimeters   
    kx_att   = 1.01;    // Attenuation Factors X-axis, BPM 4b
    ky_att   = 1.01;    // Attenuation Factors Y-axis, BPM 4b
    kx_off   = -0.89;   // Offset to X-axis in millimeters, BPM 4b
    ky_off   = 0.17;    // Offset to X-axis in millimeters, BPM 4b
    break;
  default:
    Warning( Here(here), "Invalid index %d for parameter initialization. "
	     "Parameters not properly initialized.", i );
    break;
  }

  // Constants For BurstFit()
  kFxrast = 18300;       // Raster Frequency, X-axis
  kFyrast = 22600;       // Raster Frequency, Y-axis
  kDelT   = .000004;     // Time difference between hits, seconds 

  // Initialization & Calculations for BurstFit(), these calculations only need 
  // to be done once per run, not every event.
    

  fcwxt      = 0;        // Sum(cos(w*time*index))
  fswxt      = 0;        // Sum(sin(w*time*index))
  fccwxt     = 0;        // Sum(cos(cos(w*time*index)))
  fsswxt     = 0;        // Sum(sin(sin(w*time*inedx)))
  fs2wxt     = 0;        // Sum(sin(2*w*time*index))

  fcwyt      = 0;        // Sum(cos(w*time*index))
  fswyt      = 0;        // Sum(sin(w*time*index))
  fccwyt     = 0;        // Sum(cos(cos(w*time*index)))
  fsswyt     = 0;        // Sum(sin(sin(w*time*inedx)))
  fs2wyt     = 0;        // Sum(sin(2*w*time*index))

  fwx = kFxrast * 2 * TMath::Pi(); // Raster Frequency in Radiun, X-axis
  fwy = kFyrast * 2 * TMath::Pi(); // Raster Frequency in Radiun, Y-axis

  for( int i = 1; i<7; i++ ) {
    fcwxt  = cos( fwx*kDelT*i ) + fcwxt; 
    fswxt  = sin( fwx*kDelT*i ) + fswxt;
    fccwxt = cos( cos( fwx*kDelT*i ) ) + fccwxt;
    fsswxt = sin( sin( fwx*kDelT*i ) ) + fsswxt;
    fs2wxt = sin( 2*fwx*kDelT*i ) + fs2wxt;
    fcwyt  = cos( fwy*kDelT*i ) + fcwyt;
    fswyt  = sin( fwy*kDelT*i ) + fswyt;
    fccwyt = cos( cos( fwy*kDelT*i ) ) + fccwyt;
    fsswyt = sin( sin( fwy*kDelT*i ) ) + fsswyt;
    fs2wyt = sin( 2*fwy*kDelT*i ) + fs2wyt;
  }
  kDxsub1 = fccwxt*fsswxt - TMath::Power(fs2wxt/2,2);
  kDxsub2 = fcwxt*fsswxt - fs2wxt*fswxt/2;
  kDxsub3 = fcwxt*fs2wxt/2 - fccwxt*fswxt;
  kDx     = 6*kDxsub1 - fcwxt*kDxsub2 + fswxt*kDxsub3;
  kDysub1 = fccwyt*fsswyt - TMath::Power(fs2wyt/2,2);
  kDysub2 = fcwyt*fsswyt - fs2wyt*fswyt/2;
  kDysub3 = fcwyt*fs2wyt/2 - fccwyt*fswyt;
  kDy     = 6*kDysub1 - fcwyt*kDysub2 + fswyt*kDysub3;


  // Define global variables

  RVarDef vars[] = {
    { "x",  "Corrected x-axis data", "fX" },
    { "y",  "Corrected y-axis data", "fY" },
    { "Yx", "Curve parameters x",    "fYx" },
    { "Yy", "Curve parameters y",    "fYy" },
    { 0 }
  };
  DefineVariables( vars );

  return fStatus = kOK;
}

//_____________________________________________________________________________
Int_t THaBpm::Decode( const THaEvData& evdata )
{
  // Decode BPM  data.
  // 
  // Data is brought out of the data stream and converted to physical  
  // demensions, in Millimeters from the center of the Beam line.
 
  const int MAXH = 6;
  Int_t nhit = 0;

  // Local Variables used in Data Manipulation

  Int_t xp[MAXH] = {0,0,0,0,0,0};  //Local Variable for BPM X-axis plus
  Int_t xm[MAXH] = {0,0,0,0,0,0};  //Local Variable for BPM X-axis minus
  Int_t yp[MAXH] = {0,0,0,0,0,0};  //Local Variable for BPM Y-axis plus
  Int_t ym[MAXH] = {0,0,0,0,0,0};  //Local Variable for BPM Y-axis minus

  Int_t* chan_map[16] = { xp, xm, yp, ym, xp, xm, yp, ym,
			  xp, xm, yp, ym, xp, xm, yp, ym };

  // Clear event data.

  ClearEvent();

  // Loop over all modules defined for this detector
  
  for( UShort_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );   

    // Loop over all channels that have a hit.
    for( UShort_t j = 0; j < evdata.GetNumChan( d->crate, d->slot); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan > d->hi || chan < d->lo ) continue;   // Not one of my channels.

      //Loop through the hits in the channel and
      //copy the data to local variables.
      
      Int_t n_hits = evdata.GetNumHits(d->crate, d->slot, chan);
      if( n_hits > MAXH )
	Warning( "Decode()", "Too many BPM hits: %d for crate/slot/chan = "
		 "%hu/%hu/%d. Extra data ignored.", 
		 n_hits, d->crate, d->slot, chan );

      Int_t good_hits = TMath::Max( n_hits, MAXH );
      for (Int_t k=0; k<good_hits; k++) { 
	Int_t data = evdata.GetData( d->crate, d->slot, chan, k );
	Int_t* w = chan_map[chan];
	w[k] = data;
	nhit++;
      }

      // Apply corrections to raw data.

      for (Int_t k=0; k<evdata.GetNumHits(d->crate, d->slot, chan); k++) {
	Float_t xp_ped = static_cast<Float_t>( xp[k] - kxp_ped );
	Float_t xm_ped = static_cast<Float_t>( xm[k] - kxm_ped ); 
	Float_t yp_ped = static_cast<Float_t>( yp[k] - kyp_ped );
	Float_t ym_ped = static_cast<Float_t>( ym[k] - kym_ped );  
 
	// X-axis corrected for rotation WRT the ground
	Float_t x_rot = kkappa * ((xp_ped - (kalpha_x * xm_ped)) / 
				  (xp_ped + (kalpha_x * xm_ped))); 

	// Y-axis corrected for rotation WRT the ground
	Float_t y_rot = kkappa * ((yp_ped - (kalpha_y * ym_ped)) / 
				  (yp_ped + (kalpha_y * ym_ped))); 
	 
	// Correct for attenuation and offset, save results
	fX[k] = (-1.0 * kx_off) +((x_rot - y_rot) * (kx_att / TMath::Sqrt(2.0)));
	fY[k] = ky_off + ((x_rot + y_rot) * (ky_att/TMath::Sqrt(2.0)));  
      }

      //  BurstFit(evdata.GetNumHits(d->crate, d->slot, chan));
    }
    BurstFit(6);
  }
  return nhit ;
}
  

//_____________________________________________________________________________
 void THaBpm::BurstFit(Int_t numhits)
{
  // Fit the Burst Mode hits to a curve

  fXsum      = 0;   // Sum of X-axis positions 
  fXcwtsum   = 0;   // Sum of (X-axis position * cos (wx*t))
  fXswtsum   = 0;   // Sum of (X-axis position * sin (wx*t))   
 
  fYsum      = 0;   // Sum of Y-axis positions   
  fYcwtsum   = 0;   // Sum of (Y-axis position * cos (wy*t))
  fYswtsum   = 0;   // Sum of (Y-axis position * sin (wy*t))
  

  //  Float_t   fXtest[6]  ={.08007,.080074,.080078,.080082,.080086,.08009};
  //  Float_t   fYtest[6]  ={-1.2037,-2.8747,-4.3969,-4.9971,-4.5494,-3.1479};

    
  for( Int_t i = 0; i < numhits; i++ ) {
    fXsum = fX[i] + fXsum;
    fXcwtsum = (fX[i] * cos( fwx * kDelT * (i+1) )) + fXcwtsum;
    fXswtsum = (fX[i] * sin( fwx * kDelT * (i+1))) + fXswtsum;
    fYsum = fY[i] + fYsum;
    fYcwtsum = (fY[i] * cos( fwy * kDelT * (i+1))) + fYswtsum;
    fYswtsum = (fY[i] * sin( fwy * kDelT * (i+1))) + fYswtsum;
  }
  
  // Find Determinant Xo for X-axis

  fDxsub2Xo = fcwxt * ((fXcwtsum * fsswxt) - (.5 * fs2wxt * fXswtsum));
  fDxsub3Xo = fswxt * ((.5 * fs2wxt * fXcwtsum) - (fccwxt * fXswtsum));
  fDxXo     = (fXsum * kDxsub1) - (fDxsub2Xo) + (fDxsub3Xo);

  //Find Determinant Ao cos(phi) for X-axis 

  fDxsub1AC = 6 * ((fXcwtsum * fsswxt) - (.5 * fs2wxt * fXswtsum));
  fDxsub3AC = fswxt * ((fXswtsum * fcwxt) - (fswxt * fXcwtsum));
  fDxAC     = (fDxsub1AC) - (fXsum * kDxsub2) + (fDxsub3AC);

  // Find Determinant Ao sin(phi) for X-axis

  fDxsub1AS = 6 * ((fccwxt * fXswtsum) - (fYcwtsum * .5 * fs2wxt));
  fDxsub2AS = fcwxt * ((fXswtsum * fcwxt) - (fswxt * fXcwtsum));
  fDxAS     = (fDxsub1AS) - (fDxsub2AS) + (fXsum * kDxsub3);

  // Find the Parameters of the Curve for X-axis Raster

  fYx[0]    = fDxXo/kDx;
  fYx[1]    = sqrt(TMath::Power( fDxAS/kDx, 2) +
		   TMath::Power( fDxAC/kDx, 2));
  fYx[2]    = atan((fDxAS / kDx) / (fDxAC / kDx));

  // Find Determinant Xo for Y-axis

  fDysub2Xo = fcwyt * ((fYcwtsum * fsswyt) - (.5 * fs2wyt * fYswtsum));
  fDysub3Xo = fswyt * ((.5 * fs2wyt * fYcwtsum) - (fccwyt * fYswtsum));
  fDyXo     = (fYsum * kDysub1) - (fDysub2Xo) + (fDysub3Xo);

  //Find Determinant Ao cos(phi) for Y-axis 

  fDysub1AC = 6 * ((fYcwtsum * fsswyt) - (.5 * fs2wyt * fYswtsum));
  fDysub3AC = fswyt * ((fYswtsum * fcwyt) - (fswyt * fYcwtsum));
  fDyAC     = (fDysub1AC) - (fYsum * kDysub2) + (fDysub3AC);

  // Find Determinant Ao sin(phi) for Y-axis

  fDysub1AS = 6 * ((fccwyt * fYswtsum) - (fYcwtsum * .5 * fs2wyt));
  fDysub2AS = fcwyt * ((fYswtsum * fcwyt) - (fswyt * fYcwtsum));
  fDyAS     = (fDysub1AS) - (fDysub2AS) + (fYsum * kDysub3);

  // Find the Parameters of the Curve for Y-axis Raster
  
  fYy[0]    = fDyXo/kDy;
  fYy[1]    = sqrt(TMath::Power( fDyAS/kDy, 2) + 
		   TMath::Power( fDyAC/kDy, 2));
  fYy[2]    = atan((fDyAS / kDy) / (fDyAC / kDy));

}
 

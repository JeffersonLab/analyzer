///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaCherenkov                                                              //
//                                                                           //
// Class for a generic Cherenkov consisting of one or more phototubes.       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaCherenkov.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include "THaTrack.h"
#include "THaTrackProj.h"
#include "TClonesArray.h"
#include "TDatime.h"
#include "TMath.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>

using namespace std;

//_____________________________________________________________________________
THaCherenkov::THaCherenkov( const char* name, const char* description,
			    THaApparatus* apparatus )
  : THaPidDetector(name,description,apparatus),
    fOff(0), fPed(0), fGain(0),
    fNThit(0), fT(0), fT_c(0), fNAhit(0), fA(0), fA_p(0), fA_c(0),
    tan_angle(0), sin_angle(0), cos_angle(1.)
{
  // Constructor
  fTrackProj = new TClonesArray( "THaTrackProj", 5 );
}

//_____________________________________________________________________________
Int_t THaCherenkov::DoReadDatabase( FILE* fi, const TDatime& /* date */ )
{
  // Read this detector's parameters from the database file 'fi'.
  // This function is called by THaDetectorBase::Init() once at the
  // beginning of the analysis.
  // 'date' contains the date/time of the run being analyzed.

  const char* const here = "ReadDatabase";
  const int LEN = 100;
  char buf[LEN];
  Int_t nelem;
  Int_t flags = kErrOnTooManyValues|kWarnOnDataGuess;

  if( ReadBlock(fi,&nelem,1,here,flags|kNoNegativeValues) ) // Number of mirrors
    return kInitError;

  // Reinitialization only possible for same basic configuration
  if( fIsInit && nelem != fNelem ) {
    Error( Here(here), "Cannot re-initalize with different number of mirrors. "
	   "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
    return kInitError;
  }
  fNelem = nelem;

  if( fIsInit )
    ClearEvent();

  // Read detector map.  Assumes that the first half of the entries
  // is for ADCs, and the second half, for TDCs
  while( ReadComment(fi,buf,LEN) ) {}
  fDetMap->Clear();
  while (1) {
    Int_t crate, slot, first, last, first_chan,model;
    int pos;
    fgets ( buf, LEN, fi );
    int n = sscanf( buf, "%6d %6d %6d %6d %6d %n",
		    &crate, &slot, &first, &last, &first_chan, &pos );
    if( crate < 0 ) break;
    if( n < 5 )
      return ErrPrint(fi,here);
    model=atoi(buf+pos); // if there is no model number given, set to zero
    if( fDetMap->AddModule( crate, slot, first, last, first_chan, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      return kInitError;
    }
  }
  if( fDetMap->GetTotNumChan() != 2*fNelem ) {
    Error( Here(here), "Database inconsistency.\n Defined %d channels in detector map, "
	   "but have %d total channels (%d mirrors with 1 ADC and 1 TDC each)",
	   fDetMap->GetTotNumChan(), 2*fNelem, fNelem );
    return kInitError;
  }

  // Read geometry
  Double_t dvals[3], angle;
  if( ReadBlock(fi,dvals,3,here,flags) )                   // Detector's X,Y,Z coord
    return kInitError;
  fOrigin.SetXYZ( dvals[0], dvals[1], dvals[2] );

  if( ReadBlock(fi,fSize,3,here,flags|kNoNegativeValues) ) // Size of det in X,Y,Z
    return kInitError;

  if( ReadBlock(fi,&angle,1,here,flags) )                  // Rotation angle of det
    return kInitError;

  const Double_t degrad = TMath::Pi()/180.0;
  tan_angle = TMath::Tan(angle*degrad);
  sin_angle = TMath::Sin(angle*degrad);
  cos_angle = TMath::Cos(angle*degrad);
  DefineAxes(angle*degrad);

  // Dimension arrays when initizalizing for the first time
  if( !fIsInit ) {
    // Calibration data
    fOff = new Float_t[ fNelem ];
    fPed = new Float_t[ fNelem ];
    fGain = new Float_t[ fNelem ];

    // Per-event data
    fT   = new Float_t[ fNelem ];
    fT_c = new Float_t[ fNelem ];
    fA   = new Float_t[ fNelem ];
    fA_p = new Float_t[ fNelem ];
    fA_c = new Float_t[ fNelem ];

    fIsInit = true;
    ClearEvent();
  }

  // Read calibrations
  if( ReadBlock(fi,fOff,fNelem,here,flags) )                    // TDC offsets
    return kInitError;

  if( ReadBlock(fi,fPed,fNelem,here,flags|kNoNegativeValues) )  // ADC pedestals
    return kInitError;

  if( ReadBlock(fi,fGain,fNelem,here,flags|kNoNegativeValues) ) // ADC gains
    return kInitError;

  // Debug printout
  if ( fDebug > 1 ) {
    const UInt_t N = static_cast<UInt_t>(fNelem);
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    DBRequest list[] = {
      { "Number of mirrors", &fNelem,     kInt       },
      { "Detector position", pos,         kDouble, 3 },
      { "Detector size",     fSize,       kFloat,  3 },
      { "Detector angle",    &angle,                 },
      { "TDC offsets",       fOff,        kFloat,  N },
      { "ADC pedestals",     fPed,        kFloat,  N },
      { "ADC gains",         fGain,       kFloat,  N },
      { 0 }
    };
    DebugPrint( list );
  }

  return kOK;
}

//_____________________________________________________________________________
Int_t THaCherenkov::ReadDatabase( const TDatime& date )
{
  // Wrapper around actual database reader. Using a wrapper makes it much
  // easier to close the input file in case of an error.

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  Int_t ret = DoReadDatabase( fi, date );

  fclose(fi);
  return ret;
}

//_____________________________________________________________________________
Int_t THaCherenkov::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "nthit",  "Number of Left paddles TDC times",  "fNThit" },
    { "nahit",  "Number of Right paddles TDC times", "fNAhit" },
    { "t",      "TDC values",                        "fT" },
    { "t_c",    "Corrected TDC values",              "fT_c" },
    { "a",      "ADC values",                        "fA" },
    { "a_p",    "Ped-subtracted ADC values ",        "fA_p" },
    { "a_c",    "Corrected ADC values",              "fA_c" },
    { "asum_p", "Sum of ADC minus pedestal values",  "fASUM_p" },
    { "asum_c", "Sum of corrected ADC amplitudes",   "fASUM_c" },
    { "trx",    "x-position of track in det plane",  "fTrackProj.THaTrackProj.fX" },
    { "try",    "y-position of track in det plane",  "fTrackProj.THaTrackProj.fY" },
    { "trpath", "TRCS pathlen of track to det plane","fTrackProj.THaTrackProj.fPathl" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaCherenkov::~THaCherenkov()
{
  // Destructor. Remove variables from global list.

  if( fIsSetup )
    RemoveVariables();
  if( fIsInit )
    DeleteArrays();
}

//_____________________________________________________________________________
void THaCherenkov::DeleteArrays()
{
  // Delete member arrays. Internal function used by destructor.

  delete [] fA_c;    fA_c    = NULL;
  delete [] fA_p;    fA_p    = NULL;
  delete [] fA;      fA      = NULL;
  delete [] fT_c;    fT_c    = NULL;
  delete [] fT;      fT      = NULL;

  delete [] fGain;   fGain   = NULL;
  delete [] fPed;    fPed    = NULL;
  delete [] fOff;    fOff    = NULL;
}

//_____________________________________________________________________________
inline 
void THaCherenkov::ClearEvent()
{
  // Reset all local data to prepare for next event.

  fNThit = fNAhit = 0;
  assert(fIsInit);
  for( Int_t i=0; i<fNelem; ++i ) {
    fT[i] = fT_c[i] = fA[i] = fA_p[i] = fA_c[i] = kBig;
  }
  fASUM_p = fASUM_c = 0.0;
  fTrackProj->Clear();
}

//_____________________________________________________________________________
Int_t THaCherenkov::Decode( const THaEvData& evdata )
{
  // Decode Cherenkov data, correct TDC times and ADC amplitudes, and copy
  // the data into the local data members.
  // This implementation assumes that the first half of the detector map 
  // entries corresponds to ADCs, and the second half, to TDCs.

  ClearEvent();

  // Loop over all modules defined for Cherenkov detector
  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );
    bool adc = (d->model ? fDetMap->IsADC(d) : i < fDetMap->GetSize()/2 );
    
    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan < d->lo || chan > d->hi ) continue;     // Not one of my channels

      // Get the data. Aero mirrors are assumed to have only single hit (hit=0)
      Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );

      // Get the detector channel number, starting at 0
      Int_t k = d->first + ((d->reverse) ? d->hi - chan : chan - d->lo) - 1;

#ifdef WITH_DEBUG
      if( k<0 || k>= fNelem ) {
	Warning( Here("Decode()"), "Illegal detector channel: %d", k );
        continue;
      }
#endif

      // Copy the data to the local variables.
      if ( adc ) {
	fA[k]   = data;
	fA_p[k] = data - fPed[k];
	fA_c[k] = fA_p[k] * fGain[k];
	// only add channels with signals to the sums
	if( fA_p[k] > 0.0 )
	  fASUM_p += fA_p[k];
	if( fA_c[k] > 0.0 )
	  fASUM_c += fA_c[k];
	fNAhit++;
      } else {
	fT[k]   = data;
	fT_c[k] = data - fOff[k];
	fNThit++;
      }
    }
  }

  if ( fDebug > 3 ) {
    printf("\nCherenkov %s:\n",GetPrefix());
    int ncol=3;
    for (int i=0; i<ncol; i++) {
      printf("  Mirror TDC   ADC  ADC_p  ");
    }
    printf("\n");
    
    for (int i=0; i<(fNelem+ncol-1)/ncol; i++ ) {
      for (int c=0; c<ncol; c++) {
	int ind = c*fNelem/ncol+i;
	if (ind < fNelem) {
	  printf("  %3d  %5.0f  %5.0f  %5.0f  ",ind+1,fT[ind],fA[ind],fA_p[ind]);
	} else {
	  //	  printf("\n");
	  break;
	}
      }
      printf("\n");
    }
  }
  
  return fNThit;
}

//_____________________________________________________________________________
Int_t THaCherenkov::CoarseProcess( TClonesArray& tracks )
{
  // Reconstruct coordinates of where a particle track crosses
  // the Cherenkov plane, and copy the point into the fTrackProj array.
  //
  // Calculation of coordinates of particle track cross point with Cherenkov
  // plane in the detector coordinate system. For this, parameters of track 
  // reconstructed in THaVDC::CrudeTrack() are used.

  int n_track = tracks.GetLast()+1;   // Number of reconstructed tracks

  for ( int i=0; i<n_track; i++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks[i] );
    Double_t pathl=kBig, xc=kBig, yc=kBig;
    Double_t dx=0.; // unused
    Int_t pad=-1;   // unused
    
    CalcTrackIntercept(theTrack, pathl, xc, yc);
    // if it hit or not, store the information (defaults if no hit)
    new ( (*fTrackProj)[i] ) THaTrackProj(xc,yc,pathl,dx,pad,this);
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaCherenkov::FineProcess( TClonesArray& tracks )
{
  // Fine Cherenkov processing.
  // Redo the track-matching, since tracks might have been thrown out
  // during the FineTracking stage.
  fTrackProj->Clear();
  int n_track = tracks.GetLast()+1;   // Number of reconstructed tracks

  for ( int i=0; i<n_track; i++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks[i] );
    Double_t pathl=kBig, xc=kBig, yc=kBig;
    Double_t dx=0.; // unused
    Int_t pad=-1;   // unused
    
    CalcTrackIntercept(theTrack, pathl, xc, yc);
    // if it hit or not, store the information (defaults if no hit)
    new ( (*fTrackProj)[i] ) THaTrackProj(xc,yc,pathl,dx,pad,this);
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaCherenkov::GetNTracks() const
{
  return fTrackProj->GetLast()+1;
}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaCherenkov)

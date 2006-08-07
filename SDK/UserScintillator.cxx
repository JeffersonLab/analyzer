//*-- Author :    Ole Hansen   04-Dec-03

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// UserScintillator                                                          //
//                                                                           //
// Example of how to extend an existing detector class with additional       //
// and/or different functionality. The UserScintillator can be replace any   //
// THaScintillator. In addition to all the work that THaScintillator does,   //
// a UserScintillator computes three additional quantities and exports       //
// three corresponding global variables:                                     //
//                                                                           //
//  padnum:  the number of the paddle ht by the Golden Track                 //
//  ytrk:    the transverse (y) position at the scintillator plane           //
//           calculated using the track coordinates.                         //
//           ("This is where the track went according to the VDCs")          //
//  ytdc:    dto. calculated using the scintillator TDC data.                //
//           ("This is where the track went according to the scintillator")  //
//  With standard thin scintillators and regular-precision TDCs, the ytdc    //
//  resolution is expected to be rather poor; still, it provides position    //
//  data independent of tracking.                                            //
//                                                                           //
//  To support these extra computations, only code directly related to them  //
//  has to be written: defining extra global variables, reading extra        //
//  database values, clearing new event-by-event variables, and computing    //
//  the new values.                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "UserScintillator.h"
#include "THaSpectrometer.h"
#include "VarDef.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "TClass.h"

using namespace std;

//_____________________________________________________________________________
UserScintillator::UserScintillator( const char* name, const char* description,
				  THaApparatus* apparatus ) :
  // Make sure to initialize member variables to sane values/defaults here!
  THaScintillator(name,description,apparatus), fGoodToGo(kFALSE)
{
  // Constructor
}

//_____________________________________________________________________________
UserScintillator::~UserScintillator()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//_____________________________________________________________________________
Int_t UserScintillator::DefineVariables( EMode mode )
{
  // Define/delete global variables

  // Don't redefine variables
  if( mode == kDefine && fIsSetup ) return kOK;
  // NB: This is a virtual function, and the base class does define its
  // own variables, so we MUST call the base class instance here.
  // Remember that the base class instance normally tests and sets fIsSetup,
  // so we must not do it before this call
  THaScintillator::DefineVariables( mode );
  //and then, we don't need to do it at all
  //  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "padnum", "Paddle number",                                 "fPaddle" },
    { "ytrk",   "Transverse pos'n at scint plane from track (m)","fYtrk" },
    { "ytdc",   "Transverse pos'n at scint plane from TDCs (m)", "fYtdc" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
Int_t UserScintillator::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database.
  // We use the same database file as the base class and
  // search for our own tagged entries, presumably appended at the end.

  // Now read the database file
  // First read base class parameters
  Int_t ret = THaScintillator::ReadDatabase(date);
  if( ret != kOK )
    return ret;

  // Read our parameters. For detailed comments, see UserDetector.cxx.
  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;
  
  // Storage and default values for data from database
  // Note: At present, these must be Double_t
  fC = fTDCscale = 0.0;
  Double_t stop = 0.0;

  // Set up table of tags to read and locations to store values.
  const TagDef tags[] = { 
    { "c",        &fC, 1 },        // Speed of light in medium
    { "tdcscale", &fTDCscale, 2 }, // TDC scale (ns/chan)
    { "stop",     &stop, 3 },      // Common stop mode (1=yes)
    { 0 }                          // Last element must be NULL
  };
  // Read the tags.
  Int_t err = LoadDB( fi, date, tags );

  // Complain and quit if any required values missing.
  if( err ) {    
    if( err>0 )
      Error( Here("ReadDatabase"), "Required tag %s missing in the "
	     "database.\nDetector initialization failed.",
	     tags[err-1].name );
    fclose(fi);
    return kInitError;
  }
  // Convert non-Double_t data types
  fCommonStop = (stop != 0.0);

  // Check if all paramaters ok. This is just an example that allows this
  // class to work even with non-spectrometer apparatuses and incomplete
  // databases. Alternatively, one could return an init error to force the
  // user to do more sane stuff and/or fix the database.
  bool have_spectro = GetApparatus() != NULL 
    && GetApparatus()->IsA()->InheritsFrom("THaSpectrometer");
  fGoodToGo = have_spectro && fSize[0]!=0.0 && fNelem>0;

  return kOK;
}

//_____________________________________________________________________________
Int_t UserScintillator::Decode( const THaEvData& evdata )
{
  // Decode the UserScintillator. The decoding is actually done in
  // the base class THaScintillator. Here we implement just one extension,
  // clearing of our own member variables (since ClearEvent() is not virtual).
  // Any additional processing needed at the Decode() stage should be done
  // here as well.

  fPaddle = -255;
  fYtdc = fYtrk = kBig;

  return THaScintillator::Decode(evdata);
}

//_____________________________________________________________________________
Int_t UserScintillator::FineProcess( TClonesArray& tracks )
{
  // Do additional calculations using the fine tracks.
  // Here we calculate the paddle number hit by the GoldenTrack in
  // the track array, and compute the transverse (y) position
  // within that paddle using the paddle's TDC data.

  // Call the base class method to do the standard work first:
  Int_t ret = THaScintillator::FineProcess(tracks);

  // Now here go the extensions ...

  // Number of reconstructed tracks
  Int_t n_track = tracks.GetLast()+1;
  // No tracks, bad init or bad parameters? If so, can't do anything here
  if( !(n_track > 0 && IsOK() && fGoodToGo))
    return ret;

  // Get the Golden Track 
  THaTrack* theTrack = 
    static_cast<THaSpectrometer*>(GetApparatus())->GetGoldenTrack();

  // Determine the paddle number using the focal plane coordinates
  // x and y are the coordinates of the track in the scintillator plane
  // (scintillator plane coordinate system)
  double fpe_x  = theTrack->GetX();
  double fpe_th = theTrack->GetTheta();
  double fpe_y  = theTrack->GetY();
  double fpe_ph = theTrack->GetPhi();
  double x = ( fpe_x + fpe_th * fOrigin.Z() ) / 
    ( cos_angle * (1.0 + fpe_th * tan_angle ) );
  // transverse position from track data
  fYtrk = fpe_y + fpe_ph * fOrigin.Z() - fpe_ph * sin_angle * x;

  fPaddle = (Int_t) TMath::Floor((x+fSize[0])/fSize[0]/2*fNelem);
  // Mark garbage as invalid
  if(fPaddle <0 || fPaddle >= fNelem) 
    fPaddle = -255;

  // Compute y-position from TDC data if possible
  else {
    if( fCommonStop )
      fYtdc = (fC*.30*fTDCscale*(fRT_c[fPaddle]-fLT_c[fPaddle])+2*fSize[1])/2; 
    else
      fYtdc = (fC*.30*fTDCscale*(fLT_c[fPaddle]-fRT_c[fPaddle])+2*fSize[1])/2;
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////////
ClassImp(UserScintillator)

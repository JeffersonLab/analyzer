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
#include "TMath.h"

using namespace std;

//_____________________________________________________________________________
UserScintillator::UserScintillator( const char* name, const char* description,
				    THaApparatus* apparatus ) :
  // Make sure to initialize member variables to sane values/defaults here!
  THaScintillator(name,description,apparatus),
  fPaddle(0), fYtdc(kBig), fYtrk(kBig), fGoodToGo(false), fCommonStop(true)
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
  // search for our own keys

  // const char* const here = "ReadDatabase";

  // Now read the database file
  // First read base class parameters
  Int_t ret = THaScintillator::ReadDatabase(date);
  if( ret != kOK )
    return ret;

  // Read our parameters. For detailed comments, see UserDetector.cxx.
  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // Storage and default values for data from database.
  // We might want to read a lot more here, for example calibration data.
  // See THaScintillator::ReadDatabase and other standard detectors for
  // extensive examples.
  Int_t stop = 1;

  // Read the database. This may throw exceptions, so we put it in a try block.
  Int_t err = 0;
  try {
    // Set up an array of database requests. See VarDef.h for details.
    const DBRequest request[] = {
      { "stop", &stop, kInt, 1 },    // Common stop mode (1=yes)
      { 0 }                          // Last element must be NULL
    };

    // Read the requested values
    err = LoadDB( file, date, request );
  }
  // Catch exceptions here so that we can close the file and clean up
  catch(...) {
    fclose(file);
    throw;
  }

  // Normal end of reading the database
  fclose(file);
  if( err != kOK )
    return err;

  // Convert data types
  fCommonStop = (stop != 0);

  // Check if all paramaters ok. This is just an example that allows this
  // class to work even with non-spectrometer apparatuses and incomplete
  // databases. Alternatively, one could return an init error to force the
  // user to do more sane stuff and/or fix the database.
  bool have_spectro = GetApparatus() != 0
    && GetApparatus()->IsA()->InheritsFrom("THaSpectrometer");
  fGoodToGo = have_spectro && fSize[0]!=0.0 && fNelem>0;

  return kOK;
}

//_____________________________________________________________________________
void UserScintillator::Clear( Option_t* opt )
{
  // Reset per-event data

  THaScintillator::Clear(opt);

  fPaddle = -255;
  fYtdc = fYtrk = kBig;
}

//_____________________________________________________________________________
// Int_t UserScintillator::Decode( const THaEvData& evdata )
// {
  // Decode the UserScintillator. For our purposes, the basic decoding done
  // in the base class THaScintillator is sufficient.
  //
  // Add any additional processing needed at the Decode() stage here.

//   return THaScintillator::Decode(evdata);
// }

//_____________________________________________________________________________
Int_t UserScintillator::FineProcess( TClonesArray& tracks )
{
  // Do additional calculations using the fine tracks.
  // Here we calculate the paddle number hit by the GoldenTrack in
  // the track array, and compute the transverse (y) position
  // within that paddle using the paddle's TDC data.

  // Call the base class method to do the standard work first:
  Int_t ret = THaScintillator::FineProcess(tracks);
  if( ret )
    return ret;

  // Now here go the extensions ...

  // Number of reconstructed tracks
  Int_t n_track = tracks.GetLast()+1;
  // No tracks, bad init or bad parameters? If so, can't do anything here
  if( !(n_track > 0 && IsOK() && fGoodToGo))
    // Return codes are currently ignored, but for consistency use 0 if OK
    // and non-zero for errors
    return 1;

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
  double cos_angle = fXax.X(), sin_angle = fXax.Z();
  double x = ( fpe_x + fpe_th * fOrigin.Z() ) /
    ( cos_angle + fpe_th * sin_angle );
  // transverse position from track data
  fYtrk = fpe_y + fpe_ph * fOrigin.Z() - fpe_ph * sin_angle * x;

  fPaddle = (Int_t) TMath::Floor(0.5*(x+fSize[0])/fSize[0]*fNelem);
  // Mark garbage as invalid
  if(fPaddle <0 || fPaddle >= fNelem)
    fPaddle = -255;

  // Compute y-position from TDC data if possible
  else {
    if( fCommonStop )
      fYtdc = 0.5*fCn*(fRT_c[fPaddle]-fLT_c[fPaddle]);
    else
      fYtdc = 0.5*fCn*(fLT_c[fPaddle]-fRT_c[fPaddle]);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
ClassImp(UserScintillator)

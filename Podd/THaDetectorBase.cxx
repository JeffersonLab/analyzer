//*-- Author :    Ole Hansen   15-Jun-01

//////////////////////////////////////////////////////////////////////////
//
// THaDetectorBase
//
// Abstract base class for a detector or subdetector.
//
// Derived classes must define all internal variables, a constructor
// that registers any internal variables of interest in the global
// physics variable list, and a Decode() method that fills the variables
// based on the information in the THaEvData structure.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetectorBase.h"
#include "THaDetMap.h"
#include "THaEvData.h"
#include "TMath.h"
#include "VarType.h"
#include "TRotation.h"

#include <sstream>
#include <stdexcept>

using namespace std;

//_____________________________________________________________________________
THaDetectorBase::THaDetectorBase( const char* name,
				  const char* description ) :
  THaAnalysisObject(name,description),
  fDetMap(new THaDetMap),
  fNelem(0),
  fNviews(1),
  fSize{kBig,kBig,kBig},
  fXax(1.0,0.0,0.0),
  fYax(0.0,1.0,0.0),
  fZax(0.0,0.0,1.0)
{
  // Normal constructor. Creates a detector with an empty detector map.
}

//_____________________________________________________________________________
THaDetectorBase::THaDetectorBase() :
  fDetMap(nullptr), fNelem(0), fNviews(1), fSize{kBig,kBig,kBig}
{
  // for ROOT I/O only
}

//_____________________________________________________________________________
THaDetectorBase::~THaDetectorBase()
{
  // Destructor
  RemoveVariables();
  delete fDetMap;
}

//_____________________________________________________________________________
void THaDetectorBase::Clear( Option_t* opt )
{
  // Clear event-by-event data in fDetectorData objects
  THaAnalysisObject::Clear(opt);

  for( auto& detData : fDetectorData ) {
    detData->Clear(opt);
  }
}

//_____________________________________________________________________________
void THaDetectorBase::Reset( Option_t* opt )
{
  // Clear event-by-event data and calibration data in fDetectorData objects
  Clear(opt);

  for( auto& detData : fDetectorData ) {
    detData->Reset(opt);
  }
}

//_____________________________________________________________________________
void THaDetectorBase::DefineAxes( Double_t rotation_angle )
{
  // Define detector orientation, assuming a tilt by rotation_angle (in rad)
  // around the y-axis

  fXax.SetXYZ( 1.0, 0.0, 0.0 );
  fXax.RotateY( rotation_angle );
  fYax.SetXYZ( 0.0, 1.0, 0.0 );
  fZax = fXax.Cross(fYax);

}

//_____________________________________________________________________________
Int_t THaDetectorBase::GetView( const DigitizerHitInfo_t& hitinfo ) const
{
  // Default method for getting the readout number of a detector element
  // from the hardware channel given in 'hitinfo'. The the detector map is
  // assumed to be organized in groups of fNelem consecutive channels
  // corresponding to each view.
  //
  // For example, a detector may have 6 elements (e.g. paddles), each with two
  // readouts, one on the right side, the other on the left, for a total of
  // 12 channels. The detector map should then specify the 6 right-side
  // channels first, followed by the 6 left-side channels (or vice versa).
  //
  // Derived classes may implement other schemes, for example alternating
  // left/right channels.
  //
  // Throws an exception if the view number exceeds the number of defined
  // views (fNviews).

  if( fNelem == 0 ) return 0; // don't crash if unset
  Int_t view = hitinfo.lchan / fNelem;
  if( view < 0 || view >= fNviews ) {
    ostringstream msg;
    msg << "View out of range (= " << view << ", max " << fNviews << "). "
        << "Should never happen. Call expert.";
    throw logic_error(msg.str());
  }
  return view;
}

//_____________________________________________________________________________
Int_t THaDetectorBase::FillDetMap( const vector<Int_t>& values, UInt_t flags,
				   const char* here )
{
  // Utility function to fill this detector's detector map.
  // See THaDetMap::Fill for documentation.

  Int_t ret = fDetMap->Fill( values, flags );
  if( ret > 0 )
    return ret;
  if( ret == 0 ) {
    Error( Here(here), "No detector map entries found. Check database." );
  } else if( ret == -1 ) {
    Error( Here(here), "Unknown hardware module number. "
                       "Check database or add module in THaDetMap.cxx." );
  } else if( ret == -128 ) {
    Error( Here(here), "Bad flag combination for filling "
                       "detector map. Call expert." );
  } else if( ret < 0 ) {
    Error( Here(here), "Invalid detector map data format "
                       "(wrong number of values). Check database." );
  }
  return ret;
}

//_____________________________________________________________________________
TVector3 THaDetectorBase::DetToTrackCoord( const TVector3& p ) const
{
  // Convert coordinates of point 'p' from the detector to the track
  // reference frame.

  return TVector3( p.X()*fXax + p.Y()*fYax + p.Z()*fZax + fOrigin );
}

//_____________________________________________________________________________
TVector3 THaDetectorBase::DetToTrackCoord( Double_t x, Double_t y ) const
{
  // Convert x/y coordinates in detector plane to the track reference frame.

  return TVector3( x*fXax + y*fYax + fOrigin );
}

//_____________________________________________________________________________
TVector3 THaDetectorBase::TrackToDetCoord( const TVector3& point ) const
{
  // Convert/project coordinates of the given 'point' to coordinates in this
  // detector's reference frame (defined by fOrigin, fXax, and fYax).

  TVector3 v = point - fOrigin;
  // This works out to exactly the same as a multiplication with TRotation
  return TVector3( v.Dot(fXax), v.Dot(fYax), v.Dot(fZax) );
}

//_____________________________________________________________________________
Bool_t THaDetectorBase::IsInActiveArea( Double_t x, Double_t y ) const
{
  // Check if given (x,y) coordinates are inside this detector's active area
  // (defined by fSize). x and y MUST be given in coordinates of this detector
  // plane, i.e. relative to fOrigin and measured along fXax and fYax.

  return ( TMath::Abs(x) <= fSize[0] &&
	   TMath::Abs(y) <= fSize[1] );
}

//_____________________________________________________________________________
Bool_t THaDetectorBase::IsInActiveArea( const TVector3& point ) const
{
  // Check if given point 'point' is inside this detector's active area
  // (defined by fSize). 'point' is first projected onto the detector
  // plane (defined by fOrigin, fXax and fYax), and then the projected
  // (x,y) coordinates are checked against fSize.
  // Usually 'point' lies within the detector plane. If it does not,
  // be sure you know what you are doing.

  TVector3 v = TrackToDetCoord( point );
  return IsInActiveArea( v.X(), v.Y() );
}

//_____________________________________________________________________________
void THaDetectorBase::PrintDetMap( Option_t* opt ) const
{
  // Print out detector map.

  fDetMap->Print( opt );
}

//_____________________________________________________________________________
Int_t THaDetectorBase::ReadGeometry( FILE* file, const TDatime& date,
				     Bool_t required )
{
  // Read this detector's basic geometry information from the database.
  // Derived classes may override to read more advanced data.

  const char* const here = "ReadGeometry";

  vector<double> position, size, angles;
  Bool_t optional = !required;
  DBRequest request[] = {
    { "position", &position, kDoubleV, 0, optional, 0,
      "\"position\" (detector position [m])" },
    { "size",     &size,     kDoubleV, 0, optional, 0,
      "\"size\" (detector size [m])" },
    { "angle",    &angles,   kDoubleV, 0, true, 0,
      "\"angle\" (detector angles(s) [deg]" },
    { nullptr }
  };
  Int_t err = LoadDB( file, date, request );
  if( err )
    return kInitError;

  if( !position.empty() ) {
    if( position.size() != 3 ) {
      Error( Here(here), "Incorrect number of values = %lu for "
	     "detector position. Must be exactly 3. Fix database.",
             position.size() );
      return 1;
    }
    fOrigin.SetXYZ( position[0], position[1], position[2] );
  }
  else
    fOrigin.SetXYZ(0,0,0);

  if( !size.empty() ) {
    if( size.size() != 3 ) {
      Error( Here(here), "Incorrect number of values = %lu for "
	     "detector size. Must be exactly 3. Fix database.", size.size() );
      return 2;
    }
    if( size[0] == 0 || size[1] == 0 || size[2] == 0 ) {
      Error( Here(here), "Illegal zero detector dimension. Fix database." );
      return 3;
    }
    if( size[0] < 0 || size[1] < 0 || size[2] < 0 ) {
      Warning( Here(here), "Illegal negative value for detector dimension. "
	       "Taking absolute. Check database." );
    }
    fSize[0] = 0.5 * TMath::Abs(size[0]);
    fSize[1] = 0.5 * TMath::Abs(size[1]);
    fSize[2] = TMath::Abs(size[2]);
  }
  else
    fSize[0] = fSize[1] = fSize[2] = kBig;

  if( !angles.empty() ) {
    if( angles.size() != 1 && angles.size() != 3 ) {
      Error( Here(here), "Incorrect number of values = %lu for "
	     "detector angle(s). Must be either 1 or 3. Fix database.",
	     angles.size() );
      return 4;
    }
    // If one angle is given, it indicates a rotation about y, as before.
    // If three angles are given, they are interpreted as Euler angles
    // following the y-convention, i.e. rotation about z, rotation about y',
    // rotation about z''. The sign of a rotation is defined by the right-hand
    // rule (mathematics definition): if the thumb points along the positive
    // direction of the axis, the fingers indicate a positive rotation.
    if( angles.size() == 1 ) {
      DefineAxes( angles[0] * TMath::DegToRad() );
    }
    else {
      TRotation m;
      // For TRotation (and only TRotation, it seems), ROOT's definition of the
      // sense of rotation seems to be the left-hand rule: "A rotation around a
      // specified axis means counterclockwise rotation around the positive
      // direction of the axis." But we define rotations by the right-hand rule,
      // so we need to invert all angles.
      m.SetYEulerAngles( -angles[0] * TMath::DegToRad(),
			 -angles[1] * TMath::DegToRad(),
			 -angles[2] * TMath::DegToRad() );
      fXax.SetXYZ( m.XX(), m.XY(), m.XZ() );
      fYax.SetXYZ( m.YX(), m.YY(), m.YZ() );
      fZax.SetXYZ( m.ZX(), m.ZY(), m.ZZ() );
    }
  } else
    DefineAxes(0);

  return 0;
}

//_____________________________________________________________________________
void THaDetectorBase::DebugWarning( const char* here, const char* msg,
                                    UInt_t evnum )
{
  if( fDebug > 0 ) {
    Warning( Here(here), "Event %d: %s", evnum, msg );
  }
}

//_____________________________________________________________________________
void THaDetectorBase::MultipleHitWarning( const DigitizerHitInfo_t& hitinfo,
                                          const char* here )
{
  ostringstream msg;
  msg << hitinfo.nhit << " hits on "
      << (hitinfo.type == Decoder::ChannelType::kADC ? "ADC" : "TDC")
      << " channel "
      << hitinfo.crate << "/" << hitinfo.slot << "/" << hitinfo.chan;
  ++fMessages[msg.str()];

  DebugWarning(here, msg.str().c_str(), hitinfo.ev);
}

//_____________________________________________________________________________
void THaDetectorBase::DataLoadWarning( const DigitizerHitInfo_t& hitinfo,
                                       const char* here )
{
  ostringstream msg;
  msg << "Failed to load data for "
      << (hitinfo.type == Decoder::ChannelType::kADC ? "ADC" : "TDC")
      << " channel "
      << hitinfo.crate << "/" << hitinfo.slot << "/" << hitinfo.chan
      << ". Skipping channel.";
  ++fMessages[msg.str()];

  DebugWarning(here, msg.str().c_str(), hitinfo.ev);
}

//_____________________________________________________________________________
Int_t THaDetectorBase::DefineVariables( EMode mode )
{
  // Define variables. Calls DefineVariables for all objects in fDetectorData

  Int_t ret = THaAnalysisObject::DefineVariables(mode);
  if( ret )
    return ret;

  for( auto& detData : fDetectorData )
    if( (mode == kDefine) xor detData->IsSetup() )
      detData->DefineVariables(mode);

  return kOK;
}

//_____________________________________________________________________________
Int_t THaDetectorBase::ReadDatabase( const TDatime& date )
{
  // Read database. Resets all objects in fDetectorData

  Int_t status = THaAnalysisObject::ReadDatabase(date);
  if( status != kOK )
    return status;

  for( auto& detData : fDetectorData )
    detData->Reset();

  return kOK;
}

//_____________________________________________________________________________
Int_t THaDetectorBase::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Put decoded frontend data into fDetectorData. Used by Decode().
  // hitinfo: channel info (crate/slot/channel/hit/type)
  // data:    data registered in this channel

  for( auto& detData : fDetectorData ) {
    if( !detData->HitDone() )
      detData->StoreHit(hitinfo, data);
  }

  return 0;
}

//_____________________________________________________________________________
OptUInt_t THaDetectorBase::LoadData( const THaEvData& evdata,
                                     const DigitizerHitInfo_t& hitinfo )
{
  // Default method for loading the data for the hit referenced in 'hitinfo'.
  // Callback from Decode().

  return evdata.GetData(hitinfo.crate, hitinfo.slot, hitinfo.chan, hitinfo.hit);
}

//_____________________________________________________________________________
Int_t THaDetectorBase::Decode( const THaEvData& evdata )
{
  // Generic Decode method. It loops over all active channels (= hit) in
  // 'evdata' that are associated with this detector's detector map.
  // For each hit, the following callbacks are made:
  //
  //   LoadData(evdata,hitinfo):  Returns data for the 'hitinfo' channel
  //
  //   StoreData(hitinfo,data):   Save data in member variables
  //
  // The default LoadData and StoreData may be enough for simple detectors.
  // The default StoreData sends the data to the StoreData function of all
  // 'detector data' objects that his detector has placed in fDetectorData.
  //
  // For debugging, each detector may define a PrintDecodedData function.
  // The default version calls Print on all objects in fDetectorData.

  const char* const here = "Decode";

  // Loop over all modules defined for this detector
  bool has_warning = false;
  Int_t nhits = 0;

  auto hitIter = fDetMap->MakeIterator(evdata);
  while( hitIter ) {
    const auto& hitinfo = *hitIter;
    if( hitinfo.nhit > 1 ) {
      // Multiple hits in a channel (usually noise)
      // For multifunction modules, assume "hit" is a data word index, so
      // don't log anything but assume the user simply wants the first word.
      if( hitinfo.modtype != Decoder::ChannelType::kMultiFunctionADC and
          hitinfo.modtype != Decoder::ChannelType::kMultiFunctionTDC ) {
        MultipleHitWarning(hitinfo, here);
        has_warning = true;
      }
    }

    // Get the data for this hit
    auto data = LoadData(evdata, hitinfo);
    if( !data ) {
      // Data could not be retrieved (probably decoder bug)
      DataLoadWarning(hitinfo, here);
      has_warning = true;
      continue;
    }

    // Store hit data (and derived quantities) in fDetectorData.
    // Multi-function modules can load additional data here.
    StoreHit(hitinfo, data.value());

    // Clear the hit-done flag which can be used in custom StoreHit methods
    // to reorder module processing
    for( auto& detData : fDetectorData )
      detData->ClearHitDone();

    // Next active channel
    ++hitIter;
    ++nhits;
  }
  if( has_warning )
    ++fNEventsWithWarnings;

#ifdef WITH_DEBUG
  if ( fDebug > 3 )
    PrintDecodedData(evdata);
#endif

  return nhits;
}

//_____________________________________________________________________________
void THaDetectorBase::PrintDecodedData( const THaEvData& /*evdata*/ ) const
{
  // Default Print function for decoded data

  //TODO implement
}

//_____________________________________________________________________________
ClassImp(THaDetectorBase)

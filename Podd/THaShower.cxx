///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaShower                                                                 //
//                                                                           //
// Shower counter class, describing a generic segmented shower detector      //
// (preshower or shower).                                                    //
// Currently, only the "main" cluster, i.e. cluster with the largest energy  //
// deposition is considered. Units of measurements are MeV for energy of     //
// shower and meters for coordinates.                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaShower.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "TDatime.h"
#include "TMath.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <iterator>
#include <vector>

using namespace std;
using namespace Podd;

//_____________________________________________________________________________
THaShower::THaShower( const char* name, const char* description,
		      THaApparatus* apparatus )
  : THaPidDetector(name,description,apparatus)
  , fNrows(0)
  , fEmin(0)
  , fAsum_p(kBig)
  , fAsum_c(kBig)
  , fNclust(0)
  , fE(kBig)
  , fX(kBig)
  , fY(kBig)
  , fADCData(nullptr)
{
  // Constructor
}

//_____________________________________________________________________________
THaShower::THaShower()
  : fNrows(0)
  , fEmin(0)
  , fAsum_p(kBig)
  , fAsum_c(kBig)
  , fNclust(0)
  , fE(kBig)
  , fX(kBig)
  , fY(kBig)
  , fADCData(nullptr)
{
  // Default constructor (for ROOT I/O)
}

//_____________________________________________________________________________
Int_t THaShower::ReadDatabase( const TDatime& date )
{
  // Read parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  const char* const here = "ReadDatabase";

  constexpr VarType kDataType  = std::is_same_v<Data_t, Float_t> ? kFloat  : kDouble;
  constexpr VarType kDataTypeV = std::is_same_v<Data_t, Float_t> ? kFloatV : kDoubleV;

  // Read database
  Int_t err = THaPidDetector::ReadDatabase(date);
  if( err )
    return err;

  CFile file = OpenFile( date );
  if( !file ) return kFileError;

  // Read fOrigin and fSize (required!)
  err = ReadGeometry( file, date, true );
  if( err )
    return err;

  err = ReadDetMap( file, date, 0 );
  if( err )
    return err;

  // All DAQ modules are assumed to be ADCs
  UInt_t nmodules = fDetMap->GetSize();
  for( UInt_t i = 0; i < nmodules; i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule(i);
    if( !d->model ) {
      d->MakeADC();
    }
  }

  vector<Int_t> chanmap;
  vector<Double_t> xy, dxy;
  Int_t ncols = 0, nrows = 0;

  // Read mapping/geometry/configuration parameters
  const vector<DBRequest> config_request = {
    { .name = "chanmap", .var = &chanmap, .type = kIntV,  .optional = true },
    { .name = "ncols",   .var = &ncols,   .type = kInt },
    { .name = "nrows",   .var = &nrows,   .type = kInt },
    { .name = "xy",      .var = &xy,      .type = kDoubleV, .nelem = 2 },  // center pos of block 1
    { .name = "dxdy",    .var = &dxy,     .type = kDoubleV, .nelem = 2 },  // dx and dy block spacings
    { .name = "emin",    .var = &fEmin,   .type = kDataType },
  };
  err = LoadDatabase( file, date, config_request, fPrefix );
  if( err )
    return err;

  // Sanity checks
  if( nrows <= 0 || ncols <= 0 ) {
    Error( Here(here), "Illegal number of rows or columns: %d %d. Must be > 0. "
           "Fix database.", nrows, ncols );
    return kInitError;
  }

  Int_t nelem = ncols * nrows; 
  Int_t nclbl = TMath::Min( 3, nrows ) * TMath::Min( 3, ncols );

  // Reinitialization only possible for same basic configuration
  if( fIsInit && nelem != fNelem ) {
    Error( Here(here), "Cannot re-initialize with different number of blocks or "
           "blocks per cluster (was: %d, now: %d). Detector not re-initialized.",
           fNelem, nelem );
    return  kInitError;
  }
  fNelem = nelem;
  fNrows = nrows;

  assert( fNelem >= 0 );
  const auto N = static_cast<UInt_t>(fNelem);

  // Clear out the old channel map before reading a new one
  fChanMap.clear();
  if( UInt_t tot_nchan = fDetMap->GetTotNumChan(); tot_nchan != N ) {
    Error(Here(here), "Number of detector map channels (%u) "
          "inconsistent with number of blocks (%d)", tot_nchan, nelem);
    return kInitError;
  }

  if( !chanmap.empty() ) {
    // If a map is found in the database, ensure it has the correct size
    if( size_t cmapsize = chanmap.size(); cmapsize != N ) {
      Error(Here(here), "Channel map size (%lu) and number of detector "
                        "channels (%d) must be equal. Fix database.",
            cmapsize, nelem);
      return kInitError;
    }
    // Set up the new channel map. The index into the map is the physical
    // channel (sequence number in the detector map), the value at that index,
    // the logical channel. If the map is empty, the mapping is 1-1.
    fChanMap.assign(chanmap.begin(), chanmap.end());
  }

  fBlockPos.clear(); fBlockPos.resize(nelem);
  fClBlk.clear();    fClBlk.reserve(nclbl);
  fChannelData.clear();

  fChannelData.push_back(
    make_unique<ShowerADCData>(GetPrefixName(), fTitle, N, fChanMap));
  fADCData = dynamic_cast<ShowerADCData*>(fChannelData.back().get());
  assert( fADCData && fADCData->GetSize() == N );
  fIsInit = true;

  // Compute block positions
  for( int c=0; c<ncols; c++ ) {
    for( int r=0; r<nrows; r++ ) {
      int k = nrows*c + r;
      // Units are meters
      fBlockPos[k].x = xy[0] + r * dxy[0];
      fBlockPos[k].y = xy[1] + c * dxy[1];
    }
  }

  // Read calibration parameters

  // Read ADC pedestals and gains
  // (in order of **logical** channel number = block number)
  vector<Int_t> ped;
  vector<Data_t> gain;
  ped.assign(nelem, 0.0);
  gain.assign(nelem, 1.0);
  const vector<DBRequest> calib_request = {
    { .name = "pedestals", .var = &ped,  .type = kIntV,      .nelem = N, .optional = true },
    { .name = "gains",     .var = &gain, .type = kDataTypeV, .nelem = N, .optional = true },
  };
  err = LoadDB( file, date, calib_request );
  if( err )
    return err;

  if( !(ped.empty() && gain.empty()) ) {
    for( UInt_t i = 0; i < N; ++i ) {
      auto& calib = fADCData->GetCalib(i);
      if( !ped.empty() )
        calib.ped = TMath::Max(ped[i], 0);
      if( !gain.empty() )
        calib.gain = gain[i];
    }
  }

#ifdef WITH_DEBUG
  // Debug printout
  if ( fDebug > 2 ) {
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    const vector<DBRequest> list = {
      { .name = "Number of blocks",       .var = &fNelem,  .type = kInt                   },
      { .name = "Detector center",        .var = pos,                          .nelem = 3 },
      { .name = "Detector size",          .var = fSize,    .type = kDouble,    .nelem = 3 },
      { .name = "Channel map",            .var = &chanmap, .type = kIntV                  },
      { .name = "Position of block 1",    .var = &xy,      .type = kDoubleV               },
      { .name = "Block x/y spacings",     .var = &dxy,     .type = kDoubleV               },
      { .name = "Minimum cluster energy", .var = &fEmin,   .type = kDataType,  .nelem = 1 },
      { .name = "ADC pedestals",          .var = &ped,     .type = kIntV,      .nelem = N },
      { .name = "ADC gains",              .var = &gain,    .type = kDataTypeV, .nelem = N },
    };
    DebugPrint( list );
  }
#endif

  return kOK;
}

//_____________________________________________________________________________
Int_t THaShower::DefineVariables( EMode mode )
{
  // Initialize global variables

  // Set up standard variables, including the objects in fChannelData
  if( Int_t ret = THaPidDetector::DefineVariables(mode) )
    return ret;

  const vector<RVarDef> vars = {
    { .name = "asum_p", .desc = "Sum of ped-subtracted ADCs",         .def = "fAsum_p" },
    { .name = "asum_c", .desc = "Sum of calibrated ADCs",             .def = "fAsum_c" },
    { .name = "nclust", .desc = "Number of clusters",                 .def = "fNclust" },
    { .name = "e",      .desc = "Energy (MeV) of largest cluster",    .def = "fE" },
    { .name = "x",      .desc = "x-position of largest cluster",      .def = "fX" },
    { .name = "y",      .desc = "y-position of largest cluster",      .def = "fY" },
    { .name = "mult",   .desc = "Multiplicity of largest cluster",    .def = "GetMainClusterSize()" },
    { .name = "nblk",   .desc = "Numbers of blocks in main cluster",  .def = "fClBlk.n" },
    { .name = "eblk",   .desc = "Energies of blocks in main cluster", .def = "fClBlk.E" },
    { .name = nullptr }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaShower::~THaShower()
{
  // Destructor. Removes global variables.

  RemoveVariables();
}

//_____________________________________________________________________________
void THaShower::Clear( Option_t* opt )
{
  // Clear event data

  THaPidDetector::Clear(opt);
  fAsum_p = fAsum_c = 0.0;
  fE = fX = fY = kBig;
  fClBlk.clear();
}

//_____________________________________________________________________________
Int_t THaShower::
ShowerADCData::GetLogicalChannel( const DigitizerHitInfo_t& hitinfo ) const
{
  // Get the sequence number in the detector map
  Int_t k = ADCData::GetLogicalChannel(hitinfo);

  // Translate to logical channel number. Recall that logical channel
  // numbers in the map start at 1, but array indices need to start at 0,
  // so we subtract 1.
  if( !fChanMap.empty() )
#ifdef NDEBUG
    k = fChanMap[k] - 1;
#else
    k = fChanMap.at(k) - 1;
#endif
  return k;
}

//_____________________________________________________________________________
Int_t THaShower::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Put decoded frontend data into fChannelData. Used by Decode().
  // hitinfo: channel info (crate/slot/channel/hit/type)
  // data:    data registered in this channel

  THaPidDetector::StoreHit(hitinfo, data);

  // Add channels with signals to the amplitude sums
  const auto& ADC = fADCData->GetADC(hitinfo);
  if( ADC.adc_p > 0 )
    fAsum_p += ADC.adc_p;             // Sum of ADC minus ped
  if( ADC.adc_c > 0 )
    fAsum_c += ADC.adc_c;             // Sum of ADC corrected

  return 0;
}

//_____________________________________________________________________________
Int_t THaShower::CoarseProcess( TClonesArray& tracks )
{
  // Reconstruct Clusters in shower detector and copy the data
  // into the following local data structure:
  //
  // fNclust        -  Number of clusters in shower
  // fE             -  Energy (in MeV) of the "main" cluster
  // fX             -  X-coordinate (in m) of the cluster
  // fY             -  Y-coordinate (in m) of the cluster
  // fClBlk         -  Numbers and energies of blocks composing the cluster
  //
  // Only one ("main") cluster, i.e. the cluster with the largest energy
  // deposition is considered. Units are MeV for energies and meters for
  // coordinates.

  fNclust = 0;
  fClBlk.clear();

  // Find block with maximum energy deposit
  int nmax = -1;
  double emax = fEmin;                      // Min threshold of energy in center
  for( int i = 0; i < fNelem; i++ ) {       // Find the block with max energy:
    double ei = fADCData->GetADC(i).adc_c;  // Energy in next block
    if( ei > 0.5*kBig ) continue;           // Skip invalid data
    if( ei > emax ) {
      nmax = i;                             // Number of block with max energy
      emax = ei;                            // Max energy per a blocks
    }
  }
  if ( nmax >= 0 ) {
    int nc = nmax/fNrows;                   // Column of the block with max energy
    int nr = nmax%fNrows;                   // Row of the block with max energy

    fClBlk.push_back( {nmax,emax} );        // Add the block to cluster (center)
    double sxe = emax * fBlockPos[nmax].x;  // Sum of xi*ei
    double sye = emax * fBlockPos[nmax].y;  // Sum of yi*ei
    for( int i = 0; i < fNelem; i++ ) {     // Detach surround blocks:
      double ei = fADCData->GetADC(i).adc_c;// Energy in next block
      if( ei > 0.5*kBig ) continue;         // Skip invalid data
      if( ei > 0 && i != nmax ) {           // Some energy out of cluster center
        int ic = i / fNrows;                // Column of next block
        int ir = i % fNrows;                // Row of next block
        int dr = nr - ir;                   // Distance on row up to center
        int dc = nc - ic;                   // Distance on column up to center
        if( -2<dr && dr<2 && -2<dc && dc<2 ) {   // Surround block:
                                            // Add block to cluster (surround)
          fClBlk.push_back( {i,ei} );       // Add surround block to cluster
          sxe += ei * fBlockPos[i].x;       // Sum of xi*ei of cluster blocks
          sye += ei * fBlockPos[i].y;       // Sum of yi*ei of cluster blocks
          emax += ei;                       // Sum of energies in cluster blocks
        }
      }
    }
    fNclust = 1;                            // One ("main") cluster detected
    fE      = emax;                         // Energy (MeV) in "main" cluster
    fX      = sxe/emax;                     // X coordinate (m) of the cluster
    fY      = sye/emax;                     // Y coordinate (m) of the cluster
  }

  // Calculate track projections onto shower plane

  CalcTrackProj( tracks );

  return 0;
}

//_____________________________________________________________________________
Int_t THaShower::FineProcess( TClonesArray& tracks )
{
  // Fine Shower processing.

  // Redo the track-matching, since tracks might have been thrown out
  // during the FineTracking stage.

  CalcTrackProj( tracks );

  return 0;
}

//_____________________________________________________________________________
void THaShower::PrintDecodedData( const THaEvData& evdata ) const
{
  // Print decoded data (for debugging). Called form Decode()

  cout << "Event " << evdata.GetEvNum() << "   Trigger " << evdata.GetEvType()
       << " Shower Detector " << GetPrefix() << endl;
  int ncol = 3;
  for( int i = 0; i < ncol; i++ ) {
    cout << "  Block  ADC  ADC_p  ";
  }
  cout << endl;

  for( int i = 0; i < (fNelem + ncol - 1) / ncol; i++ ) {
    for( int c = 0; c < ncol; c++ ) {
      int ind = c * fNelem / ncol + i;
      if( ind < fNelem ) {
        const auto& ADC = fADCData->GetADC(ind);
        cout << "  " << setw(3) << ind + 1;
        cout << "  ";
        WriteValue(ADC.adc);
        cout << "  ";
        WriteValue(ADC.adc_p);
        cout << "  ";
      } else {
//	  cout << endl;
        break;
      }
    }
    cout << endl;
  }
}

//_____________________________________________________________________________
#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(THaShower)
#endif

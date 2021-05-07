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
#include <cstdlib>
#include <iterator>

using namespace std;
using namespace Podd;

// Macro for better readability
#if __cplusplus >= 201402L
# define MKADCDATA(name,title,nelem,chanmap) make_unique<ShowerADCData>((name),(title),(nelem),(chanmap))
#else
# define MKADCDATA(name,title,nelem,chanmap) unique_ptr<ShowerADCData>(new ShowerADCData((name),(title),(nelem),(chanmap)))
#endif

//_____________________________________________________________________________
THaShower::THaShower( const char* name, const char* description,
		      THaApparatus* apparatus ) :
  THaPidDetector(name,description,apparatus),
  fNrows(0), fEmin(0), fAsum_p(kBig), fAsum_c(kBig),
  fNclust(0), fE(kBig), fX(kBig), fY(kBig), fADCData(nullptr)
{
  // Constructor
}

//_____________________________________________________________________________
THaShower::THaShower() :
  THaPidDetector(),
  fNrows(0), fEmin(0), fAsum_p(kBig), fAsum_c(kBig),
  fNclust(0), fE(kBig), fX(kBig), fY(kBig), fADCData(nullptr)
{
  // Default constructor (for ROOT I/O)
}

//_____________________________________________________________________________
Int_t THaShower::ReadDatabase( const TDatime& date )
{
  // Read parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  const char* const here = "ReadDatabase";

  VarType kDataType  = std::is_same<Data_t, Float_t>::value ? kFloat  : kDouble;
  VarType kDataTypeV = std::is_same<Data_t, Float_t>::value ? kFloatV : kDoubleV;

  // Read database
  Int_t err = THaPidDetector::ReadDatabase(date);
  if( err )
    return err;

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // Read fOrigin and fSize (required!)
  err = ReadGeometry( file, date, true );
  if( err ) {
    fclose(file);
    return err;
  }

  vector<Int_t> detmap, chanmap;
  vector<Double_t> xy, dxy;
  Int_t ncols = 0, nrows = 0;

  // Read mapping/geometry/configuration parameters
  DBRequest config_request[] = {
    { "detmap",       &detmap,  kIntV },
    { "chanmap",      &chanmap, kIntV,    0, true },
    { "ncols",        &ncols,   kInt },
    { "nrows",        &nrows,   kInt },
    { "xy",           &xy,      kDoubleV, 2 },  // center pos of block 1
    { "dxdy",         &dxy,     kDoubleV, 2 },  // dx and dy block spacings
    { "emin",         &fEmin,   kDataType },
    { nullptr }
  };
  err = LoadDB( file, date, config_request, fPrefix );

  // Sanity checks
  if( !err && (nrows <= 0 || ncols <= 0) ) {
    Error( Here(here), "Illegal number of rows or columns: %d %d. Must be > 0. "
	   "Fix database.", nrows, ncols );
    err = kInitError;
  }

  Int_t nelem = ncols * nrows; 
  Int_t nclbl = TMath::Min( 3, nrows ) * TMath::Min( 3, ncols );

  // Reinitialization only possible for same basic configuration
  if( !err ) {
    if( fIsInit && nelem != fNelem ) {
      Error( Here(here), "Cannot re-initialize with different number of blocks or "
	     "blocks per cluster (was: %d, now: %d). Detector not re-initialized.",
	     fNelem, nelem );
      err = kInitError;
    } else {
      fNelem = nelem;
      fNrows = nrows;
    }
  }
  assert( fNelem >= 0 );
  UInt_t nval = fNelem;

  if( !err ) {
    // Clear out the old channel map before reading a new one
    fChanMap.clear();
    if( FillDetMap(detmap, 0, here) <= 0 ) {
      err = kInitError;  // Error already printed by FillDetMap
    } else {
      UInt_t tot_nchan = fDetMap->GetTotNumChan();
      if( tot_nchan != nval ) {
        Error(Here(here), "Number of detector map channels (%u) "
                          "inconsistent with number of blocks (%u)",
              tot_nchan, nval);
        err = kInitError;
      }
    }
  }
  if( !err && !chanmap.empty() ) {
    // If a map is found in the database, ensure it has the correct size
    size_t cmapsize = chanmap.size();
    if( cmapsize != nval ) {
      Error(Here(here), "Channel map size (%lu) and number of detector "
                        "channels (%u) must be equal. Fix database.",
            cmapsize, nval);
      err = kInitError;
    }
    if( !err ) {
      // Set up the new channel map. The index into the map is the physical
      // channel (sequence number in the detector map), the value at that index,
      // the logical channel. If the map is empty, the mapping is 1-1.
      fChanMap.assign(chanmap.begin(), chanmap.end());
    }
  }

  if( err ) {
    fclose(file);
    return err;
  }

  fBlockPos.clear(); fBlockPos.reserve(nval);
  fClBlk.clear();    fClBlk.reserve(nclbl);
  fDetectorData.clear();
  auto detdata = MKADCDATA(GetPrefixName(), fTitle, nval, fChanMap);
  fADCData = detdata.get();
  fDetectorData.emplace_back(std::move(detdata));
  fIsInit = true;
  assert( fADCData->GetSize() == nval );

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
  vector<Data_t> ped, gain;
  DBRequest calib_request[] = {
    { "pedestals",    &ped,   kDataTypeV, nval, true },
    { "gains",        &gain,  kDataTypeV, nval, true },
    { nullptr }
  };
  err = LoadDB( file, date, calib_request, fPrefix );
  fclose(file);
  if( err )
    return err;

  for( UInt_t i = 0; i < nval; ++i ) {
    auto& calib = fADCData->GetCalib(i);
    calib.ped   = ped[i];
    calib.gain  = gain[i];
  }

#ifdef WITH_DEBUG
  // Debug printout
  if ( fDebug > 2 ) {
    const auto N = static_cast<UInt_t>(fNelem);
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    DBRequest list[] = {
      { "Number of blocks",       &fNelem,     kInt        },
      { "Detector center",        pos,         kDouble, 3  },
      { "Detector size",          fSize,       kDouble, 3  },
      { "Channel map",            &chanmap,    kIntV       },
      { "Position of block 1",    &xy,         kDoubleV    },
      { "Block x/y spacings",     &dxy,        kDoubleV    },
      { "Minimum cluster energy", &fEmin,      kDataType,  1  },
      { "ADC pedestals",          &ped,        kDataTypeV,  N  },
      { "ADC gains",              &gain,       kDataTypeV,  N  },
      { nullptr }
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

  // Set up standard variables, including the objects in fDetectorData
  Int_t ret = THaPidDetector::DefineVariables(mode);
  if( ret )
    return ret;

  RVarDef vars[] = {
    { "asum_p", "Sum of ped-subtracted ADCs",         "fAsum_p" },
    { "asum_c", "Sum of calibrated ADCs",             "fAsum_c" },
    { "nclust", "Number of clusters",                 "fNclust" },
    { "e",      "Energy (MeV) of largest cluster",    "fE" },
    { "x",      "x-position of largest cluster",      "fX" },
    { "y",      "y-position of largest cluster",      "fY" },
    { "mult",   "Multiplicity of largest cluster",    "GetMainClusterSize()" },
    { "nblk",   "Numbers of blocks in main cluster",  "fClBlk.n" },
    { "eblk",   "Energies of blocks in main cluster", "fClBlk.E" },
    { nullptr }
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
  // Put decoded frontend data into fDetectorData. Used by Decode().
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
ClassImp(THaShower)

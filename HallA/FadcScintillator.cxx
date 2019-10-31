///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// FadcScintillator (11202017 Hanjie Liu hanjie@jlab.org)                    //
// Using FADC read out scintillator PMT                                      //
// inherited from THaScintillator with some special FADC variabled added;    //
//                                                                           //
// Class for a scintillator (hodoscope) consisting of multiple               //
// paddles with phototubes on both ends.                                     //
//									     //          
///////////////////////////////////////////////////////////////////////////////

#include "FadcScintillator.h"
#include "THaDetMap.h"
#include "THaEvData.h"
#include "VarDef.h"
#include "VarType.h"
#include "Fadc250Module.h"
#include <cstdio>
#include <cassert>
#include <algorithm>

#define ALL(c) (c).begin(), (c).end()

using namespace std;
using namespace Decoder;

namespace HallA {

//_____________________________________________________________________________
FadcScintillator::FadcScintillator( const char* name, const char* description,
                                    THaApparatus* apparatus )
  : THaScintillator(name, description, apparatus)
{
  // Constructor
}

//_____________________________________________________________________________
FadcScintillator::FadcScintillator() : THaScintillator()
{
  // Default constructor (for ROOT I/O)

}

//_____________________________________________________________________________
Int_t FadcScintillator::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database

  Int_t err = THaScintillator::ReadDatabase(date);
  if( err )
    return err;
  fIsInit = false;

  FILE* file = OpenFile(date);
  if( !file ) return kFileError;

  // Dimension arrays
  fFADCDataR.resize(fNelem);
  fFADCDataL.resize(fNelem);

  // Configuration parameters (mandatory)
  fFADCConfig.clear();
  DBRequest calib_request[] = {
    { "NPED",  &fFADCConfig.nped,  kInt },
    { "NSA",   &fFADCConfig.nsa,   kInt },
    { "NSB",   &fFADCConfig.nsb,   kInt },
    { "Win",   &fFADCConfig.win,   kInt },
    { "TFlag", &fFADCConfig.tflag, kInt },
    { nullptr }
  };
  err = LoadDB(file, date, calib_request, fPrefix);
  fclose(file);
  if( err )
    return err;

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t FadcScintillator::DefineVariables( EMode mode )
{
  // Initialize global variables and lookup table for decoder

  if( mode == kDefine && fIsSetup ) return kOK;
  Int_t ret = THaScintillator::DefineVariables(mode);
  if( ret )
    return ret;

  // Register variables in global list

  RVarDef vars[] = {
    { "lpeak",      "FADC ADC peak values left side",     "fFADCDataL.fPeak" },
    { "rpeak",      "FADC ADC peak values right side",    "fFADCDataR.fPeak" },
    { "lt_fadc",    "FADC TDC values left side",          "fFADCDataL.fT" },
    { "ltc_fadc",   "FADC Corrected times left side",     "fFADCDataL.fT_c" },
    { "rt_fadc",    "FADC TDC values right side",         "fFADCDataR.fT" },
    { "rtc_fadc",   "FADC Corrected times right side",    "fFADCDataR.fT_c" },
    { "loverflow",  "overflow bit of FADC pulse",         "fFADCDataL.fOverflow" },
    { "lunderflow", "underflow bit of FADC pulse",        "fFADCDataL.fUnderflow" },
    { "lbadped",    "pedestal quality bit of FADC pulse", "fFADCDataL.fPedq" },
    { "roverflow",  "overflow bit of FADC pulse",         "fFADCDataR.fOverflow" },
    { "runderflow", "underflow bit of FADC pulse",        "fFADCDataR.fUnderflow" },
    { "rbadped",    "pedestal quality bit of FADC pulse", "fFADCDataR.fPedq" },
    { nullptr }
  };
  ret = DefineVarsFromList(vars, mode);
  if( ret == kOK )
    fIsSetup = (mode == kDefine);
  return ret;
}

//_____________________________________________________________________________
FadcScintillator::~FadcScintillator()
{
  // Destructor. Remove variables from global list.

  if( fIsSetup )
    RemoveVariables();
}

//_____________________________________________________________________________
void FadcScintillator::Clear( Option_t* opt )
{
  // Reset per-event data.
  THaScintillator::Clear(opt);

  for_each(ALL(fFADCDataR), []( FADCData_t& d ) { d.clear(); });
  for_each(ALL(fFADCDataL), []( FADCData_t& d ) { d.clear(); });
}

//_____________________________________________________________________________
void FadcScintillator::SetupModule( const THaEvData& evdata,
                                    THaDetMap::Module* pModule )
{
  // Callback from THaScintillator::Decode() called once for each
  // defined detector map module 'pModule'

  THaScintillator::SetupModule(evdata, pModule);  // sets fModule

  fFADC = dynamic_cast<Decoder::Fadc250Module*>(fModule);
}

//_____________________________________________________________________________
struct FADCItem_t {
  FADCItem_t( EModuleType _type, Int_t _chan, Int_t _hit )
    : type(_type), chan(_chan), hit(_hit), val(0), good(false) {}
  const EModuleType type;
  Int_t  chan, hit, val;
  Bool_t good;
};

inline static Int_t GetFADCItem( Decoder::Module* module, FADCItem_t& m )
{
  m.good = (module->HasCapability(m.type) &&
            m.hit < module->GetNumEvents(m.type, m.chan) );
  m.val = m.good ? module->GetData(m.type, m.chan, m.hit)
                 : 0; // Invalid data
  return m.good;
}

//_____________________________________________________________________________
THaScintillator::OptInt_t
FadcScintillator::LoadData( const THaEvData& evdata, DetMapItem* d, Bool_t adc,
                            Int_t chan, Int_t hit, Int_t pad, ESide side )
{
  // Callback from Decoder for loading the data for 'hit' in 'chan' of
  // module 'd', destined for 'pad' on 'side'. 'adc' indicates that ADC
  // data should be loaded, otherwise load TDC data for this channel/hit.

  Int_t data = 0;
  Bool_t good = false;
  if( adc ) {
    if( fFADC ) {
      auto& FDAT = (side == kRight) ? fFADCDataR[pad] : fFADCDataL[pad];
      FADCItem_t items[4] = { { kPulseIntegral, chan, hit},
                              { kPulsePeak,     chan, hit},
                              { kPulseTime,     chan, hit},
                              { kPulsePedestal, chan, 0} };
      for ( auto& item : items ) {
        GetFADCItem(fFADC, item );
        if( !item.good ) {
          // TODO: error! Decoder's GetNumHits lied to us!
        }
      }
      data            = items[0].val;
      FDAT.fPeak      = items[1].good ? items[1].val : kBig;
      FDAT.fT         = items[2].good ? items[2].val : kBig;
      FDAT.fT_c       = items[2].good ? FDAT.fT * 0.0625 : kBig; //FIXME: make this mystery factor configurable
      FDAT.fOverflow  = fFADC->GetOverflowBit(chan, 0);  // TODO: hit=0 correct?
      FDAT.fUnderflow = fFADC->GetUnderflowBit(chan, 0);
      FDAT.fPedq      = fFADC->GetPedestalQuality(chan, 0);
      // Retrieve pedestal, if available
      if( FDAT.fPedq == 0 && items[3].good ) {
        Int_t ped = items[3].val;
        auto& calib = fCalib[side][pad];
        if( fFADCConfig.tflag ) {
          calib.ped = static_cast<Double_t>(ped) *
                      (fFADCConfig.nsa + fFADCConfig.nsb) / fFADCConfig.nped;
        } else {
          calib.ped = static_cast<Double_t>(ped) * fFADCConfig.win / fFADCConfig.nped;
        }
      }
      good = items[0].good;
    } else {
      //TODO: error! Bad configuration. ADCs must be FADC250s
    }
  } else {
    // Load TDCs like the standard scintillator does. This may change when/if
    // we install pipelined TDCs of some kind
    return THaScintillator::LoadData(evdata, d, adc, chan, hit, pad, side);
  }

  return make_pair(data,good);
}

} // namespace HallA

//_____________________________________________________________________________
ClassImp(HallA::FadcScintillator)

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

  FILE* file = OpenFile(date);
  if( !file ) return kFileError;

  // Dimension arrays
  UInt_t nval = fNelem;
  if( !fIsInit ) {
    // Per-event data
    fFADCDataR.resize(nval);
    fFADCDataL.resize(nval);

    fIsInit = true;
  }

  // Configuration parameters (mandatory)
  fFADCConfig.clear();
  DBRequest calib_request[] = {
    { "NPED",             &fFADCConfig.nped,        kInt },
    { "NSA",              &fFADCConfig.nsa,         kInt },
    { "NSB",              &fFADCConfig.nsb,         kInt },
    { "Win",              &fFADCConfig.win,         kInt },
    { "TFlag",            &fFADCConfig.tflag,       kInt },
    { nullptr }
  };
  err = LoadDB(file, date, calib_request, fPrefix);
  fclose(file);
  if( err )
    return err;

  return kOK;
}

//_____________________________________________________________________________
Int_t FadcScintillator::DefineVariables( EMode mode )
{
  // Initialize global variables and lookup table for decoder

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = (mode == kDefine);

  // Register variables in global list

  RVarDef vars[] = {
    { "lpeak",      "FADC ADC peak values left side",     "fPeak" },
    { "rpeak",      "FADC ADC peak values right side",    "fRPeak" },
    { "lt_fadc",    "FADC TDC values left side",          "fT" },
    { "ltc_fadc",   "FADC Corrected times left side",     "fT_c" },
    { "rt_fadc",    "FADC TDC values right side",         "fRT_FADC" },
    { "rtc_fadc",   "FADC Corrected times right side",    "fRT_FADC_c" },
    { "loverflow",  "overflow bit of FADC pulse",         "fOverflow" },
    { "lunderflow", "underflow bit of FADC pulse",        "fUnderflow" },
    { "lbadped",    "pedestal quality bit of FADC pulse", "fPedq" },
    { "roverflow",  "overflow bit of FADC pulse",         "froverflow" },
    { "runderflow", "underflow bit of FADC pulse",        "frunderflow" },
    { "rbadped",    "pedestal quality bit of FADC pulse", "frpedq" },
    { "lnhits",     "Number of hits for left PMT",        "fLNhits" },
    { "rnhits",     "Number of hits for right PMT",       "fRNhits" },
    { nullptr }
  };
  return DefineVarsFromList(vars, mode);
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

  for_each( ALL(fFADCDataR), [](FADCData_t& d){ d.clear(); });
  for_each( ALL(fFADCDataL), [](FADCData_t& d){ d.clear(); });
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
Int_t FadcScintillator::LoadData( const THaEvData& evdata,
                                  THaDetMap::Module* d, Bool_t adc,
                                  Int_t chan, Int_t hit, Int_t pad, ESide side )
{
  // Callback from Decoder for loading the data for 'hit' in 'chan' of
  // module 'd', destined for 'pad' on 'side'.

  Int_t data;
  auto& F = (side==kRight) ? fFADCDataR[pad] : fFADCDataL[pad];
  if( adc ) {
    if( fFADC ) {
      data = evdata.GetData(kPulseIntegral, d->crate, d->slot, chan, hit);
      F.fPeak = evdata.GetData(kPulsePeak, d->crate, d->slot, chan, hit);
      F.fT = evdata.GetData(kPulseTime, d->crate, d->slot, chan, hit);
      F.fT_c = F.fT * 0.0625; //FIXME: make this mystery factor configurable

      if( fFADC ) {
        F.fOverflow = fFADC->GetOverflowBit(chan, 0);
        F.fUnderflow = fFADC->GetUnderflowBit(chan, 0);
        F.fPedq = fFADC->GetPedestalQuality(chan, 0);
        if( F.fPedq == 0 ) {
          auto& calib = fCalib[side][pad];
          Int_t ped = evdata.GetData(kPulsePedestal, d->crate, d->slot, chan, 0);
          if( fFADCConfig.tflag ) {
            calib.ped = static_cast<Double_t>(ped) *
                        (fFADCConfig.nsa + fFADCConfig.nsb) / fFADCConfig.nped;
          } else {
            calib.ped = static_cast<Double_t>(ped) * fFADCConfig.win / fFADCConfig.nped;
          }
        }
      }
    } else {
      //TODO: error! Bad configuration. ADCs must be FADC250s
      data = 0;
    }
  } else {
    data = evdata.GetData(d->crate, d->slot, chan, hit);
  }

  return data;
}

} // namespace HallA

//_____________________________________________________________________________
ClassImp(HallA::FadcScintillator)

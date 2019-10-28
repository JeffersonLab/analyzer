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
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <algorithm>

#define ALL(c) (c).begin(), (c).end()

using namespace std;
using namespace Decoder;

namespace HallA {

//_____________________________________________________________________________
FadcScintillator::FadcScintillator( const char* name, const char* description,
                                    THaApparatus* apparatus )
  : THaScintillator(name, description, apparatus), fFADC(nullptr)
{
  // Constructor
}

//_____________________________________________________________________________
FadcScintillator::FadcScintillator() : THaScintillator(), fFADC(nullptr)
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
    fFADCData.resize(nval);

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
    { "lpeak",      "FADC ADC peak values left side",     "fLPeak" },
    { "rpeak",      "FADC ADC peak values right side",    "fRPeak" },
    { "lt_fadc",    "FADC TDC values left side",          "fLT_FADC" },
    { "ltc_fadc",   "FADC Corrected times left side",     "fLT_FADC_c" },
    { "rt_fadc",    "FADC TDC values right side",         "fRT_FADC" },
    { "rtc_fadc",   "FADC Corrected times right side",    "fRT_FADC_c" },
    { "loverflow",  "overflow bit of FADC pulse",         "floverflow" },
    { "lunderflow", "underflow bit of FADC pulse",        "flunderflow" },
    { "lbadped",    "pedestal quality bit of FADC pulse", "flpedq" },
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

  for_each( ALL(fFADCData), [](FADCData_t& d){ d.clear(); });
}

//_____________________________________________________________________________
Int_t FadcScintillator::Decode( const THaEvData& evdata )
{
  // Decode scintillator data, correct TDC times and ADC amplitudes, and copy
  // the data to the local data members.
  // This implementation makes the following assumptions about the detector map:
  // - The first half of the map entries corresponds to ADCs,
  //   the second half, to TDCs.
  // - The first fNelem detector channels correspond to the PMTs on the
  //   right hand side, the next fNelem channels, to the left hand side.
  //   (Thus channel numbering for each module must be consecutive.)

  // Loop over all modules defined for this detector

  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule(i);
    bool adc = (d->model ? fDetMap->IsADC(d) : (i < fDetMap->GetSize() / 2));

    if( adc ) fFADC = dynamic_cast <Fadc250Module*> (evdata.GetModule(d->crate, d->slot));

    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan(d->crate, d->slot); j++ ) {

      Int_t chan = evdata.GetNextChan(d->crate, d->slot, j);
      if( chan < d->lo || chan > d->hi ) continue;     // Not one of my channels

#ifdef WITH_DEBUG
      Int_t nhit = evdata.GetNumHits(d->crate, d->slot, chan);
      if( nhit > 1 )
        Warning(Here("Decode"), "%d hits on %s channel %d/%d/%d",
                nhit, adc ? "ADC" : "TDC", d->crate, d->slot, chan);
#endif
      // Get the detector channel number, starting at 0
      Int_t k = d->first + ((d->reverse) ? d->hi - chan : chan - d->lo) - 1;

#ifdef WITH_DEBUG
      if( k < 0 || k > NSIDES * fNelem ) {
        // Indicates bad database
        Warning(Here("Decode()"), "Illegal detector channel: %d", k);
        continue;
      }
#endif
      // Copy the data to the local variables.
//      DataDest* dest = fDataDest + k / fNelem;
      //decide whether use event by event pedestal
      int jj = k / fNelem;
      k = k % fNelem;

      Int_t data, ftime = 0, fpeak = 0;
      if( adc ) {
        data = evdata.GetData(kPulseIntegral, d->crate, d->slot, chan, 0);
        ftime = evdata.GetData(kPulseTime, d->crate, d->slot, chan, 0);
        fpeak = evdata.GetData(kPulsePeak, d->crate, d->slot, chan, 0);
      } else {
        if( jj == 0 ) {
          fRNhits[k] = evdata.GetNumHits(d->crate, d->slot, chan);
          data = evdata.GetData(d->crate, d->slot, chan, fRNhits[k] - 1);
        } else {
          fLNhits[k] = evdata.GetNumHits(d->crate, d->slot, chan);
          data = evdata.GetData(d->crate, d->slot, chan, fLNhits[k] - 1);
        }

      }

      if( adc ) {
        if( fFADC != NULL ) {
          if( jj == 1 ) {
            floverflow[k] = fFADC->GetOverflowBit(chan, 0);
            flunderflow[k] = fFADC->GetUnderflowBit(chan, 0);
            flpedq[k] = fFADC->GetPedestalQuality(chan, 0);
            fLPeak[k] = static_cast<Double_t>(fpeak);
            fLT_FADC[k] = static_cast<Double_t>(ftime);
            fLT_FADC_c[k] = fLT_FADC[k] * 0.0625;
          } else {
            froverflow[k] = fFADC->GetOverflowBit(chan, 0);
            frunderflow[k] = fFADC->GetUnderflowBit(chan, 0);
            frpedq[k] = fFADC->GetPedestalQuality(chan, 0);
            fRPeak[k] = static_cast<Double_t>(fpeak);
            fRT_FADC[k] = static_cast<Double_t>(ftime);
            fRT_FADC_c[k] = fRT_FADC[k] * 0.0625;
          }
        }

        if((jj == 1 && flpedq[k] == 0) || (jj == 0 && frpedq[k] == 0)) {
          if( fFADCConfig.tflag == 1 ) {
//            dest->ped[k] =
//              (fFADCConfig.nsa + fFADCConfig.nsb) * (static_cast<Double_t>(evdata.GetData(kPulsePedestal, d->crate, d->slot, chan, 0))) /
//              fFADCConfig.nped;
          } else {
//            dest->ped[k] =
//              fFADCConfig.win * (static_cast<Double_t>(evdata.GetData(kPulsePedestal, d->crate, d->slot, chan, 0))) / fFADCConfig.nped;
          }
        }
      }

      if( adc ) {
//        dest->adc[k] = static_cast<Double_t>( data );
//        dest->adc_p[k] = data - dest->ped[k];
//        dest->adc_c[k] = dest->adc_p[k] * dest->gain[k];
//        (*dest->nahit)++;
      } else {
//        dest->tdc[k] = static_cast<Double_t>( data );
//        dest->tdc_c[k] = (data - dest->offset[k]) * fTdc2T;
//        (*dest->nthit)++;
      }
    }
  }

  return fNHits[kRight].tdc + fNHits[kLeft].tdc;;
}

} // namespace HallA

//_____________________________________________________________________________
ClassImp(HallA::FadcScintillator)

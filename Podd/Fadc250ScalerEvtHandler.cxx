///////////////////////////////////////////////////////////////////////////////
//
//   Fadc250ScalerEvtHandler
//
//   Event handler for FADC250 scalers in bank 9250
//
//   Ole Hansen, Apr 2026
//   Adapted from Hanjie's MollerPolScalerEvtHandler, which in turn
//   is derived from Bob Michaels's THaScalerEvtHandler.
//
//   To use in the analyzer, your setup script needs something like this
//
//   gHaEvtHandlers->Add(
//      new FADC250ScalerEvtHandler("F250","FADC250 scaler event handler"));
//
//   TODO this class is a kludge and will disappear in the no-too-far future
//   TODO in favor of a more general solution
//
///////////////////////////////////////////////////////////////////////////////

#include "Fadc250ScalerEvtHandler.h"
#include "CodaDecoder.h"        // for CodaDecoder, THaEvData
#include "Database.h"           // for OpenDBFile, VarType
#include "GenScaler.h"          // for GenScaler
#include "Helper.h"             // for DeleteContainer
#include "Scaler9250.h"         // for Scaler9250
#include "THaAnalysisObject.h"  // for THaAnalysisObject enums
#include "THaGlobals.h"         // for gHaVars
#include "THaString.h"          // for CmpNoCase
#include "THaVarList.h"         // for THaVarList
#include "TString.h"            // for TString, Rtypes, operator+, kMaxUInt, ClassImp, operator<<
#include "TTree.h"              // for TTree
#include "TROOT.h"
#include "Textvars.h"           // for vsplit
#include <cassert>              // for assert
#include <cctype>               // for isspace
#include <cstdio>               // for size_t, fgets, sscanf
#include <cstdlib>              // for atoi
#include <cstring>              // for memset
#include <iostream>             // for ostream, operator<<, cout, endl
#include <string>               // for string, char_traits, string

using namespace std;
using namespace Decoder;
using THaString::CmpNoCase;
using Podd::vsplit;

namespace {
constexpr UInt_t MAXCHAN   = 32;
constexpr UInt_t defaultDT = 4;
}

//_____________________________________________________________________________
struct FadcScalerLoc { // Utility class used by Fadc250ScalerEvtHandler
  FadcScalerLoc( TString nm, TString desc, UInt_t idx, UInt_t sl,
             UInt_t ich, Int_t iki )
    : name(std::move(nm))
    , description(std::move(desc))
    , index(idx)
    , islot(sl)
    , ichan(ich)
    , ikind(iki)
  {}
  TString name, description;
  UInt_t index, islot, ichan, ikind;
};

//_____________________________________________________________________________
Fadc250ScalerEvtHandler::Fadc250ScalerEvtHandler(const char *name,
                                                 const char* description)
  : THaEvtTypeHandler(name, description)
  , fEvCount(0)
  , fEvNum(0)
  , fNormIdx(kMaxUInt)
  , fNormSlot(kMaxUInt)
  , dvars(nullptr)
  , fScalerTree(nullptr)
{}

//_____________________________________________________________________________
Fadc250ScalerEvtHandler::~Fadc250ScalerEvtHandler()
{
  Podd::DeleteContainer(scalerloc);
  Podd::DeleteContainer(scalers);
  delete [] dvars;
  if (!TROOT::Initialized())
    delete fScalerTree;
}

//_____________________________________________________________________________
Int_t Fadc250ScalerEvtHandler::End( THaRunBase* )
{
  if( fScalerTree )
    fScalerTree->Write();
  return 0;
}

//_____________________________________________________________________________
Int_t Fadc250ScalerEvtHandler::Analyze(THaEvData *evdata)
{
  const char* const here = "Analyze";

  // if( !IsMyEvent(evdata->GetEvType()) )
  if( !evdata->IsPhysicsTrigger() )
    return -1;

  if( !fScalerTree ) {
    TString sname1 = "TS";
    TString sname2 = sname1 + fName;
    TString sname3 = fName + "  Scaler Data";

    fScalerTree = new TTree(sname2,sname3);
    fScalerTree->SetAutoSave(200000000);

    TString name = "evnum";
    TString tinfo = name + "/l";
    fScalerTree->Branch(name, &fEvNum, tinfo, 4000);

    name = "evcount";
    tinfo = name + "/l";
    fScalerTree->Branch(name, &fEvCount, tinfo, 4000);

    for( size_t i = 0; i < scalerloc.size(); i++) {
      name = scalerloc[i]->name;
      tinfo = name + "/D";
      fScalerTree->Branch(name, &dvars[i], tinfo, 4000);
    }
  }  // if (!fScalerTree)


  auto* codaevent = dynamic_cast<CodaDecoder*>(evdata);
  if( !codaevent ){
    Error(here, "Decoder is not a CodaDecoder. Cannot proceed.");
    return -1;
  }
  auto bank = codaevent->FindBank(icrate , imodel);
  if( bank.pos == 0 ) {
    // Bank not found. This is expected for some/many events.
    return 0;
  }

  // Now bank.pos and bank.len contain the position and length of
  // the bank data in evbuffer. Decode the bank header:
  auto data = codaevent->GetBank(bank);
  if( !data ) {
    // Error decoding bank
    Error(here, "Cannot decode bank %d of crate %u", imodel, icrate);
    return -1;
  }
  if( data.GetDataType() != CodaDecoder::BankInfo::kUInt ) {
    Error(here, "Data type of bank %d of crate %u is not uint32_t",
          imodel, icrate );
    return -1;
  }

  // Pointer to start of bank data buffer (length = data.len_)
  Bool_t ifound = false;
  for( auto& scaler: scalers ) {
    scaler->LoadSlot(nullptr, evdata->GetRawDataBuffer(),
                     data.pos_, data.len_ );
    if( scaler->IsDecoded() )
      ifound = true;
  }

  if( !ifound )
    return 0;

  // The correspondence between dvars and the scaler and the channel
  // will be driven by a scaler.map file, or could be hard-coded.

  for( size_t i = 0; i < scalerloc.size(); i++ ) {
    UInt_t idx = scalerloc[i]->index;
    UInt_t ichan = scalerloc[i]->ichan;
    if( idx < scalers.size() && ichan < MAXCHAN ) {
      if( scalerloc[i]->ikind == ICOUNT )
        dvars[i] = scalers[idx]->GetData(ichan);
      if( scalerloc[i]->ikind == IRATE )
        dvars[i] = scalers[idx]->GetRate(ichan);
    } else {
      cout << "Fadc250ScalerEvtHandler:: ERROR:: incorrect index " << i
           << "  " << idx << "  " << ichan << endl;
    }
  }

  fEvCount++;
  fEvNum = evdata->GetEvNum();

  for( auto* s: scalers )
    s->Clear();

  if( fScalerTree )
    fScalerTree->Fill();

  return 1;
}

//_____________________________________________________________________________
// Helper functions for Init()
void Fadc250ScalerEvtHandler::ParseVariable( const vector<string>& dbline )
{
  string sdesc;
  for( size_t j = 5; j < dbline.size(); j++ ) {
    sdesc += " ";
    sdesc += dbline[j];
  }
  Int_t islot = atoi(dbline[1].c_str());
  Int_t ichan = atoi(dbline[2].c_str());
  Int_t ikind = atoi(dbline[3].c_str());
  TString tsname(dbline[4].c_str());
  TString tsdesc(sdesc.c_str());
  AddVars(tsname, tsdesc, islot, ichan, ikind);
}

//_____________________________________________________________________________
void Fadc250ScalerEvtHandler::ParseMap( const char* cbuf, const vector<string>& dbline )
{
  using ssiz_t = decltype(scalers)::size_type;
  //Int_t imodel = 0;
  //UInt_t icrate = 0;
  UInt_t islot = 0, inorm = 0, header = 0, mask = 0;
  char cdum[21];
  sscanf(cbuf, "%20s %d %u %u %x %x %u \n",
         cdum, &imodel, &icrate, &islot, &header, &mask, &inorm);
  if( fNormSlot != inorm )
    cout << "Fadc250ScalerEvtHandler:: WARN: contradictory norm slot " << inorm << endl;
  fNormSlot = inorm;  // slot number used for normalization.  This variable is not used but is checked.
  UInt_t clkchan = kMaxUInt;
  Double_t clkfreq = 1;
  if( dbline.size() > 8 ) {
    clkchan = atoi(dbline[7].c_str());
    clkfreq = static_cast<Double_t>( atoi(dbline[8].c_str()) );
  }
  switch( imodel ) {
    case 9250:
      scalers.push_back(new Scaler9250(icrate, islot));
    default:
      break;
  }
  if( !scalers.empty() ) {
    ssiz_t idx = scalers.size() - 1;
    scalers[idx]->SetHeader(header, mask);
// The normalization slot has the clock in it, so we automatically recognize it.
// fNormIdx is the index in scaler[] and
// fNormSlot is the slot#, checked for consistency
    if( clkchan != kMaxUInt ) {
      scalers[idx]->SetClock(defaultDT, clkchan, clkfreq);
      fNormIdx = idx;
      if( islot != fNormSlot )
        cout << "Fadc250ScalerEvtHandler:: WARN: contradictory norm slot ! "
             << islot << endl;
    }
  }
}

//_____________________________________________________________________________
void Fadc250ScalerEvtHandler::AssignNormScaler()
{
  if( fNormIdx < scalers.size() ) {
    UInt_t i = 0;
    for( auto* scaler: scalers ) {
      if( i++ == fNormIdx )
        continue;
      scaler->LoadNormScaler(scalers[fNormIdx]);
    }
  }
}

//_____________________________________________________________________________
void Fadc250ScalerEvtHandler::VerifySlots()
{
  for( size_t i1 = 0; i1 < scalers.size() - 1; i1++ ) {
    for( size_t i2 = i1 + 1; i2 < scalers.size(); i2++ ) {
      if( scalers[i1]->GetSlot() == scalers[i2]->GetSlot() )
        cout << "Fadc250ScalerEvtHandler:: WARN:  same slot defined twice" << endl;
    }
  }
}

//_____________________________________________________________________________
void Fadc250ScalerEvtHandler::SetIndices()
{
  for( size_t i = 0; i < scalers.size(); i++ ) {
    for( auto& loc: scalerloc ) {
      if( loc->islot == scalers[i]->GetSlot() )
        loc->index = i;
    }
  }
}

//_____________________________________________________________________________
Int_t Fadc250ScalerEvtHandler::ReadDatabase( const TDatime& date )
{
  // Parse the map file which defines what scalers exist and the global variables.
  FILE* file = OpenFile(date);
  if( !file )
    return kFileError;

  const char comment = '#';
  const string svariable = "variable";
  const string smap = "map";
  const int LEN = 256;
  char cbuf[LEN];

  while( fgets(cbuf, LEN, file) ) {
    const vector<string> dblines = vsplit(cbuf);
    if( dblines.empty() )
      continue;
    assert(!dblines.front().empty() && !isspace(dblines.front()[0])); // else bug in vsplit
    if( dblines.front()[0] == comment )
      continue;

    if( dblines.size() > 4 && CmpNoCase(dblines.front(), svariable) == 0 ) {
      ParseVariable(dblines);
    }
    else if( dblines.size() > 6 && CmpNoCase(dblines.front(), smap) == 0 ) {
      ParseMap(cbuf, dblines);
    }
  }
  (void) fclose(file);
  return kOK;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus Fadc250ScalerEvtHandler::Init(const TDatime& date)
{
  fStatus = THaEvtTypeHandler::Init(date);
  if( fStatus != kOK )
    return fStatus;

  fNormIdx = -1;

  //AddEvtType(140);  // what events to look for


  // need to do LoadNormScaler after scalers created and if fNormIdx found.
  AssignNormScaler();

  DefVars();

  // Verify that the slots are not defined twice
  VerifySlots();

  // Identify indices of scalers[] vector to variables.
  SetIndices();

  return kOK;
}

//_____________________________________________________________________________
void Fadc250ScalerEvtHandler::AddVars( const TString& name, const TString& desc,
                                       UInt_t iscal, UInt_t ichan, UInt_t ikind )
{
  // need to add fName here to make it a unique variable.  (Left vs Right HRS, for example)
  // We don't yet know the correspondence between index of scalers[] and slots.
  // Will put that in later.
  scalerloc.push_back(new FadcScalerLoc(GetPrefix() + name, fName + desc,
                                        0, iscal, ichan, ikind));
}

//_____________________________________________________________________________
void Fadc250ScalerEvtHandler::DefVars()
{
  // called after AddVars has finished being called.
  size_t Nvars = scalerloc.size();
  if( Nvars == 0 )
    return;
  delete [] dvars;
  dvars = new Double_t[Nvars];  // dvars is a member of this class
  memset(dvars, 0, Nvars * sizeof(Double_t));
  if( !gHaVars ) {
    cout << "No gHaVars ?!  Well, that's a problem !!" << endl;
    return;
  }
  const Int_t* count = nullptr;
  for( size_t i = 0; i < scalerloc.size(); i++ ) {
    gHaVars->DefineByType(scalerloc[i]->name.Data(),
                          scalerloc[i]->description.Data(),
                          &dvars[i], kDouble, count);
  }
}

//_____________________________________________________________________________
#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(Fadc250ScalerEvtHandler)
#endif

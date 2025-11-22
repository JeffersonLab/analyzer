//*-- Author :    Ole Hansen   28/08/22

//////////////////////////////////////////////////////////////////////////
//
// DAQconfig, DAQInfoExtra
//
// Helper classes to support DAQ configuration info.
//
// Used in CodaDecoder and THaRunBase.
// Filled in CodaDecoder::daqConfigDecode.
//
//////////////////////////////////////////////////////////////////////////

#include "DAQInfoExtra.h"
#include "THaRun.h"
#include "THaRunParameters.h"
#include "TVirtualObject.h"
#include "TList.h"
#include <utility>
#include <iostream>

using namespace std;

const Int_t kMinScan = 50;

//_____________________________________________________________________________
DAQInfoExtra::DAQInfoExtra()
  : fMinScan(kMinScan)
{}

//_____________________________________________________________________________
namespace {
DAQInfoExtra* GetExtraInfo( TObject* p )
{
  auto* ifo = dynamic_cast<DAQInfoExtra*>(p);
  if( !ifo ) {
    auto* lst = dynamic_cast<TList*>(p);
    if( lst ) {
      for( auto* obj: *lst ) {
        ifo = dynamic_cast<DAQInfoExtra*>(obj);
        if( ifo )
          break;
      }
    }
  }
  return ifo;
}}

//_____________________________________________________________________________
THaRunParameters* DAQInfoExtra::UpdateRunParam( THaRunBase* run )
{
  if(!run) {
    cerr << "no run" << endl;
    return nullptr;
  }
  if( !run->fExtra ) {
    cerr << "no fExtra" << endl;
    return nullptr;
  }
  auto* ifo = GetExtraInfo(run->fExtra);
  if( !ifo ) {
    cerr << "No ifo" << endl;
    return nullptr;
  }
  auto* rp = run->GetParameters();
  if( !rp ) {
    cerr << "No rp" << endl;
    return nullptr;
  }
  //rp->Print();
  //cerr << "nstrings = " << ifo->fDAQconfig.strings.size() << endl;
  //int i = 0;
  for( size_t i = 0; auto& text : ifo->fDAQconfig.strings ) {
    //cerr << "size(" << i++ << ") = " << text.size() << endl;
    UInt_t crate = (i >= ifo->fTags.size()) ? 0 : ifo->fTags[i];
    // Event number and event type are not available in previous class versions.
    // crate (roc) is available starting with analyzer 1.7.14
    rp->AddDAQConfig({0, 0, crate, std::move(text)});  // Parses text
    ++i;
  }
  ifo->fDAQconfig.strings.clear();
  ifo->fDAQconfig.keyval.clear();
  return rp;
}

//_____________________________________________________________________________
unsigned int DAQInfoExtra::GetMinScan( THaRun* run )
{
  if(!run) {
    cerr << "Minscan: no p" << endl;
    return kMinScan;
  }
  if( !run->fExtra ) {
    cerr << "Minscan: no fExtra" << endl;
    return kMinScan;
  }
  auto* ifo = GetExtraInfo(run->fExtra);
  if( !ifo ) {
    cerr << "Minscan: No ifo" << endl;
    return kMinScan;
  }
  //cerr << "Minscan = " << ifo->fMinScan << endl;
  return ifo->fMinScan;
}

//_____________________________________________________________________________
void DAQInfoExtra::DeleteExtra( THaRunBase* run )
{
  if(!run) {
    cerr << "Del: no p" << endl;
    return;
  }
  delete run->fExtra;
  run->fExtra = nullptr;
}

//_____________________________________________________________________________
ClassImp(DAQInfoExtra)
ClassImp(DAQconfig)

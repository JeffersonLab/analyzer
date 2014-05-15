////////////////////////////////////////////////////////////////////
//
//   ToyPhysicsEvtHandler
//
/////////////////////////////////////////////////////////////////////

#include "ToyEvtTypeHandler.h"
#include "ToyPhysicsEvtHandler.h"
#include "ToyModule.h"
#include "THaEvData.h"
#include "THaCrateMap.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyPhysicsEvtHandler::ToyPhysicsEvtHandler() { 
}

ToyPhysicsEvtHandler::~ToyPhysicsEvtHandler() { 
}

Int_t ToyPhysicsEvtHandler::Decode(THaEvData *evdata) {

  assert( evbuffer && fMap );
#ifdef FIXME
  if( fDoBench ) fBench->Begin("physics_decode");
#endif
  Int_t status = HED_OK;

  if( (evdata->GetRawData(1)&0xffff) != 0x10cc ) std::cout<<"Warning, header error"<<std::endl;
  if( (evdata->GetRawData(1)>>16) > MAX_PHYS_EVTYPE ) std::cout<<"Warning, Event type makes no sense"<<std::endl;
  memset(rocdat,0,MAXROC*sizeof(RocDat_t));
  // Set pos to start of first ROC data bank
  Int_t pos = evdata->GetRawData(2)+3;  // should be 7
  Int_t nroc = 0;
  Int_t irn[MAXROC];   // Lookup table i-th ROC found -> ROC number
  while( pos+1 < evdata->GetRawData(0)+1 && nroc < MAXROC ) {
    Int_t len  = evdata->GetRawData(pos);
    Int_t iroc = (evdata->GetRawData(pos+1)&0xff0000)>>16;
    if( iroc>=MAXROC ) {
#ifdef FIXME
      if(TestBit(kVerbose)) { 
	cout << "ERROR in EvtTypeHandler::FindRocs "<<endl;
	cout << "  illegal ROC number " <<dec<<iroc<<endl;
      }
      if( fDoBench ) fBench->Stop("physics_decode");
#endif
      return HED_ERR;
    }
    // Save position and length of each found ROC data block
    rocdat[iroc].pos  = pos;
    rocdat[iroc].len  = len;
    irn[nroc++] = iroc;
    pos += len+1;
  }

  // Decode each ROC
  // This is not part of the loop above because it may exit prematurely due 
  // to errors, which would leave the rocdat[] array incomplete.
  for( Int_t i=0; i<nroc; i++ ) {
    Int_t iroc = irn[i];
    const RocDat_t* proc = rocdat+iroc;
    Int_t ipt = proc->pos + 1;
    Int_t iptmax = proc->pos + proc->len;
    while (ipt++ < iptmax) {
      //      evdata|>FillCrateSlot(iroc,ipt);
    }
  }

  return 1;

}

Int_t ToyPhysicsEvtHandler::FindRocs(const Int_t *evbuffer) {
  
  assert( evbuffer && fMap );
#ifdef FIXME
  if( fDoBench ) fBench->Begin("physics_decode");
#endif
  Int_t status = HED_OK;

  if( (evbuffer[1]&0xffff) != 0x10cc ) std::cout<<"Warning, header error"<<std::endl;
  if( (evbuffer[1]>>16) > MAX_PHYS_EVTYPE ) std::cout<<"Warning, Event type makes no sense"<<std::endl;
  memset(rocdat,0,MAXROC*sizeof(RocDat_t));
  // Set pos to start of first ROC data bank
  Int_t pos = evbuffer[2]+3;  // should be 7
  Int_t nroc = 0;
  Int_t irn[MAXROC];   // Lookup table i-th ROC found -> ROC number
  while( pos+1 < evbuffer[0]+1 && nroc < MAXROC ) {
    Int_t len  = evbuffer[pos];
    Int_t iroc = (evbuffer[pos+1]&0xff0000)>>16;
    if( iroc>=MAXROC ) {
#ifdef FIXME
      if(TestBit(kVerbose)) { 
	cout << "ERROR in EvtTypeHandler::FindRocs "<<endl;
	cout << "  illegal ROC number " <<dec<<iroc<<endl;
      }
      if( fDoBench ) fBench->Stop("physics_decode");
#endif
      return HED_ERR;
    }
    // Save position and length of each found ROC data block
    rocdat[iroc].pos  = pos;
    rocdat[iroc].len  = len;
    irn[nroc++] = iroc;
    pos += len+1;
  }

  return 1;

}


Int_t ToyPhysicsEvtHandler::Init(THaCrateMap *map) {
  fMap = map;
  return 1;  
}


ClassImp(ToyPhysicsEvtHandler)

////////////////////////////////////////////////////////////////////
//
//   ToyEvtTypeHandler
//   handles events of a particular type
//
/////////////////////////////////////////////////////////////////////

#include "ToyEvtTypeHandler.h"
#include "THaEvData.h"
#include "THaCrateMap.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyEvtTypeHandler::ToyEvtTypeHandler() { 
  fMap=0;
}

ToyEvtTypeHandler::~ToyEvtTypeHandler() { 
}

Int_t ToyEvtTypeHandler::Decode(THaEvData *evdata) {
  return 1;
}

Int_t ToyEvtTypeHandler::Init(THaCrateMap *map) {
  *fMap = *map;   // need to define copy C'tor
  return 1;
}

Int_t ToyEvtTypeHandler::FindRocs(const Int_t *evbuffer) {
  
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

ClassImp(ToyEvtTypeHandler)

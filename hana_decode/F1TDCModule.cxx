/////////////////////////////////////////////////////////////////////
//
//   F1TDCModule
//   JLab F1 TDC Module
//
/////////////////////////////////////////////////////////////////////

#include "F1TDCModule.h"
#include "VmeModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>

using namespace std;

namespace Decoder {

  const Int_t NTDCCHAN = 32;
  const Int_t MAXHIT   = 100;

  Module::TypeIter_t F1TDCModule::fgThisType =
    DoRegister( ModuleType( "Decoder::F1TDCModule" , 3201 ));

F1TDCModule::F1TDCModule( UInt_t crate, UInt_t slot ) :
  VmeModule(crate, slot), fNumHits(0), fResol(ILO),
  fTdcData(NTDCCHAN*MAXHIT),
  IsInit(false), slotmask(0), chanmask(0), datamask(0)
{
  F1TDCModule::Init();
}

void F1TDCModule::Init() {
  VmeModule::Init();
  fTdcData.resize(NTDCCHAN*MAXHIT);
  Clear();
  IsInit = true;
  fName = "F1 TDC 3201";
  fNumChan = 32;
  fWdcntMask=0;
  SetResolution(1);
  if (fModelNum == 6401) SetResolution(0);
}


Bool_t F1TDCModule::IsSlot(UInt_t rdata)
{
  if (fDebugFile)
    *fDebugFile << "is F1TDC slot ? "<<hex<<fHeader
                <<"  "<<fHeaderMask<<"  "<<rdata<<dec<<endl;
  return ((rdata != 0xffffffff) & ((rdata & fHeaderMask)==fHeader));
}

UInt_t F1TDCModule::GetData( UInt_t chan, UInt_t hit ) const
{
  UInt_t idx = chan*MAXHIT + hit;
  if( idx > MAXHIT * NTDCCHAN ) return 0;
  return fTdcData[idx];
}

void F1TDCModule::Clear(Option_t* opt) {
  VmeModule::Clear(opt);
  fNumHits = 0;
  fTdcData.assign(NTDCCHAN*MAXHIT,0);
}

UInt_t F1TDCModule::LoadSlot( THaSlotData *sldat, const UInt_t *evbuffer,
                              const UInt_t *pstop ) {
// this increments evbuffer
  if (fDebugFile) *fDebugFile << "F1TDCModule:: loadslot "<<endl;
  fWordsSeen = 0;

  // CAUTION: this routine re-numbers the channels
  // compared to the labelled numbering scheme on the board itself.
  // According to the labelling and internal numbering scheme,
  // the F1 module has odd numbered channels on one connector
  // and even numbered channels on the other.
  // However we usually put neighboring blocks/wires into the same
  // cable, connector etc.
  // => hana therefore uses a numbering scheme different from the module
  //
  // In normal resolution mode, the scheme is:
  //    connector 1:  ch 0 - 15
  //    connector 2:  ch 16 - 31
  //    connector 33: ch 32 - 47
  //    connector 34: ch 48 - 63
  //
  // In high-resolution mode, only two connectors are used since
  // two adjacent channels are internally combined and read out as the
  // internally-even numbered channels.
  // this is kind of inconvenient for the rest of the software
  // => hana therefore uses a numbering scheme different from the module
  //    connector 1:  unused
  //    connector 2:  ch 0 - 15
  //    connector 33: unused
  //    connector 34: ch 16 - 31
  //
  // In both modes:
  // it is assumed that we only get data from one single trigger
  // if the F1 is run in multiblock mode (buffered mode)
  // this might not be the case anymore - but this will be interesting anyhow
  // triggertime and eventnumber are not yet read out, they will again
  // be useful when multiblock mode (buffered mode) is used

   const UInt_t F1_HIT_OFLW = BIT(24); // bad
   const UInt_t F1_OUT_OFLW = BIT(25); // bad
   const UInt_t F1_RES_LOCK = BIT(26); // good
   const UInt_t DATA_CHK = F1_HIT_OFLW | F1_OUT_OFLW | F1_RES_LOCK;
   const UInt_t DATA_MARKER = BIT(23);
   // look at all the data
   const UInt_t *loc = evbuffer;
#ifdef WITH_DEBUG
   if(fDebug > 1 && fDebugFile)
     *fDebugFile<< "Debug of F1TDC data, fResol =  "<<fResol<<"  model num  "<<fModelNum<<endl;
#endif
   while ( loc <= pstop && IsSlot(*loc) ) {
     if ( !( (*loc) & DATA_MARKER ) ) {
       // header/trailer word, to be ignored
#ifdef WITH_DEBUG
       if(fDebug > 1 && fDebugFile)
         *fDebugFile<< "[" << (loc-evbuffer) << "] header/trailer  0x"
         <<hex<<*loc<<dec<<endl;
#endif
     } else {
       UInt_t chan = ((*loc) >> 16) & 0x3f;  // internal channel number
#ifdef WITH_DEBUG
       UInt_t chn = chan; // save original for debug message below
       if (fDebug > 1 && fDebugFile)
         *fDebugFile<< "[" << (loc-evbuffer) << "] data            0x"
         <<hex<<*loc<<dec<<endl;
#endif
       if( IsHiResolution() ) {
         // drop last bit for channel renumbering
         chan >>= 1;
       } else {
         // do the reordering of the channels, for contiguous groups
         // odd numbered TDC channels from the board -> +16
         chan = (chan & 0x20) + ((chan & 0x01) << 4) + ((chan & 0x1e) >> 1);
       }
       //FIXME: cross-check slot number here
       if( ((*loc) & DATA_CHK) != F1_RES_LOCK ) {
         UInt_t f1slot = ((*loc) & 0xf8000000) >> 27;
         cout << "\tWarning: F1 TDC " << hex << (*loc) << dec;
         cout << "\tSlot (Ch) = " << f1slot << "(" << chan << ")";
         if( (*loc) & F1_HIT_OFLW ) {
           cout << "\tHit-FIFO overflow";
         }
         if( (*loc) & F1_OUT_OFLW ) {
           cout << "\tOutput FIFO overflow";
         }
         if( !((*loc) & F1_RES_LOCK) ) {
           cout << "\tResolution lock failure!";
         }
         cout << endl;
       }

       UInt_t raw = (*loc) & 0xffff;
#ifdef WITH_DEBUG
       if(fDebug > 1 && fDebugFile) {
         *fDebugFile<<" int_chn chan data "<<dec<<chn<<"  "<<chan
             <<"  0x"<<hex<<raw<<dec<<endl;
       }
#endif
       /*Int_t status = */sldat->loadData("tdc",chan,raw,raw);
       UInt_t idx = chan * MAXHIT + 0;  // 1 hit per chan ???
       if( idx < MAXHIT * NTDCCHAN ) fTdcData[idx] = raw;
       fWordsSeen++;
     }
     loc++;
   }

  return fWordsSeen;
}

}

ClassImp(Decoder::F1TDCModule)

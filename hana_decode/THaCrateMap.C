/////////////////////////////////////////////////////////////////////
//
//  THaCrateMap
//  Layout, or "map", of DAQ Crates.
//
//  THaCrateMap contains info on how the DAQ crates
//  are arranged in Hall A, i.e whether slots are
//  fastbus or vme, what the module types are, and
//  what header info to expect.  Probably nobody needs
//  to know about this except the author, and at present
//  an object of this class is a private member of the decoder.
//
//  author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////


#include "THaCrateMap.h"
#include <iostream>
#include <ctime>

using namespace std;

const int THaCrateMap::CM_OK = 1;
const int THaCrateMap::CM_ERR = -1;

int THaCrateMap::getScalerCrate(int data) const {
  for (int crate=0; crate<MAXROC; crate++) {
    if (!crate_used[crate]) continue;
    if (isScalerCrate(crate)) {
      int headtry = data&0xfff00000;
      int zero = data&0x0000ff00;          
      if ((zero == 0) && 
	  (headtry == header[crate][1])) return crate;
    }
  }
  return 0;
}

int THaCrateMap::setCrateType(int crate, const char* ctype) {
  if ( crate < 0 || crate >= MAXROC ) 
    return CM_ERR;
  TString type(ctype);
  crate_used[crate] = true;
  crate_type[crate] = type;
  if (type == "fastbus") 
    crate_code[crate] = kFastbus;
  else if (type == "vme")
    crate_code[crate] = kVME;
  else if (type == "scaler")
    crate_code[crate] = kScaler;
  else if (type == "camac")
    crate_code[crate] = kCamac;
  else {
    crate_used[crate] = false;
    crate_type[crate] = "unknown";
    crate_code[crate] = kUnknown;
    return CM_ERR;
  }
  return CM_OK;
}

int THaCrateMap::setModel(int crate, int slot, UShort_t mod,
			  UShort_t nc, UShort_t nd ) {
  if ( crate < 0 || crate >= MAXROC ) 
    return CM_ERR;
  incrNslot(crate);
  if ( slot < 0 || slot >= MAXSLOT ) 
    return CM_ERR;
  setUsed(crate,slot);
  model[crate][slot] = mod;
  if( !SetModelSize( crate, slot, mod )) {
    nchan[crate][slot] = nc;
    ndata[crate][slot] = nd;
  }
  return CM_OK;
}

int THaCrateMap::SetModelSize( int crate, int slot, UShort_t model ) 
{
  // Set the max number of channels and data words for some known modules
  struct ModelPar_t { UShort_t model, nchan, ndata; };
  static const ModelPar_t modelpar[] = {
    { 1875, 64, 512 },  // Detector TDC
    { 1877, 96, 672 },  // Wire-chamber TDC
    { 1881, 64, 64 },   // Detector ADC
    { 550, 512, 1024 }, // CAEN 550 (RICH)
    { 560,  16, 16 },   // CAEN 560 scaler
    { 1182, 8, 128 },   // LeCroy 1182 ADC (?)
    { 1151, 16, 16 },   // LeCroy 1151 scaler
    { 3123, 16, 16 },   // VMIC 3123 ADC
    { 3800, 32, 32 },   // Struck 3800 scaler
    { 3801, 32, 32 },   // Struck 3801 scaler
    { 7510, 8, 1024 },   // Struck 7510 ADC (multihit)
    { 0 }
  };
  const ModelPar_t* item = modelpar;
  while( item->model ) {
    if( model == item->model ) {
      nchan[crate][slot] = item->nchan;
      ndata[crate][slot] = item->ndata;
      return 1;
    }
    item++;
  }
  return 0;
}

int THaCrateMap::setHeader(int crate, int slot, int head) {
  if ( crate < 0 || crate >= MAXROC ) 
    return CM_ERR;
  incrNslot(crate);
  if ( slot < 0 || slot >= MAXSLOT ) 
    return CM_ERR;
  setUsed(crate,slot);
  header[crate][slot] = head;
  return CM_OK;
}

int THaCrateMap::setMask(int crate, int slot, int mask) {
  if ( crate < 0 || crate >= MAXROC ) 
    return CM_ERR;
  incrNslot(crate);
  if ( slot < 0 || slot >= MAXSLOT ) 
    return CM_ERR;
  setUsed(crate,slot);
  headmask[crate][slot] = mask;
  return CM_OK;
}

int THaCrateMap::setScalerLoc(int crate, const char* loc) {
  if ( crate < 0 || crate >= MAXROC ) 
    return CM_ERR;
  incrNslot(crate); 
  setCrateType(crate,"scaler");
  scalerloc[crate] = loc;
  return CM_OK;
}

void THaCrateMap::incrNslot(int crate) {
  nslot[crate] = 0;
  for (int slot=0; slot<MAXSLOT; slot++) {
    if (model[crate][slot] != 0)
      nslot[crate]++;
  }
}

// Initialization of the default crate map.  R. Michaels
// I think I know where all crates and models are, so here I set it up.
// Ideally this method would use a database, but for now we use a 
// crude mechanism to determine time dependence.  
// time_period determined from the Unix time passed as argument.
// The Unix time is obtained from Prestart event by the user class.

   int THaCrateMap::init(UInt_t tloc) {
     int crate, slot, month=0, year=0;
// Default state: most recent.
     int prior_oct_2000 = 0;
     int prior_jan_2001 = 0;
     int after_may_2001 = 0;   
     int after_sep_2002 = 1;  // most recent

     if (tloc == 0) {
//       cout << "Initializing crate map for the time='now'."<<endl;
     } else {
       time_t t = tloc;
       struct tm* tp = localtime(&t);
       year = 1900 + tp->tm_year;
       month = tp->tm_mon+1;
       if (year < 2000 || (year < 2001 && month < 10)) 
                prior_oct_2000 = 1; 
       if (year < 2001) prior_jan_2001 = 1;  
       if (year >= 2001 && month > 5) after_may_2001 = 1; 
       if (year < 2002 || (year == 2002 && month < 9)) 
                after_sep_2002 = 0;
     } 
     if (prior_oct_2000) prior_jan_2001 = 1;
     for(crate=0; crate<MAXROC; crate++) {
        nslot[crate] = 0;
        crate_used[crate] = false;
        crate_type[crate] = "unknown";
        for(slot=0; slot<MAXSLOT; slot++) {
           slot_used[crate][slot] = false;
           model[crate][slot] = 0;
           header[crate][slot] = 0;
           slot_clear[crate][slot] = true;
        }
     }
// ROC1
     setCrateType(1,"fastbus");
     if ( after_sep_2002 ) {
       for(slot=6; slot<=15; slot++) setModel(1,slot,1877);  
       for(slot=23; slot<=25; slot++) setModel(1,slot,1881); 
       setModel(1,16,1875);
       setModel(1,17,1875);
     } else {
       for(slot=3; slot<=19; slot++) setModel(1,slot,1877);  
       for(slot=22; slot<=25; slot++) setModel(1,slot,1881); 
       setModel(1,20,1875);
       setModel(1,21,1875);
     }
// ROC2
     setCrateType(2,"fastbus");
     if ( after_sep_2002 ) {
        for(slot=3; slot<=12; slot++) setModel(2,slot,1877);
        setModel(2,22,1881);
        setModel(2,23,1881);
     } else {
       if ( prior_jan_2001 ) {
          for(slot=6; slot<=22; slot++) setModel(2,slot,1877);
          setModel(2,23,1875);
          setModel(2,24,1881);
          setModel(2,25,1881);
       } else {
          setModel(2,3,1875);
          for(slot=4; slot<=21; slot++) setModel(2,slot,1877);
          for(slot=22; slot<=25; slot++) setModel(2,slot,1881);
       }
     }
// ROC3
     setCrateType(3,"fastbus");
     if ( after_sep_2002 ) {
        setModel(3,3,1875);
        for(slot=4; slot<=21; slot++) setModel(3,slot,1877); 
        for(slot=22; slot<=25; slot++) setModel(3,slot,1881); 
     } else {
        for(slot=4; slot<=10; slot++) setModel(3,slot,1877); 
        setModel(3,22,1881);
        setModel(3,23,1877);
     }
// ROC4
     setCrateType(4,"fastbus");
     if ( after_sep_2002 ) {
       for(slot= 4; slot<=10; slot++) setModel(4,slot,1877);
       setModel(4,22,1881);
       setModel(4,23,1877);
     } else {
       for(slot= 2; slot<= 7; slot++) setModel(4,slot,1881);
       for(slot= 8; slot<=11; slot++) setModel(4,slot,1877);
       for(slot=22; slot<=24; slot++) setModel(4,slot,1881);
     }
// Scalers on the "right" spectrometer, call it crate 7
     setCrateType(7,"scaler");
     if ( prior_oct_2000 ) {
       setScalerLoc(7,"lscaler");   
     } else { 
       setScalerLoc(7,"rscaler");   
     }
     setModel(7,0,3801); //was a 1151, but there are 32 chans in the data...
     for(slot=1; slot<5; slot++) setModel(7,slot,1151);
     if (prior_oct_2000) {
       for(slot=5; slot<6; slot++) setModel(7,slot,560);  
       for(slot=0; slot<6; slot++) setClear(7,slot,false);  
     } else {
       for(slot=5; slot<=6; slot++) setModel(7,slot,560);  
       setModel(7,7,3800);    
       for(slot=0; slot<=7; slot++) setClear(7,slot,false);  
     } 
     setHeader(7,1,0xceb00000);
     setMask(7,1,0xfff00000);
     if (after_may_2001) {  // this is a mess
       for(slot=8; slot<=9; slot++) {
	 setModel(7,slot,3800);
         setClear(7,slot,false);
         setHeader(7,slot,0xceb00000);
         setMask(7,slot,0xfff00000);
       }
     }
// Scalers on the "left" spectrometer, call it crate 8
     setCrateType(8,"scaler");
     if ( prior_oct_2000 ) {
       setScalerLoc(8,"rscaler");
     } else {
       setScalerLoc(8,"lscaler");
     }
     for(slot=0; slot<=2; slot++) setModel(8,slot,1151); 
     setModel(8,3,3801);
     setModel(8,4,3800);
     setModel(8,5,3801);
     setModel(8,6,1151);
     for(slot=7; slot<=8; slot++) setModel(8,slot,560);
     for(slot=0; slot<=8; slot++) setClear(8,slot,false);
     setHeader(8,1,0xabc00000);
     setMask(8,1,0xfff00000);
// crate 9, the RCS scalers
     setCrateType(9,"scaler");
     setScalerLoc(9,"rcs");
     for(slot=1; slot<=3; slot++) {
        setModel(9,slot,3801);
        setHeader(9,slot,0xbbc00000+(slot-1)*0x10000);
        setMask(9,slot,0xffff0000);
     }
// ROC10 synchronous event readout of scalers
     setCrateType(10,"scaler");
     setScalerLoc(10,"evright");
     setModel(10,1,3801);
     setHeader(10,1,0xceb70000);
     setMask(10,1,0xffff0000);
     setModel(10,2,3801);
     setHeader(10,2,0xceb80000);
     setMask(10,2,0xffff0000);
     setModel(10,3,3801);
     setHeader(10,3,0xceb90000);
     setMask(10,3,0xffff0000);
// ROC11 synchronous event readout of scalers
     setCrateType(11,"scaler");
     setScalerLoc(11,"evleft");
     setModel(11,1,3801);
     setHeader(11,1,0xabc30000);
     setMask(11,1,0xffff0000);
     setModel(11,2,3801);
     setHeader(11,2,0xabc40000);
     setMask(11,2,0xffff0000);
     setModel(11,3,3801);
     setHeader(11,3,0xabc50000);
     setMask(11,3,0xffff0000);
// ROC12
     setCrateType(12,"vme");
     for (slot = 1; slot <= 24; slot++) { // actually ADCs (2 per 12 slots)
       setModel(12,slot,550);
       setHeader(12,slot,0xcd000000+((slot-1)<<16));
       setMask(12,slot,0xffff0000);     
     }
     // This is a hack by BR, to read the tir_dat register of the
     // trigger modoule in this crate, this model ID is just 
     //  arbitary, maybe one can find something better
     //  there is also no physical slot 25 
     setModel(12,25,7353);
     setHeader(12,25,0x73530000);
     setMask(12,25,0xffff0000);
// ROC13
     setCrateType(13,"vme");
     setModel(13,1,7510);
     setHeader(13,1,0xf7510000);
     setMask(13,1,0xffff0000);
// ROC14
     setCrateType(14,"vme");
     setModel(14,1,1182);
     setHeader(14,1,0xfadc1182);
     setMask(14,1,0xffffffff);
     setModel(14,2,1182);
     setHeader(14,2,0xfadd1182);
     setMask(14,2,0xffffffff);
     setModel(14,3,3123);
     setHeader(14,3,0xfadc3123);
     setMask(14,3,0xffffffff);
     setModel(14,4,7510);
     setHeader(14,4,0xf7510000);
     setMask(14,4,0xfffff000);
     setModel(14,5,7510);
     setHeader(14,5,0xf7511000);
     setMask(14,5,0xfffff000);
     setModel(14,6,560);
     setHeader(14,6,0xfca56000);
     setMask(14,6,0xfffff000);
// ROC15
     setCrateType(15,"vme");
     setModel(15,1,1182);
     setHeader(15,1,0xfade1182);
     setMask(15,1,0xffffffff);
     setModel(15,2,1182);
     setHeader(15,2,0xfadf1182);
     setMask(15,2,0xffffffff);
     setModel(15,3,3123);
     setHeader(15,3,0xfadd3123);
     setMask(15,3,0xffffffff);
// ROC16  
     setCrateType(16,"vme");
     for (slot = 1; slot <= 16; slot++) { // actually ADCs (2 per 8 slots)
        setModel(16,slot,550);
	setHeader(16,slot,0xcd000000+((slot-1)<<16));
        setMask(16,slot,0xff0f0000);
     }

     return CM_OK;
  }

void THaCrateMap::print() const
{
  for( int roc=0; roc<MAXROC; roc++ ) {
    if( !crate_used[roc] || nslot[roc] ==0 ) continue;
    cout << "==== Crate " << roc << " type " << crate_type[roc];
    if( !scalerloc[roc].IsNull() )  cout << " \"" << scalerloc[roc] << "\"";
    cout << endl;
    cout << "slot\tmodel\tclear\theader\tmask\tnchan\tndata\n";
    for( int slot=0; slot<MAXSLOT; slot++ ) {
      if( !slot_used[roc][slot] ) continue;
      cout << "  " << slot << "\t" << model[roc][slot] 
	   << "\t" << slot_clear[roc][slot]
	   << "\t0x" << hex << header[roc][slot]
	   << "\t0x" << headmask[roc][slot] << dec
	   << "\t" << nchan[roc][slot]
	   << "\t" << ndata[roc][slot]
	   << endl;
    }
  }
}

ClassImp(THaCrateMap)

/////////////////////////////////////////////////////////////////////
//
//   THaFastBusWord
//   Interpretation of standard fastbus data words
//
//   Given a word of fastbus data, we use methods here
//   to pick out the slot, channel, data, and opt
//   which depends on the fastbus model (1875, 1877, 1881, etc).
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaFastBusWord.h"
#include <iostream>

using namespace std;

namespace Decoder {

  const UShort_t THaFastBusWord::FB_ERR = ~(UShort_t)0;

// Definition of fastbus modules.
void THaFastBusWord::init() {
    UShort_t imod,idx;
    for(imod=0; imod<MAXMODULE; imod++) module_type[imod]=0;
    module_type[0] = 1877;
    module_type[1] = 1881;
    module_type[2] = 1875;
    for (imod=0; imod<MAXMODULE; imod++) {
      idx = module_type[imod]-modoff;
      if (idx < MAXIDX) {
	modindex[idx] = imod;
      } else {
	cout << "Warning, no mapping of module index for "<<idx<<endl;
      }
      switch (module_type[imod]) {
        case 1877:
            module_info[imod].headexists = true;
            module_info[imod].wdcntmask = 0x7ff;
            module_info[imod].datamask  = 0xffff;
            module_info[imod].chanmask  = 0xfe0000;
            module_info[imod].optmask   = 0x10000;
            module_info[imod].chanshift = 17;
            module_info[imod].optshift  = 16;
            module_info[imod].devtype = "tdc";
            break;
        case 1875:
            module_info[imod].headexists = false;
            module_info[imod].wdcntmask = 0;
            module_info[imod].datamask  = 0xfff;
            module_info[imod].chanmask  = 0x7f0000;
            module_info[imod].optmask   = 0x800000;
            module_info[imod].chanshift = 16;
            module_info[imod].optshift  = 23;
            module_info[imod].devtype = "tdc";
            break;
        case 1881:
            module_info[imod].headexists = true;
            module_info[imod].wdcntmask = 0x7f;
            module_info[imod].datamask  = 0x3fff;
            module_info[imod].chanmask  = 0x7e0000;
            module_info[imod].optmask   = 0x3000000;
            module_info[imod].chanshift = 17;
            module_info[imod].optshift  = 24;
            module_info[imod].devtype = "adc";
            break;
        default:     // undefined
            module_info[imod].headexists = false;
            module_info[imod].wdcntmask = 0;
            module_info[imod].datamask  = 0;
            module_info[imod].chanmask  = 0;
            module_info[imod].optmask   = 0;
            module_info[imod].chanshift = 0;
            module_info[imod].optshift  = 0;
            module_info[imod].devtype = "";
       }
    }
}

}

ClassImp(Decoder::THaFastBusWord)

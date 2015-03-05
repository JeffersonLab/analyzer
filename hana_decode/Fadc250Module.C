/////////////////////////////////////////////////////////////////////
//
//   Fadc250Module
//   JLab FADC 250 Module
//
/////////////////////////////////////////////////////////////////////

#include "Fadc250Module.h"
#include "VmeModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

namespace Decoder {

  static const Int_t NADCCHAN = 16;
  static const Int_t MAXDAT   = 1000;

  Module::TypeIter_t Fadc250Module::fgThisType =
    DoRegister( ModuleType( "Decoder::Fadc250Module" , 250 ));

  Fadc250Module::Fadc250Module(Int_t crate, Int_t slot) : VmeModule(crate, slot) {
    fDebugFile=0;
    Init();
  }

  Fadc250Module::~Fadc250Module() {
    if (fNumAInt) delete [] fNumAInt;
    if (fNumTInt) delete [] fNumTInt;
    if (fNumSample) delete [] fNumSample;
    if (fAdcData) delete [] fAdcData;
    if (fTdcData) delete [] fTdcData;
  }

  void Fadc250Module::Init() {
    fNumAInt = new Int_t[NADCCHAN];
    fNumTInt = new Int_t[NADCCHAN];
    fNumSample = new Int_t[NADCCHAN];
    fAdcData = new Int_t[NADCCHAN*MAXDAT];
    fTdcData = new Int_t[NADCCHAN*MAXDAT];
    memset(fNumAInt, 0, NADCCHAN*sizeof(Int_t));
    memset(fNumTInt, 0, NADCCHAN*sizeof(Int_t));
    memset(fNumSample, 0, NADCCHAN*sizeof(Int_t));
    fDebugFile=0;
    f250_setmode=-1;
    f250_foundmode=-2;
    Clear("");
    IsInit = kTRUE;
    fName = "FADC 250";
    SetMode(F250_SAMPLE);  // needs to be driven by cratemap ... later
  }


  Bool_t Fadc250Module::IsSlot(UInt_t rdata) {
    if (fDebugFile) *fDebugFile << "is slot ? "<<hex<<fHeader<<"  "<<fHeaderMask<<"  "<<rdata<<dec<<endl;
    return ((rdata != 0xffffffff) & ((rdata & fHeaderMask)==fHeader));
  }

  void Fadc250Module::CheckSetMode() const {
    if (f250_setmode != F250_SAMPLE && f250_setmode != F250_INTEG) {
      cout << "Check the F250 setmode.  It is = "<<f250_setmode<<endl;
      cout << "And should be set to either "<<F250_SAMPLE<<"   or "<<F250_INTEG<<endl;
    }
  }

  Int_t Fadc250Module::GetMode() const {
    if (fDebugFile) *fDebugFile << "GetMode ... "<<f250_setmode<< "   "<<f250_foundmode<<endl;
    if (f250_setmode == f250_foundmode) return f250_setmode;
    return -1;
  }

  void Fadc250Module::CheckFoundMode() const {
    if (f250_setmode != f250_foundmode) {
      cout << "Fadc250Module:: ERROR: The set mode "<<f250_setmode;
      cout << "   is not consistent with the found mode "<<f250_foundmode<<endl;
    }
  }

  Int_t Fadc250Module::GetAdcData(Int_t chan, Int_t ievent) const {
    Int_t result = GetData(GET_ADC, chan, ievent);
    if (result < 0) cout << "Fadc250Module:: WARNING:  Strange ADC data "<<endl;
    return result;
  }

  Int_t Fadc250Module::GetTdcData(Int_t chan, Int_t ievent) const {
    Int_t result = GetData(GET_TDC, chan, ievent);
    if (result < 0) cout << "Fadc250Module:: WARNING:  Strange TDC data "<<endl;
    return result;
  }


  Int_t Fadc250Module::GetData(Int_t which, Int_t chan, Int_t ievent) const {

    int index;
    int nevent;

    if ( !IsInit ) {
      cout << "ERROR:: Fadc250Module:: Not initialized "<<endl;
      cout << "Need to execute SetMode "<<endl;
    }

    if (chan < 0 || chan > NADCCHAN) {
      cout << "ERROR:: Fadc250Module:: GetAdcData:: invalid channel "<<chan<<endl;
      return -1;
    }

    nevent = 0;

    if (fDebugFile) *fDebugFile << "GetData:  f250_foundmode "<<f250_foundmode<<endl;

    if (f250_foundmode == F250_INTEG) {
      if (which == GET_ADC) {
	nevent = fNumAInt[chan];
      }
      if (which == GET_TDC) {
	nevent = fNumAInt[chan];
      }
    } else {
      nevent = fNumSample[chan];
    }

    if (ievent < 0 || ievent > nevent) {
      if (fDebugFile) {
	*fDebugFile << "Fadc250:: info "<<which<<"   "<<f250_foundmode<<"   "<<f250_setmode<<endl;
	*fDebugFile << "ERROR:: Fadc250Module:: GetData:: invalid event "<<ievent<<"  "<<nevent<<endl;
      }
      return -1;
    }

    index = MAXDAT*chan + ievent;
    if (index < 0 || index > NADCCHAN*MAXDAT) {
      cout << "ERROR:: Fadc250Module:: GetData:: invalid index "<<index<<endl;
      return -1;
    }

    if (f250_foundmode == F250_INTEG) {
      if (which == GET_ADC) {
	return fAdcData[index];
      }
      if (which == GET_TDC) {
	return fTdcData[index];
      }
    } else {
      return fAdcData[index];
    }

    return -1;

  }

  void Fadc250Module::Clear(const Option_t *opt) {
    f250_foundmode=-1;
    fNumEvents = 0;
    fNumTrig = 0;
    if (IsIntegMode()) {
      memset(fNumAInt, 0, NADCCHAN*sizeof(Int_t));
      memset(fNumTInt, 0, NADCCHAN*sizeof(Int_t));
    }
    if (IsSampleMode()) memset(fNumSample, 0, NADCCHAN*sizeof(Int_t));
  }

  Int_t Fadc250Module::LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, const UInt_t *pstop) {
    // this increments evbuffer
    if (fDebugFile) *fDebugFile << "Fadc250Module:: loadslot "<<endl;
    fWordsSeen = 0;
    Int_t numwords = 0;
    if (IsSlot(*evbuffer)) {
      if (IsIntegMode()) numwords = *(evbuffer+2);
      if (IsSampleMode()) numwords = *(evbuffer+4);
    }
    if (fDebugFile) *fDebugFile << "Fadc250Module:: num words "<<dec<<numwords<<endl;
    // Fill data structures of this class
    Clear("");
    while ( evbuffer < pstop && fWordsSeen < numwords ) {
      Decode(evbuffer);
      fWordsSeen++;
      evbuffer++;
    }
    if (fDebugFile) *fDebugFile << "Fadc250Module:: mode info "<<dec<<f250_setmode<<"   "<<f250_foundmode<<endl;
    CheckFoundMode();

    // Now load the THaSlotData

    for (Int_t chan=0; chan<NADCCHAN; chan++) {
      if (fDebugFile) *fDebugFile << "Fadc250:: Chan  "<<chan<<"  num sample "<<fNumSample[chan]<<"  MAXDAT "<<MAXDAT<<endl;
      for (Int_t i=0; i<fNumAInt[chan]; i++) {
	Int_t index = MAXDAT*chan + i;
	if (fDebugFile) *fDebugFile << "Fadc250:: indexing  "<<index<<"  "<<NADCCHAN*MAXDAT<<endl;
	if (index < 0 || index > NADCCHAN*MAXDAT) {
	  if (fDebugFile) *fDebugFile << "Fadc250:: INDEX problem !  "<<index<<endl;
	  cerr << "Fadc250:: WARN: index out of range "<<dec<<index<<endl;
	} else {
	  sldat->loadData("adc", chan, fAdcData[index], fAdcData[index]);
	}
      }
      for (Int_t i=0; i<fNumTInt[chan]; i++) {
	Int_t index = MAXDAT*chan + i;
	if (index < 0 || index > NADCCHAN*MAXDAT) {
	  cerr << "Fadc250:: WARN: index out of range "<<dec<<index<<endl;
	} else {
	  sldat->loadData("tdc", chan, fTdcData[index], fTdcData[index]);
	}
      }
      for (Int_t i=0; i<fNumSample[chan]; i++) {
	Int_t index = MAXDAT*chan + i;
	if (fDebugFile) *fDebugFile << "channel "<<dec<<chan<<"  "<<i<<"  "<<index<<"   data "<<fAdcData[index]<<endl;
	if (index < 0 || index > NADCCHAN*MAXDAT) {
	  cerr << "Fadc250:: WARN: index out of range "<<dec<<index<<endl;
	} else {
	  sldat->loadData("adc", chan, fAdcData[index], fAdcData[index]);
	}
      }
    }

    return fWordsSeen;
  }

  Int_t Fadc250Module::Decode(const UInt_t *pdat)
  { // Routine from B. Moffit, adapted by R. Michaels for this class.
    // Note, there are several modes, but for now (Aug 2014) we only use two.

    int i_print = 1;
    static unsigned int type_last = 15;	/* initialize to type FILLER WORD */
    static unsigned int time_last = 0;
    static unsigned int iword=0;

    UInt_t data = *pdat;
    Int_t nsamples, index, chan;

    if ((i_print==1) && (fDebugFile > 0)) *fDebugFile << "Fadc250::Decode   "<< data<<"   "<<iword<<endl;
    iword++;

    if( data & 0x80000000 )		/* data type defining word */
      {
	fadc_data.new_type = 1;
	fadc_data.type = (data & 0x78000000) >> 27;
      }
    else
      {
	fadc_data.new_type = 0;
	fadc_data.type = type_last;
      }

    switch( fadc_data.type )
      {
      case 0:		/* BLOCK HEADER */
	fadc_data.slot_id_hd = ((data) & 0x7C00000) >> 22;
	fadc_data.n_evts = (data & 0x3FF800) >> 11;
	fadc_data.blk_num = (data & 0x7FF);
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - BLOCK HEADER - slot = %d   n_evts = %d   n_blk = %d\n"<<hex<<data<<dec<<"  "<< fadc_data.slot_id_hd<<"  "<< fadc_data.n_evts<<"  "<< fadc_data.blk_num<<endl;
	break;
      case 1:		/* BLOCK TRAILER */
	fadc_data.slot_id_tr = (data & 0x7C00000) >> 22;
	fadc_data.n_words = (data & 0x3FFFFF);
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - BLOCK TRAILER - slot = %d   n_words = %d\n"<<"  "<<hex<< data<<dec<<"  "<< fadc_data.slot_id_tr<<"  "<< fadc_data.n_words<<endl;
	break;
      case 2:		/* EVENT HEADER */
	if( fadc_data.new_type )
	  {
	    fadc_data.evt_num_1 = (data & 0x7FFFFFF);
	    fNumEvents++;
	    fNumTrig = fadc_data.evt_num_1;
	    if(  (i_print == 1) && (fDebugFile > 0) )
	      *fDebugFile <<"%8X - EVENT HEADER 1 - evt_num = %d\n"<<"  "<< hex<<data<<dec<<"  "<< fadc_data.evt_num_1<<"  "<<fNumEvents<<endl;
	  }
	else
	  {
	    fadc_data.evt_num_2 = (data & 0x7FFFFFF);
	    if(  (i_print == 1) && (fDebugFile > 0) )
	      *fDebugFile <<"%8X - EVENT HEADER 2 - evt_num = %d\n"<< "  "<<hex<<data<<dec<<"  "<< fadc_data.evt_num_2<<endl;
	  }
	break;
      case 3:		/* TRIGGER TIME */
	if( fadc_data.new_type )
	  {
	    fadc_data.time_1 = (data & 0xFFFFFF);
	    if(  (i_print == 1) && (fDebugFile > 0) )
	      *fDebugFile <<"%8X - TRIGGER TIME 1 - time = %08x\n"<<"  "<<data<<"  "<< fadc_data.time_1<<endl;
	    fadc_data.time_now = 1;
	    time_last = 1;
	  }
	else
	  {
	    if( time_last == 1 )
	      {
		fadc_data.time_2 = (data & 0xFFFFFF);
		if(  (i_print == 1) && (fDebugFile > 0) )
		  *fDebugFile <<"%8X - TRIGGER TIME 2 - time = %08x\n"<<"  "<< hex<< data<<dec<<"  "<< fadc_data.time_2<<endl;
		fadc_data.time_now = 2;
	      }
	    else if( time_last == 2 )
	      {
		fadc_data.time_3 = (data & 0xFFFFFF);
		if(  (i_print == 1) && (fDebugFile > 0) )
		  *fDebugFile <<"%8X - TRIGGER TIME 3 - time = %08x\n"<<"  "<< hex<<data<<dec<<"  "<<fadc_data.time_3<<endl;
		fadc_data.time_now = 3;
	      }
	    else if( time_last == 3 )
	      {
		fadc_data.time_4 = (data & 0xFFFFFF);
		if(  (i_print == 1) && (fDebugFile > 0) )
		  *fDebugFile <<"%8X - TRIGGER TIME 4 - time = %08x\n"<<"  "<<hex<< data<<dec<<"   "<< fadc_data.time_4<<endl;
		fadc_data.time_now = 4;
	      }
	    else
	      if(  (i_print == 1) && (fDebugFile > 0) )
		*fDebugFile <<"%8X - TRIGGER TIME - (ERROR)\n"<<"  "<<hex<< data<<dec<<endl;

	    time_last = fadc_data.time_now;
	  }
	break;
      case 4:		/* WINDOW RAW DATA */

	if( fadc_data.new_type )
	  {
	    fadc_data.chan = (data & 0x7800000) >> 23;
	    fadc_data.width = (data & 0xFFF);
	    nsamples = fadc_data.width;
	    if(  (i_print == 1) && (fDebugFile > 0) )
	      *fDebugFile <<"%8X - WINDOW RAW DATA - chan = %d   nsamples = %d\n"<<"  "<<	data<<"   "<< fadc_data.chan<<"  "<< fadc_data.width<<endl;
	  }
	else
	  {
	    fadc_data.valid_1 = 1;
	    fadc_data.valid_2 = 1;
	    fadc_data.adc_1 = (data & 0x1FFF0000) >> 16;
	    if( data & 0x20000000 )
	      fadc_data.valid_1 = 0;
	    fadc_data.adc_2 = (data & 0x1FFF);
	    if( data & 0x2000 )
	      fadc_data.valid_2 = 0;
	    if(  (i_print == 1) && (fDebugFile > 0) )
	      *fDebugFile <<"%8X - RAW SAMPLES - valid = %d  adc = %4d   valid = %d  adc = %4d\n"<< "  "<<hex<<data<<"   "<<dec<< fadc_data.valid_1<<"   "<< fadc_data.adc_1<< "  "<<fadc_data.valid_2<<"   "<< fadc_data.adc_2<<endl;
	    f250_foundmode = F250_SAMPLE;
	    if(  (i_print == 1) && (fDebugFile > 0) )
	      *fDebugFile << "Setting f250_foundmode "<<f250_foundmode<<endl;
	    chan=0;
	    if (fadc_data.chan < static_cast<UInt_t>(NADCCHAN)) {
	      chan = fadc_data.chan;
	    } else {
	      cout << "ERROR:: Fadc250Module:: ADC channel makes no sense !"<<endl;
	    }
	    for (Int_t ii=0; ii<2; ii++) {
	      index = MAXDAT*fadc_data.chan + fNumSample[chan];
	      fNumSample[chan]++;
	      if (index >= 0 && index < NADCCHAN*MAXDAT) {
		if (ii==0) {
		  fAdcData[index] = fadc_data.adc_1;
		} else {
		  fAdcData[index] = fadc_data.adc_2;
		}
	      } else {
		cout << "Fadc250Module:: too many raw data words ? "<<endl;
	      }
	    }
	    if(  (i_print == 1) && (fDebugFile > 0) )
	      *fDebugFile <<"Chan "<<chan<<"    Num samples "<<fNumSample[chan]<<"   mode "<<f250_foundmode<<endl;
	  }
	break;
      case 5:		/* WINDOW SUM */
	fadc_data.over = 0;
	fadc_data.chan = (data & 0x7800000) >> 23;
	fadc_data.adc_sum = (data & 0x3FFFFF);
	if( data & 0x400000 )
	  fadc_data.over = 1;
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - WINDOW SUM - chan = %d   over = %d   adc_sum = %08x\n"<<"  "<<hex<<data<<"  "<<dec<< fadc_data.chan<<"  "<< fadc_data.over<<"  "<< fadc_data.adc_sum<<endl;
	break;
      case 6:		/* PULSE RAW DATA */
	if( fadc_data.new_type )
	  {
	    fadc_data.chan = (data & 0x7800000) >> 23;
	    fadc_data.pulse_num = (data & 0x600000) >> 21;
	    fadc_data.thres_bin = (data & 0x3FF);
	    if(  (i_print == 1) && (fDebugFile > 0) )
	      *fDebugFile <<"%8X - PULSE RAW DATA - chan = %d   pulse # = %d   threshold bin = %d\n"<<"  "<<hex<<data<<dec<<"  "<< fadc_data.chan<<"   "<< fadc_data.pulse_num<<"  "<< fadc_data.thres_bin<<endl;
	  }
	else
	  {
	    fadc_data.valid_1 = 1;
	    fadc_data.valid_2 = 1;
	    fadc_data.adc_1 = (data & 0x1FFF0000) >> 16;
	    if( data & 0x20000000 )
	      fadc_data.valid_1 = 0;
	    fadc_data.adc_2 = (data & 0x1FFF);
	    if( data & 0x2000 )
	      fadc_data.valid_2 = 0;
	    if(  (i_print == 1) && (fDebugFile > 0) )
	      *fDebugFile <<"%8X - PULSE RAW SAMPLES - valid = %d  adc = %d   valid = %d  adc = %d\n"<<"  "<<hex<<data<<dec<<"  "<< fadc_data.valid_1<<"  "<< fadc_data.adc_1<<"  "<< fadc_data.valid_2<<"  "<< fadc_data.adc_2<<endl;
	  }
	break;
      case 7:		/* PULSE INTEGRAL */
	f250_foundmode = F250_INTEG;
	fadc_data.chan = (data & 0x7800000) >> 23;
	fadc_data.pulse_num = (data & 0x600000) >> 21;
	fadc_data.quality = (data & 0x180000) >> 19;
	fadc_data.integral = (data & 0x7FFFF);
	chan=0;
	if (fadc_data.chan < static_cast<UInt_t>(NADCCHAN)) {
	  chan = fadc_data.chan;
	} else {
	  cout << "ERROR:: Fadc250Module:: ADC channel makes no sense !"<<endl;
	}
	index = MAXDAT*chan + fNumAInt[chan];
	fNumAInt[chan]++;
	if (index >= 0 && index < NADCCHAN*MAXDAT) {
	  fAdcData[index] = fadc_data.integral;
	} else {
	  cout << "Fadc250Module:: too many raw data words ? "<<endl;
	}
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - PULSE INTEGRAL - chan = %d   pulse # = %d   quality = %d   integral = %d\n"<<"  "<< data<<"  "<< fadc_data.chan<<"  "<< fadc_data.pulse_num<<"  "<< fadc_data.quality<<"  "<< fadc_data.integral<<endl;
	break;
      case 8:		/* PULSE TIME */
	fadc_data.chan = (data & 0x7800000) >> 23;
	fadc_data.pulse_num = (data & 0x600000) >> 21;
	fadc_data.quality = (data & 0x180000) >> 19;
	fadc_data.time = (data & 0xFFFF);
	chan=0;
	if (fadc_data.chan < static_cast<UInt_t>(NADCCHAN)) {
	  chan = fadc_data.chan;
	} else {
	  cout << "ERROR:: Fadc250Module:: ADC channel makes no sense !"<<endl;
	}
	index = NADCCHAN*chan + fNumTInt[chan];
	fNumTInt[chan]++;
	if (index >= 0 && index < NADCCHAN*MAXDAT) {
	  fTdcData[index] = fadc_data.time;
	} else {
	  cout << "Fadc250Module:: too many raw data words ? "<<endl;
	}
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - PULSE TIME - chan = %d   pulse # = %d   quality = %d   time = %d\n"<<"  "<<hex<<data<<dec<<"   "<< fadc_data.chan<<"  "<< fadc_data.pulse_num<<"  "<<fadc_data.quality<< "  "<< fadc_data.time<<endl;
	break;
      case 9:		/* STREAMING RAW DATA */
	if( fadc_data.new_type )
	  {
	    fadc_data.chan_a = (data & 0x3C00000) >> 22;
	    fadc_data.source_a = (data & 0x4000000) >> 26;
	    fadc_data.chan_b = (data & 0x1E0000) >> 17;
	    fadc_data.source_b = (data & 0x200000) >> 21;
	    if(  (i_print == 1) && (fDebugFile > 0) )
	      *fDebugFile <<"%8X - STREAMING RAW DATA - ena A = %d  chan A = %d   ena B = %d  chan B = %d\n"<<"  "<<hex<<data<<dec<<"  "<< fadc_data.source_a<<"  "<< fadc_data.chan_a<<"  "<< fadc_data.source_b<<"   "<< fadc_data.chan_b<<endl;
	  }
	else
	  {
	    fadc_data.valid_1 = 1;
	    fadc_data.valid_2 = 1;
	    fadc_data.adc_1 = (data & 0x1FFF0000) >> 16;
	    if( data & 0x20000000 )
	      fadc_data.valid_1 = 0;
	    fadc_data.adc_2 = (data & 0x1FFF);
	    if( data & 0x2000 )
	      fadc_data.valid_2 = 0;
	    fadc_data.group = (data & 0x40000000) >> 30;
	    if( fadc_data.group )
	      {
		if(  (i_print == 1) && (fDebugFile > 0) )
		  *fDebugFile <<"%8X - RAW SAMPLES B - valid = %d  adc = %d   valid = %d  adc = %d\n"<< "  "<<hex<<data<<dec<<"  "<< fadc_data.valid_1<< "  "<< fadc_data.adc_1<<"  "<< fadc_data.valid_2 <<"  "<< fadc_data.adc_2<<endl;
	      }
	    else
	      if(  (i_print == 1) && (fDebugFile > 0) )
		*fDebugFile <<"%8X - RAW SAMPLES A - valid = %d  adc = %d   valid = %d  adc = %d\n"<<"  "<<hex<<data<<dec<<"  "<< fadc_data.valid_1<<"  "<< fadc_data.adc_1<<"  "<< fadc_data.valid_2 << "  "<< fadc_data.adc_2<<endl;
	  }
	break;
      case 10:		/* PULSE AMPLITUDE DATA */
	fadc_data.chan = (data & 0x7800000) >> 23;
	fadc_data.pulse_num = (data & 0x600000) >> 21;
	fadc_data.vmin = (data & 0x1FF000) >> 12;
	fadc_data.vpeak = (data & 0xFFF);
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - PULSE V - chan = %d   pulse # = %d   vmin = %d   vpeak = %d\n" << "   "<<hex<<data<<dec<<"   "<< fadc_data.chan<<"  "<< fadc_data.pulse_num<<"  "<< fadc_data.vmin<<"   "<< fadc_data.vpeak<<endl;
	break;

      case 11:		/* INTERNAL TRIGGER WORD */
	fadc_data.trig_type_int = data & 0x7;
	fadc_data.trig_state_int = (data & 0x8) >> 3;
	fadc_data.evt_num_int = (data & 0xFFF0) >> 4;
	fadc_data.err_status_int = (data & 0x10000) >> 16;
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - INTERNAL TRIGGER - type = %d   state = %d   num = %d   error = %d\n"<<"  "<<hex<<data<<dec<<"  "<< fadc_data.trig_type_int<<"  "<< fadc_data.trig_state_int<<"  "<< fadc_data.evt_num_int <<"  "<<fadc_data.err_status_int<<endl;
      case 12:		/* UNDEFINED TYPE */
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - UNDEFINED TYPE = %d\n"<<"  "<< data<<"  "<< fadc_data.type<<endl;
	break;
      case 13:		/* END OF EVENT */
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - END OF EVENT = %d\n"<< "  " << data<< "  " << fadc_data.type<<endl;
	break;
      case 14:		/* DATA NOT VALID (no data available) */
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - DATA NOT VALID = %d\n"<< "  " << data<< "  " << fadc_data.type<<endl;
	break;
      case 15:		/* FILLER WORD */
	if(  (i_print == 1) && (fDebugFile > 0) )
	  *fDebugFile <<"%8X - FILLER WORD = %d\n"<< "  " << data<< "  " << fadc_data.type<<endl;
	break;
      }

    type_last = fadc_data.type;	/* save type of current data word */

    return 1;

  }

}

ClassImp(Decoder::Fadc250Module)

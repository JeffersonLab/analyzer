////////////////////////////////////////////////////////////////////
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

//#define DEBUG

namespace Decoder {

  static const Int_t NADCCHAN = 16;
  static const Int_t MAXDAT   = 1000;

  Module::TypeIter_t Fadc250Module::fgThisType =
    DoRegister( ModuleType( "Decoder::Fadc250Module" , 250 ));

  Fadc250Module::Fadc250Module(Int_t crate, Int_t slot)
    : VmeModule(crate, slot), fNumAInt(0), fNumTInt(0), fNumSample(0),
      fAdcData(0), fTdcData(0)
  {
    Init();
  }

  Fadc250Module::~Fadc250Module() {
    delete [] fNumAInt;
    delete [] fNumTInt;
    delete [] fNumSample;
    delete [] fAdcData;
    delete [] fTdcData;
#if defined DEBUG && defined WITH_DEBUG
    delete fDebugFile; fDebugFile = 0;
#endif
  }

  void Fadc250Module::Init() {
    // Delete memory in case of re-Init()
    //delete [] fNumAInt;
    //delete [] fNumTInt;
    //delete [] fNumSample;
    //delete [] fAdcData;
    //delete [] fTdcData;
#if defined DEBUG && defined WITH_DEBUG
    // this will make a HUGE output
    if( fDebugFile ) fDebugFile->close();
    fDebugFile=new ofstream;
    fDebugFile->open("fadcdebug.dat");
#endif
    fNumAInt = new Int_t[NADCCHAN];
    fNumTInt = new Int_t[NADCCHAN];
    fNumSample = new Int_t[NADCCHAN];
    fAdcData = new Int_t[NADCCHAN*MAXDAT];
    fTdcData = new Int_t[NADCCHAN*MAXDAT];
    memset(fNumAInt, 0, NADCCHAN*sizeof(Int_t));
    memset(fNumTInt, 0, NADCCHAN*sizeof(Int_t));
    memset(fNumSample, 0, NADCCHAN*sizeof(Int_t));
    f250_setmode=-1;
    f250_foundmode=-2;
    Clear("");
    IsInit = kTRUE;
    fName = "FADC 250";
    SetMode(F250_SAMPLE);  // needs to be driven by cratemap ... later
  }


  Bool_t Fadc250Module::IsSlot(UInt_t rdata) {
#ifdef WITH_DEBUG
    if (fDebugFile) {
      *fDebugFile << "is slot ? "<<hex<<fHeader<<"  "<<fHeaderMask<<"  "<<rdata<<dec<<endl;
      if ((rdata != 0xffffffff) & ((rdata & fHeaderMask)==fHeader))
	*fDebugFile << "Yes, is slot "<<endl;
    }
#endif
    return ((rdata != 0xffffffff) & ((rdata & fHeaderMask)==fHeader));
  }

  void Fadc250Module::CheckSetMode() const {
    if (f250_setmode != F250_SAMPLE && f250_setmode != F250_INTEG) {
      cout << "Check the F250 setmode.  It is = "<<f250_setmode<<endl;
      cout << "And should be set to either "<<F250_SAMPLE<<"   or "<<F250_INTEG<<endl;
    }
  }

  Int_t Fadc250Module::GetMode() const {
#ifdef WITH_DEBUG
    if (fDebugFile)
      *fDebugFile << "GetMode ... "<<f250_setmode<< "   "<<f250_foundmode<<endl;
#endif
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

  Int_t Fadc250Module::GetNumSamples(Int_t chan) const {
    if (chan < 0 || chan > NADCCHAN) {
      cout << "ERROR:: Fadc250Module:: GetAdcData:: invalid channel "<<chan<<endl;
      return 0;
    }
    return fNumSample[chan];
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

#ifdef WITH_DEBUG
    if (fDebugFile)
      *fDebugFile << "GetData:  f250_foundmode "<<f250_foundmode<<endl;
#endif
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
#ifdef WITH_DEBUG
      if (fDebugFile) {
	*fDebugFile << "Fadc250:: info "<<which<<"   "<<f250_foundmode<<"   "<<f250_setmode<<endl;
	*fDebugFile << "ERROR:: Fadc250Module:: GetData:: invalid event "<<ievent<<"  "<<nevent<<endl;
      }
#endif
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
#ifdef WITH_DEBUG
    if (fDebugFile)
      *fDebugFile << "Fadc250Module:: loadslot "<<endl;
#endif
    fWordsSeen = 0;
    Int_t numwords = 0;
    if (IsSlot(*evbuffer)) {
      if (IsIntegMode()) numwords = *(evbuffer+2);
      if (IsSampleMode()) numwords = 10000;  // Where is it ?
    }
// This would not happen if CRL puts numwords into output
    if (numwords < 0) cout << "Fadc250: warning: negative num words ?"<<endl;
#ifdef WITH_DEBUG
    if (fDebugFile)
      *fDebugFile << "Fadc250Module:: num words "<<dec<<numwords<<endl;
#endif
    // Fill data structures of this class
    Clear("");
    // Read until out of data or until Decode says that the slot is finished
    Int_t btrail=0;
    while ( evbuffer < pstop && fWordsSeen < numwords && btrail==0) {
      btrail=Decode(evbuffer);
      fWordsSeen++;
      evbuffer++;
    }
#ifdef WITH_DEBUG
    if (fDebugFile)
      *fDebugFile << "Fadc250Module:: mode info "<<dec
		  <<f250_setmode<<"   "<<f250_foundmode<<endl;
#endif
    CheckFoundMode();

    // Now load the THaSlotData

    for (Int_t chan=0; chan<NADCCHAN; chan++) {
#ifdef WITH_DEBUG
      if (fDebugFile)
	*fDebugFile << "Fadc250:: Chan  "<<chan<<"  num sample "
		    <<fNumSample[chan]<<"  MAXDAT "<<MAXDAT<<endl;
#endif
      for (Int_t i=0; i<fNumAInt[chan]; i++) {
	Int_t index = MAXDAT*chan + i;
#ifdef WITH_DEBUG
	if (fDebugFile)
	  *fDebugFile << "Fadc250:: indexing  "<<index<<"  "<<NADCCHAN*MAXDAT<<endl;
#endif
	if (index < 0 || index > NADCCHAN*MAXDAT) {
#ifdef WITH_DEBUG
	  if (fDebugFile)
	    *fDebugFile << "Fadc250:: INDEX problem !  "<<index<<endl;
#endif
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
#ifdef WITH_DEBUG
	if (fDebugFile)
	  *fDebugFile << "channel "<<dec<<chan<<"  "<<i<<"  "
		      <<index<<"   data "<<fAdcData[index]<<endl;
#endif
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

    static unsigned int type_last = 15;	/* initialize to type FILLER WORD */
    static unsigned int time_last = 0;
    static unsigned int iword=0;

    Int_t return_value=0;

    int i_print = 1;
    UInt_t data = *pdat;
    Int_t nsamples, index, chan;

#ifdef WITH_DEBUG
    if ((i_print==1) && (fDebugFile !=0))
      *fDebugFile << "Fadc250::Decode   "<< hex<< data<<"   "<<dec<<iword<<endl;
#endif
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
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - BLOCK HEADER - slot = %d   n_evts = %d   n_blk = %d\n"
		      <<hex<<data<<dec<<"  "<< fadc_data.slot_id_hd<<"  "
		      << fadc_data.n_evts<<"  "<< fadc_data.blk_num<<endl;
#endif
	break;
      case 1:		/* BLOCK TRAILER */
	fadc_data.slot_id_tr = (data & 0x7C00000) >> 22;
	fadc_data.n_words = (data & 0x3FFFFF);
	return_value = 1;	// Indicate block trailer found
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - BLOCK TRAILER - slot = %d   n_words = %d\n  "
		      <<hex<< data<<dec<<"  "<< fadc_data.slot_id_tr<<"  "
		      << fadc_data.n_words<<endl;
#endif
	break;
      case 2:		/* EVENT HEADER */
	if( fadc_data.new_type )
	  {
	    fadc_data.evt_num_1 = (data & 0x7FFFFFF);
	    fNumEvents++;
	    fNumTrig = fadc_data.evt_num_1;
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      *fDebugFile <<"%8X - EVENT HEADER 1 - evt_num = %d\n  "
			  << hex<<data<<dec<<"  "<< fadc_data.evt_num_1<<"  "
			  <<fNumEvents<<endl;
#endif
	  }
	else
	  {
	    fadc_data.evt_num_2 = (data & 0x7FFFFFF);
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      *fDebugFile <<"%8X - EVENT HEADER 2 - evt_num = %d\n  "
			  <<hex<<data<<dec<<"  "<< fadc_data.evt_num_2<<endl;
#endif
	  }
	break;
      case 3:		/* TRIGGER TIME */
	if( fadc_data.new_type )
	  {
	    fadc_data.time_1 = (data & 0xFFFFFF);
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      *fDebugFile <<"%8X - TRIGGER TIME 1 - time = %08x\n  "
			  <<data<<"  "<< fadc_data.time_1<<endl;
#endif
	    fadc_data.time_now = 1;
	    time_last = 1;
	  }
	else
	  {
	    if( time_last == 1 )
	      {
		fadc_data.time_2 = (data & 0xFFFFFF);
#ifdef WITH_DEBUG
		if(  (i_print == 1) && (fDebugFile != 0) )
		  *fDebugFile <<"%8X - TRIGGER TIME 2 - time = %08x\n  "
			      << hex<< data<<dec<<"  "<< fadc_data.time_2<<endl;
#endif
		fadc_data.time_now = 2;
	      }
	    else if( time_last == 2 )
	      {
		fadc_data.time_3 = (data & 0xFFFFFF);
#ifdef WITH_DEBUG
		if(  (i_print == 1) && (fDebugFile != 0) )
		  *fDebugFile <<"%8X - TRIGGER TIME 3 - time = %08x\n  "
			      << hex<<data<<dec<<"  "<<fadc_data.time_3<<endl;
#endif
		fadc_data.time_now = 3;
	      }
	    else if( time_last == 3 )
	      {
		fadc_data.time_4 = (data & 0xFFFFFF);
#ifdef WITH_DEBUG
		if(  (i_print == 1) && (fDebugFile != 0) )
		  *fDebugFile <<"%8X - TRIGGER TIME 4 - time = %08x\n  "
			      <<hex<< data<<dec<<"   "<< fadc_data.time_4<<endl;
#endif
		fadc_data.time_now = 4;
	      }
	    else
#ifdef WITH_DEBUG
	      if(  (i_print == 1) && (fDebugFile != 0) )
		*fDebugFile <<"%8X - TRIGGER TIME - (ERROR)\n  "
			    <<hex<< data<<dec<<endl;
#endif
	    time_last = fadc_data.time_now;
	  }
	break;
      case 4:		/* WINDOW RAW DATA */

	if( fadc_data.new_type )
	  {
	    fadc_data.chan = (data & 0x7800000) >> 23;
	    fadc_data.width = (data & 0xFFF);
	    nsamples = fadc_data.width;
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      *fDebugFile <<"%8X - WINDOW RAW DATA - chan = %d   nsamples = %d\n  "
			  <<data<<"   "<< fadc_data.chan
			  <<"  "<< fadc_data.width<<endl;
#endif
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
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      *fDebugFile <<"%8X - RAW SAMPLES - valid = %d  adc = %4d   "
			  <<"valid = %d  adc = %4d\n  "
			  <<hex<<data<<"   "<<dec<< fadc_data.valid_1<<"   "
			  << fadc_data.adc_1<< "  "<<fadc_data.valid_2<<"   "
			  << fadc_data.adc_2<<endl;
#endif
	    f250_foundmode = F250_SAMPLE;
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      *fDebugFile << "Setting f250_foundmode "<<f250_foundmode<<endl;
#endif
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
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      *fDebugFile <<"Chan "<<chan<<"    Num samples "
			  <<fNumSample[chan]<<"   mode "<<f250_foundmode<<endl;
#endif
	  }
	break;
      case 5:		/* WINDOW SUM */
	fadc_data.over = 0;
	fadc_data.chan = (data & 0x7800000) >> 23;
	fadc_data.adc_sum = (data & 0x3FFFFF);
	if( data & 0x400000 )
	  fadc_data.over = 1;
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - WINDOW SUM - chan = %d   over = %d   adc_sum = %08x\n  "
		      <<hex<<data<<"  "<<dec<< fadc_data.chan<<"  "
		      << fadc_data.over<<"  "<< fadc_data.adc_sum<<endl;
#endif
	break;
      case 6:		/* PULSE RAW DATA */
	if( fadc_data.new_type )
	  {
	    fadc_data.chan = (data & 0x7800000) >> 23;
	    fadc_data.pulse_num = (data & 0x600000) >> 21;
	    fadc_data.thres_bin = (data & 0x3FF);
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      *fDebugFile <<"%8X - PULSE RAW DATA - chan = %d   pulse # = %d   "
			  <<"threshold bin = %d\n  "
			  <<hex<<data<<dec<<"  "<< fadc_data.chan<<"   "
			  << fadc_data.pulse_num<<"  "<< fadc_data.thres_bin<<endl;
#endif
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
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      *fDebugFile <<"%8X - PULSE RAW SAMPLES - valid = %d  adc = %d   "
			  <<"valid = %d  adc = %d\n  "
			  <<hex<<data<<dec<<"  "<< fadc_data.valid_1<<"  "
			  << fadc_data.adc_1<<"  "<< fadc_data.valid_2<<"  "
			  << fadc_data.adc_2<<endl;
#endif
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
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - PULSE INTEGRAL - chan = %d   pulse # = %d   "
		      <<"quality = %d   integral = %d\n  "
		      << data<<"  "<< fadc_data.chan<<"  "
		      << fadc_data.pulse_num<<"  "<< fadc_data.quality<<"  "
		      << fadc_data.integral<<endl;
#endif
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
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - PULSE TIME - chan = %d   pulse # = %d  "
		      <<"quality = %d   time = %d\n  "<<hex<<data<<dec
		      <<"   "<< fadc_data.chan<<"  "<< fadc_data.pulse_num
		      <<"  "<<fadc_data.quality<< "  "<< fadc_data.time<<endl;
#endif
	break;
      case 9:		/* STREAMING RAW DATA */
	if( fadc_data.new_type )
	  {
	    fadc_data.chan_a = (data & 0x3C00000) >> 22;
	    fadc_data.source_a = (data & 0x4000000) >> 26;
	    fadc_data.chan_b = (data & 0x1E0000) >> 17;
	    fadc_data.source_b = (data & 0x200000) >> 21;
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      *fDebugFile <<"%8X - STREAMING RAW DATA - ena A = %d  chan A = %d  "
			  <<"ena B = %d  chan B = %d\n  "<<hex<<data<<dec
			  <<"  "<< fadc_data.source_a<<"  "<< fadc_data.chan_a
			  <<"  "<< fadc_data.source_b<<"   "<< fadc_data.chan_b<<endl;
#endif
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
#ifdef WITH_DEBUG
	    if(  (i_print == 1) && (fDebugFile != 0) )
	      {
		if( fadc_data.group )
		  *fDebugFile <<"%8X - RAW SAMPLES B - valid = %d  adc = %d  "
			      <<"valid = %d  adc = %d\n  "<<hex<<data
			      <<dec<<"  "<< fadc_data.valid_1<< "  "
			      << fadc_data.adc_1<<"  "<< fadc_data.valid_2
			      <<"  "<< fadc_data.adc_2<<endl;
		else
		  *fDebugFile <<"%8X - RAW SAMPLES A - valid = %d  adc = %d  "
			      <<"valid = %d  adc = %d\n  "<<hex<<data<<dec
			      <<"  "<< fadc_data.valid_1<<"  "<< fadc_data.adc_1
			      <<"  "<< fadc_data.valid_2 << "  "
			      << fadc_data.adc_2<<endl;
	      }
#endif
	  }
	break;
      case 10:		/* PULSE AMPLITUDE DATA */
	fadc_data.chan = (data & 0x7800000) >> 23;
	fadc_data.pulse_num = (data & 0x600000) >> 21;
	fadc_data.vmin = (data & 0x1FF000) >> 12;
	fadc_data.vpeak = (data & 0xFFF);
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - PULSE V - chan = %d   pulse # = %d   vmin = %d   "
		      <<"vpeak = %d\n   "<<hex<<data<<dec<<"   "
		      << fadc_data.chan<<"  "<< fadc_data.pulse_num<<"  "
		      << fadc_data.vmin<<"   "<< fadc_data.vpeak<<endl;
#endif
	break;

      case 11:		/* INTERNAL TRIGGER WORD */
	fadc_data.trig_type_int = data & 0x7;
	fadc_data.trig_state_int = (data & 0x8) >> 3;
	fadc_data.evt_num_int = (data & 0xFFF0) >> 4;
	fadc_data.err_status_int = (data & 0x10000) >> 16;
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - INTERNAL TRIGGER - type = %d   state = %d   "
		      <<"num = %d   error = %d\n  "
		      <<hex<<data<<dec<<"  "<<fadc_data.trig_type_int
		      <<"  "<<fadc_data.trig_state_int
		      <<"  "<< fadc_data.evt_num_int
		      <<"  "<<fadc_data.err_status_int
		      <<endl;
#endif
	break;
      case 12:		/* UNDEFINED TYPE */
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - UNDEFINED TYPE = %d\n  "<< data<<"  "<< fadc_data.type<<endl;
	break;
#endif
      case 13:		/* END OF EVENT */
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - END OF EVENT = %d\n  " << data<< "  " << fadc_data.type<<endl;
#endif
	break;
      case 14:		/* DATA NOT VALID (no data available) */
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - DATA NOT VALID = %d\n  " << data<< "  " << fadc_data.type<<endl;
#endif
	break;
      case 15:		/* FILLER WORD */
#ifdef WITH_DEBUG
	if(  (i_print == 1) && (fDebugFile != 0) )
	  *fDebugFile <<"%8X - FILLER WORD = %d\n  " << data<< "  " << fadc_data.type<<endl;
#endif
	break;
      }

    type_last = fadc_data.type;	/* save type of current data word */

    return return_value;

  }

}

ClassImp(Decoder::Fadc250Module)

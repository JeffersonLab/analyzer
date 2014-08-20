#ifndef Fadc250Module_
#define Fadc250Module_

/////////////////////////////////////////////////////////////////////
//
//   Fadc250Module
//   JLab FADC 250 Module
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "VmeModule.h"

#define NADCCHAN   16
#define MAXDAT     50000

class Fadc250Module : public VmeModule {

public:

   Fadc250Module() {};  
   Fadc250Module(Int_t crate, Int_t slot);  
   virtual ~Fadc250Module();  

   void Init();
   Bool_t IsSlot(UInt_t rdata);
   Int_t Decode(const Int_t *evbuffer);

   Int_t GetNumEvents() { return fNumEvents; };

   Int_t GetAdcData(Int_t chan, Int_t ievent); 
   Int_t GetTdcData(Int_t chan, Int_t ievent);  

   void SetMode(Int_t mode) { 
     f250_setmode = mode;
     IsInit = kTRUE;
     CheckSetMode();
   }


private:
 
   enum { F250_SAMPLE = 1, F250_INTEG = 2 };  // supported modes
   Int_t f250_setmode, f250_foundmode;
   enum { GET_ADC = 1, GET_TDC = 2 };  

   struct fadc_data_struct {
      unsigned int new_type;	
      unsigned int type;	
      unsigned int slot_id_hd;
      unsigned int slot_id_tr;
      unsigned int n_evts;
      unsigned int blk_num;
      unsigned int n_words;
      unsigned int evt_num_1;
      unsigned int evt_num_2;
      unsigned int time_now;
      unsigned int time_1;
      unsigned int time_2;
      unsigned int time_3;
      unsigned int time_4;
      unsigned int chan;
      unsigned int width;
      unsigned int valid_1;
      unsigned int adc_1;
      unsigned int valid_2;
      unsigned int adc_2;
      unsigned int over;
      unsigned int adc_sum;
      unsigned int pulse_num;
      unsigned int thres_bin;
      unsigned int quality;
      unsigned int integral;
      unsigned int time;
      unsigned int chan_a;
      unsigned int source_a;
      unsigned int chan_b;
      unsigned int source_b;
      unsigned int group;
      unsigned int time_coarse;
      unsigned int time_fine;
      unsigned int vmin;
      unsigned int vpeak;
      unsigned int trig_type_int;	/* next 4 for internal trigger data */
      unsigned int trig_state_int;	/* e.g. helicity */
      unsigned int evt_num_int;
      unsigned int err_status_int;
   };
  
   fadc_data_struct fadc_data;

   Int_t GetData(Int_t chan, Int_t event, Int_t which=0);
// Loads sldat and increments ptr to evbuffer
   Int_t LoadSlot(THaSlotData *sldat,  const Int_t* evbuffer, const Int_t *pstop );  

   Int_t fNumTrig, fNumEvents, *fNumAInt, *fNumTInt,  *fNumSample;
   Int_t *fAdcData;  // Raw data (either samples or pulse integrals)
   Int_t *fTdcData;  
   Bool_t IsInit; 
   void Clear(const Option_t *opt);
   void CheckSetMode();
   void CheckFoundMode();
   static TypeIter_t fgThisType;
   Int_t slotmask, chanmask, datamask;
   ClassDef(Fadc250Module,0)  //  JLab FADC 250 Module

};

#endif

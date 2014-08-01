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
#include "ToyModuleX.h"

#define NADCCHAN   16
#define MAXDAT     50000

class Fadc250Module : public ToyModuleX {

public:

   Fadc250Module() {};  
   Fadc250Module(Int_t crate, Int_t slot);  
   virtual ~Fadc250Module();  

   Bool_t IsSlot(Int_t rdata);
   Int_t Decode(const Int_t *evbuffer);

   Int_t GetNumEvents() { return fNumEvent; };

   Int_t GetAdcData(Int_t chan, Int_t isample, Int_t ievent=0); 
   Int_t GetTdcData(Int_t chan, Int_t isample, Int_t ievent=0);  

   void SetMode(Int_t mode) { 
     f250_setmode = mode;
     CheckSetMode();
   }

// Loads sldat and increments ptr to evbuffer
   Int_t LoadSlot(THaSlotData *sldat,  const Int_t* evbuffer, Int_t pstop );  

private:
 
   enum { F250_SAMPLE = 0, F250_INTEG = 1 };
   Int_t f250_setmode, f250_foundmode;

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

   Int_t fTrigNum,  *fNumEvent,  *fNumSample;
   Float_t *fAdcData;  // Raw data (either samples or pulse integrals)
   Float_t *fTdcData; 
   void Clear(); 
   void CheckSetMode();
   void CheckFoundMode();
   static TypeIter_t fgThisType;
   Int_t slotmask, chanmask, datamask;
   ClassDef(Fadc250Module,0)  //  JLab FADC 250 Module

};

#endif

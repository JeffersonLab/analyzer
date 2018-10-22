/////////////////////////////////////////////////////////////////////
//
//   THaGenDetTest
//   General Detector Test of Decoder
//
//   THaGenDetTest is an illustrative example of how to use
//   the Hall A C++ data I/O and decoder classes for 
//   a detector class one might develop.   
//   This is also used to measure the baseline speed
//   of I/O and decoder.
//   This is a "throw away" test code.  
//
//   author  Robert Michaels  (rom@jlab.org)  
//
/////////////////////////////////////////////////////////////////////

#include "THaGenDetTest.h"
#include "THaEvData.h"

//#define SIMPLELOOP 

static const int PRINTOUT = 0;  // to printout (1) or not (0)

using namespace std;

THaGenDetTest::THaGenDetTest()
{
};

THaGenDetTest::~THaGenDetTest()
{
};

void THaGenDetTest::init()
{
// Set up my detector.  I have to know what crate, slots, and
// channels belong to my detector.  Here "my detector" is
// the scint and VDC on both arms.

       for (int i=0; i<MAX; i++) mycrates[i]=0;

  // E-arm Scintillators
       mydevice[0] = "E-arm S1 ADCs on Left PMTs";
       mycrates[0] = 1;        
       myslots[0] = 25;
       chanlo[0] = 0;  chanhi[0] = 5;
       mydevice[1] = "E-arm S1 ADCs on Right PMTs";
       mycrates[1] = 1;        
       myslots[1] = 25;
       chanlo[1] = 48;  chanhi[1] = 53;
       mydevice[2] = "E-arm S2 ADCs on Left PMTs";
       mycrates[2] = 1;        
       myslots[2] = 25;
       chanlo[2] = 6;  chanhi[2] = 11;
       mydevice[3] = "E-arm S2 ADCs on Right PMTs";
       mycrates[3] = 1;        
       myslots[3] = 25;
       chanlo[3] = 54;  chanhi[3] = 59;

       mydevice[4] = "E-arm S1 TDCs on Left PMTs";
       mycrates[4] = 1;        
       myslots[4] = 20;        
       chanlo[4] = 16;  chanhi[4] = 21;
       mydevice[5] = "E-arm S1 TDCs on Right PMTs";
       mycrates[5] = 1;        
       myslots[5] = 20;        
       chanlo[5] = 32;  chanhi[5] = 37;
       mydevice[6] = "E-arm S2 TDCs on Left PMTs";
       mycrates[6] = 1;        
       myslots[6] = 20;        
       chanlo[6] = 22;  chanhi[6] = 27;
       mydevice[7] = "E-arm S2 TDCs on Right PMTs";
       mycrates[7] = 1;        
       myslots[7] = 20;        
       chanlo[7] = 38;  chanhi[7] = 43;

  // H-arm Scintillators
       mydevice[8] = "H-arm S1 ADCs on Left PMTs";
       mycrates[8] = 2;        
       myslots[8] = 25;
       chanlo[8] = 0;  chanhi[8] = 5;
       mydevice[9] = "H-arm S1  ADCs on Right PMTs";
       mycrates[9] = 2;        
       myslots[9] = 25;
       chanlo[9] = 16;  chanhi[9] = 21;
       mydevice[10] = "H-arm S2 ADCs on Left PMTs";
       mycrates[10] = 2;        
       myslots[10] = 25;
       chanlo[10] = 6;  chanhi[10] = 11;
       mydevice[11] = "H-arm S2  ADCs on Right PMTs";
       mycrates[11] = 2;        
       myslots[11] = 25;
       chanlo[11] = 22;  chanhi[11] = 27;

       mydevice[12] = "H-arm S1 TDCs on Left PMTs";
       mycrates[12] = 2;        
       myslots[12] = 23;
       chanlo[12] = 0;  chanhi[12] = 5;
       mydevice[13] = "H-arm S1 TDCs on Right PMTs";
       mycrates[13] = 2;        
       myslots[13] = 23;
       chanlo[13] = 16;  chanhi[13] = 21;
       mydevice[14] = "H-arm S2 TDCs on Left PMTs";
       mycrates[14] = 2;        
       myslots[14] = 23;
       chanlo[14] = 6;  chanhi[14] = 11;
       mydevice[15] = "H-arm S2 TDCs on Right PMTs";
       mycrates[15] = 2;        
       myslots[15] = 23;
       chanlo[15] = 22;  chanhi[15] = 27;
      
  // E-arm VDCs
       for (int i=0; i<16; i++) {
           mydevice[i+16] = "E-arm VDCs";
           mycrates[i+16] = 1;
           myslots[i+16] = i+4;
           chanlo[i+16] = 0;
           chanhi[i+16] = 95;
           if (((i+1)%4) == 0) chanhi[i+16]=79;
       }

  // H-arm VDCs
       for (int i=0; i<16; i++) {
           mydevice[i+33] = "H-arm VDCs";
           mycrates[i+33] = 2;
           myslots[i+33] = i+6;
           chanlo[i+33] = 0;
           chanhi[i+33] = 95;
           if (((i+1)%4) == 0) chanhi[i+33]=79;
       }

       //       mycrates[50] = 14;

       
  
};

void THaGenDetTest::process_event(THaEvData *evdata)
{
  if (PRINTOUT) {
    cout << "\n\n------------------------------------------------"<<endl;
    cout << "Event type " << dec << evdata->GetEvType() << endl;
    cout << "Event length " << evdata->GetEvLength() << endl;
    cout << "Event number " << evdata->GetEvNum() << endl;
    cout << "Run number " << evdata->GetRunNum() << endl;
    if (evdata->IsPhysicsTrigger()) cout << "Physics trigger" << endl;
    if (evdata->IsScalerEvent()) cout << "Scaler event" << endl;
    if (evdata->IsEpicsEvent()) cout << "Epics data" << endl;
    if (evdata->IsPrescaleEvent()) {
        cout << "Prescale data: \n  Trig    Prescale factor" << endl;
        for (int trig = 1; trig<=8; trig++) {
	    cout << dec << "   " << trig << "       ";
            cout << evdata->GetPrescaleFactor(trig) << endl;
	}
    }
    if (evdata->IsScalerEvent()) {
     cout << "Scaler data. (it will remain static until";
     cout << " next scaler event): "<<endl;
     for (int sca = 0; sca < 3; sca++) { // 1st 3 scaler banks, left spectrom.
       for (int cha = 0; cha < 16; cha++)  {
        cout << "Scaler " << dec << sca << " channel " << cha;
        cout << "  data = (decimal) " << evdata->GetScaler("left",sca,cha);
        cout << "  data = (hex) "<<hex<<evdata->GetScaler("left",sca,cha)<<endl;
       }
     }
    }
  }
  if (evdata->IsEpicsEvent()) {
    if (PRINTOUT) {
      cout << "BPM 3A, X   IPM1H03A.XPOS  =  ";
      cout << evdata->GetEpicsData("IPM1H03A.XPOS")<<endl;
      cout << "BPM 3A, Y   IPM1H03A.YPOS  =  ";
      cout << evdata->GetEpicsData("IPM1H03A.YPOS")<<endl;
      cout << "BPM 3B, X   IPM1H03B.XPOS  =  ";
      cout << evdata->GetEpicsData("IPM1H03B.XPOS")<<endl;
      cout << "BPM 3B, Y   IPM1H03B.YPOS  =  ";
      cout << evdata->GetEpicsData("IPM1H03B.YPOS")<<endl;
      cout << "Avg of 2 BCM,   hac_bcm_average = ";
      cout << evdata->GetEpicsData("hac_bcm_average")<<endl;
      cout << "Energy   halla_MeV  =  ";
      cout << evdata->GetEpicsData("halla_MeV")<<endl; 
    }
   }
  if (!evdata->IsPhysicsTrigger()) return;
  if (PRINTOUT) {
  int crate = 16;  
  cout << "--------  Test of RICH raw data  ---------"<<endl;
  for (int slot = 1; slot <= 16; slot++) {  // Actually ADCs (2 ADCs per 8 VME slots)
    cout << "Slot "<<slot<<endl;
    for (int chan = 0; chan < 500; chan++) {    // channel numbers start at 0
     cout<<"channel "<<chan<<" num hits "<<evdata->GetNumHits(crate,slot,chan)<<endl;
     for (int hit = 0; hit < evdata->GetNumHits(crate,slot,chan); hit++) {
        cout << "  ADC data = "<<evdata->GetData(crate,slot,chan,hit)<<endl;
     }
    }
  }
  }
  unsigned long sum = 0;
  for (int j = 0; j < MAX; j++) {
      if (!mycrates[j]) continue;
      int crate = mycrates[j];
      int slot = myslots[j];
      //      if (PRINTOUT) evdata->PrintSlotData(crate,slot);
      bool firstone = true;

// Two ways to retrieve the data: 1) simple loop over all channels,
// and 2) loop over list of unique channels which were hit (faster). 

#ifdef SIMPLELOOP
        for (int chan = chanlo[j]; chan <= chanhi[j]; chan++) {
#else
	for (int list = 0; list < evdata->GetNumChan(crate,slot); list++) {
          int chan = evdata->GetNextChan(crate,slot,list);       
#endif
          if (( chan > chanhi[j]) || (chan < chanlo[j])) continue;
          for (int hit = 0; hit < evdata->GetNumHits(crate,slot,chan); hit++) {
            int data = evdata->GetData(crate,slot,chan,hit);
	    if (PRINTOUT) {
              if(firstone) {
		cout << "\n Data in " << mydevice[j]<<"     / ";
                 cout << "crate "<<dec<<crate<<" /  slot "<<slot<<endl;  
                 firstone = false;
	      }
	      cout << "channel " << dec << chan << "  hit # " << hit;
              cout << "   data (decimal) = " << data;
              cout << "   data (hex) = " << hex << data << endl;
	    }
            if (sum < 200000000) sum += data;   // Do something with data...
	  }
        }
  }
  return;
};  

    




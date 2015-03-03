#ifndef THaScaler_
#define THaScaler_

/////////////////////////////////////////////////////////////////////
//
//   THaScaler
//
//   Scaler Data in Hall A at Jlab.
//
//   See comments in implementation.
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#define SCAL_NUMCHAN     32
#define SCAL_NUMBANK     12
#define SCAL_EVTYPE     140
#define SCAL_ERROR       -1
#define SCAL_VERBOSE      1  // verbose warnings (0 = silent, recommend = 1)

#include "TObject.h"
#include "TString.h"
#include "Decoder.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <time.h>

class Bdate;
class THaScalerDB;
class THaEvData;
class TDatime;

class THaScaler : public TObject {
public:

// Constructors.
// 'Bankgroup' is collection of scalers, "Left"(L-arm), "Right"(R-arm)
//  "EvLeft" = event stream, Left HRS,  "EvRight" = event stream, Right HRS.

   THaScaler( const char* Bankgroup );
   virtual ~THaScaler();

// Init() is *REQUIRED* to be run once in the life of this object.
// 'Time' is the Day-Month-Year to look up scaler map related to data taking.
// European format of Time: '21-05-1999' --> 21st of May 1999.
// 'Time' can also be "now"
   Int_t Init( const char* date = "now" ); // default time = "now"
   Int_t Init( const TDatime& date );      // default time = current time

// Various ways of loading data into this object, each load is for one
// 'event' of data.  Any CODA files are opened once, read sequentially
   Int_t LoadData(const THaEvData& evdata); // load from THaEvData (return if non-scaler evt)
// from CODA file (assumed opened already)
   Int_t LoadDataCodaFile(Decoder::THaCodaFile *codafile);
// from CODA file 'filename' (we'll open it for you)
   Int_t LoadDataCodaFile(const char* filename);
   Int_t LoadDataCodaFile(TString filename);
// Load data from scaler history file for run number
   Int_t LoadDataHistoryFile(int run_num, int hdeci=0); // default file "scaler_history.dat"
   Int_t LoadDataHistoryFile(const char* filename, int run_num, int hdeci=0);
// Online 'server' may be Name or IP of VME cpu, or
// 'server' may also be a mnemonic like "Left", "Right", etc
   Int_t LoadDataOnline();    // server and port is known for 'Bankgroup'
   Int_t LoadDataOnline(const char* server, int port);

   virtual void Print( Option_t* opt="" ) const;   // Prints data contents
   virtual void PrintSummary();  // Print out a summary of important scalers.

   const char* GetName() const { return bankgroup.c_str(); };
   Int_t GetCrate() const { return crate; };

// Get scaler data from slot #slot and channel #chan (slot >= 0, chan >= 0)
// Get counts by history, histor = 1 = previous event, 0 = present.
   Int_t GetScaler(Int_t slot, Int_t chan, Int_t histor=0);

// Get accumulated COUNTS on detector and channel
// detector = "s1", "s2", "gasC", "a1", "a2", "leadgl", "edtm"
   Int_t GetScaler(const char* detector, Int_t chan);  // if no L/R distinction.
// PMT = "left", "right", "LR" (LR means left.AND.right)
   Int_t GetScaler(const char* detector, const char* PMT, Int_t chan, Int_t histor=0);
   Int_t GetTrig(Int_t trigger);   // counts for trig# 1,2,3,4,5,etc
// Beam current, which= 'bcm_u1','bcm_d1','bcm_u3','bcm_d3','bcm_u10','bcm_d10'
   Int_t GetBcm(const char* which);
   Int_t GetPulser(const char* which);  // which = 'clock', 'edt', 'edtat', 'strobe'
// Accumulated COUNTS by helicity state (-1, 0, +1),  0 is non-helicity gated
// Get counts by history, histor = 1 = previous event, 0 = present.
   Int_t GetTrig(Int_t helicity, Int_t trigger, Int_t histor=0);
   Int_t GetBcm(Int_t helicity, const char* which, Int_t histor=0);
   Int_t GetPulser(Int_t helicity, const char* which, Int_t histor=0);
   Int_t GetNormData(Int_t tgtstate, Int_t helicity, const char* which, Int_t histor=0);
   Int_t GetNormData(Int_t helicity, const char* which, Int_t histor=0);
   Int_t GetNormData(Int_t tgtstate, Int_t helicity, Int_t chan, Int_t histor);
   Int_t GetNormData(Int_t helicity, Int_t chan, Int_t histor=0);
// RATES (Hz) since last update, similar usage to above.
   Double_t GetScalerRate(Int_t slot, Int_t chan);
   Double_t GetScalerRate(const char* plane, const char* PMT, Int_t chan);
   Double_t GetScalerRate(const char* plane, Int_t chan);
   Double_t GetTrigRate(Int_t trigger);
   Double_t GetTrigRate(Int_t helicity, Int_t trigger);
   Double_t GetBcmRate(const char* which);
   Double_t GetBcmRate(Int_t helicity, const char* which);
   Double_t GetPulserRate(const char* which);
   Double_t GetPulserRate(Int_t helicity, const char* which);
   Double_t GetNormRate(Int_t tgtstate, Int_t helicity, const char* which);
   Double_t GetNormRate(Int_t helicity, const char* which);
   Double_t GetNormRate(Int_t helicity, Int_t chan);
   Double_t GetNormRate(Int_t tgtstate, Int_t helicity, Int_t chan);
   Double_t GetClockRate() const { return clockrate; }
   Double_t GetIRate(Int_t slot, Int_t chan);
   Bool_t IsRenewed() const { return new_load; }  // kTRUE if obj has new data
   void SetIpAddress(std::string ipaddress);  // set IP for online data
   void SetPort(Int_t port);                    // set PORT# for online data
   void SetClockLoc(Int_t slot, Int_t chan);  // set clock location
   void SetClockRate(Double_t clkrate);       // set clock rate
   void SetTimeInterval(Double_t time); // set avg time interval between events. (if no clk)
   void SetIChan(Int_t slot, Int_t chan);  // Set channel to norm. by (for GetIRate)
   Int_t SetDebug( Int_t level );          // Set debug level
   Int_t GetSlot(std::string which, Int_t helicity=0);
   Int_t GetSlot(Int_t tgtstate, Int_t helicity);
   Int_t GetChan(std::string which, Int_t helicity=0, Int_t chan=0);
   THaScalerDB* GetDataBase() { return database; };

protected:

   std::string bankgroup;
   Decoder::THaCodaFile *fcodafile;
   std::vector<Int_t> onlmap;
   THaScalerDB *database;
   std::multimap<std::string, Int_t> normmap;
   Int_t *rawdata;
   Bool_t coda_open;
   UInt_t header;
   Int_t crate, evstr_type;
   std::string vme_server;
   int vme_port, clkslot, clkchan;
   int icurslot, icurchan;
   Bool_t found_crate,first_loop;
   Bool_t did_init, new_load, one_load, use_clock, isclockreset;
   Int_t *normslot;
   Double_t clockrate;
   Int_t fDebug;

   Int_t InitData(const std::string& bankgroup, const Bdate& bd);
   Int_t CheckInit();
   void Clear(Option_t* opt="");
   void ClearAll();
   void LoadPrevious();
   Int_t ExtractRaw(const UInt_t* data, int len=0);
   void DumpRaw(Int_t flag=0);
   UInt_t header_str_to_base16(const std::string& header);
   Double_t calib_u1,calib_u3,calib_u10,calib_d1,calib_d3,calib_d10;
   Double_t off_u1,off_u3,off_u10,off_d1,off_d3,off_d10;
   Double_t GetTimeDiff(Int_t tgtstate, Int_t helicity);
   Double_t GetTimeDiff(Int_t helicity);
   Double_t GetTimeDiffSlot(Int_t slot, Int_t chan=7);
   void SetupNormMap();

private:

   THaScaler();
   THaScaler(const THaScaler &bk);
   THaScaler& operator=(const THaScaler &bk);

 public:
   ClassDef(THaScaler,0)
};

#endif

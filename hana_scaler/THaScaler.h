#ifndef THaScaler_
#define THaScaler_


/////////////////////////////////////////////////////////////////////
//
//   THaScaler
//
//   Scaler Data in Hall A at Jlab.  
//  
//   The usage covers several implementations:
//
//   1. Works within context of full analyzer, or standalone.
//   2. Time dependent channel mapping to account for movement
//      of channels or addition of new channels.
//   3. Source of data is either THaEvData, CODA file, online,
//      or scaler-history-file, depending on context.
//   4. Optional displays of rates, counts, history.
//   5. Derived classes may be written to determine derived
//      quantities like charge, deadtime, and helicity correlations.
//      Examples provided separately.
//
//   Terminology:
//      bankgroup = group of scaler banks, e.g. Left Spectrometer crate.
//      bank  = group of scalers, e.g. S1L PMTs.
//      normalization scaler =  bank assoc. with normalization (charge, etc)
//      slot  = slot of scaler channels (1 module in VME)
//      channel = individual channel of scaler data
//
//    Procedure to add a new scaler channel or bank:
//      1. Imitate, e.g. 'edtm'
//      2. Constructor to create a new THaScalerBank and call AddBank.
//      3. However, if the scaler is a subset of the normalization
//         scaler it must be treated differently:
//         a) Nothing to THaScaler
//         b) Add to the 'chan_name' vector in C'tor of THaNormScaler
//      4. Corresponding changes to destructors, and declaration in headers.
//      5. Add appropriate line to GetScaler(char* detector...) method
//         if not normalization scaler.
//      6. Add the line for the channel or bank to 'scaler.map' file
//         with same short name as used in C'tor.
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#define SCAL_NUMBANK     12
#define SCAL_CLOCKRATE 1024
#define SCAL_EVTYPE     140
#define SCAL_ERROR       -1
#define SCAL_VERBOSE      1  // verbose warnings (0 = silent, recommend = 1)

// VME scaler servers 
#define SERVER_LEFT  "129.57.164.11"   /* halladaq4.jlab.org */
#define SERVER_RIGHT "129.57.164.8"    /* halladaq1.jlab.org */
#define SERVER_RCS   "192.168.2.3"     /* ts2 crate in RCS setup */
/* Port number of VME servers */
#define PORT_LEFT  5022
#define PORT_RIGHT 5021
#define PORT_RCS   5022
#define MAXBLK   20
#define MSGSIZE  50
/* structure for requests from this client to HRS VME server */
struct request { 
   int reply;
   long ibuf[16*MAXBLK]; 
   char message[MSGSIZE];
   int clearflag; int checkend;
};
/* But RCS is different... */
#define RCSMAXBLK 10
#define RCSMSG 1024
struct rcsrequest { 
   int reply;
   char message[RCSMSG];
};
struct rcsreply {
  long ibuf[16*RCSMAXBLK];
  int  endrunflag;
  char message[RCSMSG];
};
#define SERVER_WORK_PRIORITY 100
#define SERVER_STACK_SIZE 10000
#define MAX_QUEUED_CONNECTIONS 5
#define SOCK_ADDR_SIZE  sizeof(struct sockaddr_in))
#define REPLY_MSG_SIZE 1000

#include "TObject.h"
#include "TString.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <time.h>

class THaNormScaler;
class THaScalerBank;
class THaScalerDB;
class BscaLoc;
class THaCodaFile;
class THaEvData;

class TDatime;

class THaScaler : public TObject {
public:

// Constructors.
// 'Bankgroup' is collection of scalers, "Left"(L-arm), "Right"(R-arm), "RCS"

//   THaScaler();
   THaScaler( const char* Bankgroup );
   virtual ~THaScaler();

// Init() is *REQUIRED* to be run once in the life of this object.
// 'Time' is the Day-Month-Year to look up scaler map related to data taking.
// European format of Time: '21-05-1999' --> 21st of May 1999.
// 'Time' can also be "now"
//   Int_t Init( const char* Bankgroup ); // default time = "now"
//   Int_t Init( const char* Time, const char* Bankgroup ); 
   Int_t Init( const char* date = "now" ); // default time = "now"
   Int_t Init( const TDatime& date );      // default time = current time

// Various ways of loading data into this object, each load is for one
// 'event' of data.  Any CODA files are opened once, read sequentially
   Int_t LoadData(const THaEvData& evdata); // load from THaEvData (return if non-scaler evt)
// from CODA file (assumed opened already)
   Int_t LoadDataCodaFile(THaCodaFile *codafile);  
// from CODA file 'filename' (we'll open it for you)
   Int_t LoadDataCodaFile(const char* filename);      
   Int_t LoadDataCodaFile(TString filename);      
// Load data from scaler history file for run number
   Int_t LoadDataHistoryFile(int run_num); // default file "scaler_history.dat"
   Int_t LoadDataHistoryFile(const char* filename, int run_num);
// Online 'server' may be Name or IP of VME cpu, or
// 'server' may also be a mnemonic like "Left", "Right", "RCS", etc
   Int_t LoadDataOnline();    // server and port is known for 'Bankgroup'
   Int_t LoadDataOnline(const char* server, int port); 

   virtual Int_t InitPlots();    // Initialize plots (xscaler style)
   virtual void Print( Option_t* opt="" ) const;   // Prints data contents
   virtual void PrintSummary();  // Print out a summary of important scalers.

   const char* GetName() const { return bankgroup.c_str(); }

// Get scaler data from slot #slot and channel #chan (slot >= 0, chan >= 0)
// Get counts by history, histor = 1 = previous event, 0 = present.
   Int_t GetScaler(Int_t bank, Int_t chan, Int_t histor=0);
// Get accumulated COUNTS on detector and channel
// detector = "s1", "s2", "gasC", "a1", "a2", "leadgl", "rcs1-3", "edtm" 
   Int_t GetScaler(const char* detector, Int_t chan);  // if no L/R distinction.
// PMT = "left", "right", "LR" (LR means left.AND.right)
   Int_t GetScaler(const char* detector, const char* PMT, Int_t chan, 
     Int_t histor=0);  
   Int_t GetTrig(Int_t trigger);   // counts for trig# 1,2,3,4,5,etc
// Beam current, which= 'bcm_u1','bcm_d1','bcm_u3','bcm_d3','bcm_u10','bcm_d10'
   Int_t GetBcm(const char* which); 
   Int_t GetPulser(const char* which);  // which = 'clock', 'edt', 'edtat', 'strobe'
// Accumulated COUNTS by helicity state (-1, 0, +1),  0 is non-helicity gated
// Get counts by history, histor = 1 = previous event, 0 = present.
   Int_t GetTrig(Int_t helicity, Int_t trigger, Int_t histor=0);   
   Int_t GetBcm(Int_t helicity, const char* which, Int_t histor=0);   
   Int_t GetPulser(Int_t helicity, const char* which, Int_t histor=0);  
   Int_t GetNormData(Int_t helicity, const char* which, Int_t histor=0);  
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
   Double_t GetNormRate(Int_t helicity, const char* which);
   Double_t GetNormRate(Int_t helicity, Int_t chan);
   
protected:

   Int_t InitMap(string bankgroup);
   map < string, THaScalerBank* > scalerbankmap;
   vector < THaScalerBank* > scalerbanks;
   THaScalerBank *s1L,*s2L,*s1R,*s2R,*s1,*s2;
   THaScalerBank *gasC,*a1L,*a2L,*a1R,*a2R;
   THaScalerBank *s0,*misc1,*misc2,*misc3,*misc4;
   THaScalerBank *leadgl,*rcs1,*rcs2,*rcs3,*edtm;
   THaNormScaler *nplus,*nminus,*norm;
   multimap< string, BscaLoc > bmap;
   THaScalerDB *database;
   void AddBank(THaScalerBank *bk);
   string bankgroup;
   THaCodaFile *fcodafile;
   Bool_t coda_open;
   UInt_t header_left, header_right, header_rcs;
   Int_t cratenum_left, cratenum_right, cratenum_rcs;
   Int_t header, crate;
   string vme_server;
   int vme_port;
   Bool_t found_crate,first_loop;
   Bool_t did_init;
   Double_t clockrate;
   Int_t CheckInit();
   void Clear(Option_t* opt="");
   void ClearAll();
   void LoadPrevious();
   Int_t ExtractRaw(int* data);
   Int_t Load();
   Int_t *rawdata;
   UInt_t header_str_to_base16(string header);
   Double_t calib_u1,calib_u3,calib_u10,calib_d1,calib_d3,calib_d10;
   Double_t off_u1,off_u3,off_u10,off_d1,off_d3,off_d10;
   Double_t GetTimeDiff(Int_t helicity);

private:

   THaScaler();
   THaScaler(const THaScaler &bk);
   THaScaler& operator=(const THaScaler &bk);

#ifndef ROOTPRE3
ClassDef(THaScaler,0)  // Scaler data
#endif

};

#endif

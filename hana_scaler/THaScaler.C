////////////////////////////////////////////////////////////////////
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
//         a) Nothing to  THaScaler
//         b) Add to the 'chan_name' vector in C'tor of THaNormScaler
//      4. Corresponding changes to destructors, and declaration in headers.
//      5. Add appropriate line to GetScaler(const char* detector...) method
//         if not normalization scaler.
//      6. Add the line for the channel or bank to 'scaler.map' file
//         with same short name as used in C'tor.
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaScaler.h"
#include "THaScalerBank.h"
#include "THaNormScaler.h"
#include "THaScalerDB.h"
#include "THaCodaFile.h"
#include "THaEvData.h"
#include "TDatime.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>

#include <iostream>

#ifndef ROOTPRE3
ClassImp(THaScaler)
#endif

THaScaler::THaScaler() {}

THaScaler::THaScaler( const char* bankgr ) {
// Set up the scaler banks.  Each bank is a group of related scalers.
// The arg (string like "s1R") must match the scaler.map file used
// by database THaScalerDB.
// 'bankgr' is group of scaler banks, "Left"(L-arm), "Right"(R-arm), "RCS"
  if( !bankgr || strlen(bankgr) == 0 ) {
    MakeZombie();
    return;
  }
  bankgroup = bankgr;
  did_init = kFALSE;
  coda_open = kFALSE;
  fcodafile = new THaCodaFile;
  rawdata = new Int_t[2*SCAL_NUMBANK*SCAL_NUMCHAN];
  memset(rawdata,0,2*SCAL_NUMBANK*SCAL_NUMCHAN*sizeof(Int_t));
  s1L     = new THaScalerBank("s1L");    AddBank(s1L);
  s2L     = new THaScalerBank("s2L");    AddBank(s2L);
  s1R     = new THaScalerBank("s1R");    AddBank(s1R);
  s2R     = new THaScalerBank("s2R");    AddBank(s2R);
  s0      = new THaScalerBank("s0");     AddBank(s0);
  s1      = new THaScalerBank("s1");     AddBank(s1);
  s2      = new THaScalerBank("s2");     AddBank(s2);
  misc1   = new THaScalerBank("misc1");  AddBank(misc1);
  misc2   = new THaScalerBank("misc2");  AddBank(misc2);  
  misc3   = new THaScalerBank("misc3");  AddBank(misc3);
  misc4   = new THaScalerBank("misc4");  AddBank(misc4);   
  gasC    = new THaScalerBank("gasC");   AddBank(gasC);
  a1L     = new THaScalerBank("a1L");    AddBank(a1L);
  a1R     = new THaScalerBank("a1R");    AddBank(a1R);
  a2L     = new THaScalerBank("a2L");    AddBank(a2L);
  a2R     = new THaScalerBank("a2R");    AddBank(a2R);
  leadgl  = new THaScalerBank("leadgl"); AddBank(leadgl);
  rcs1    = new THaScalerBank("rcs1");   AddBank(rcs1);
  rcs2    = new THaScalerBank("rcs2");   AddBank(rcs2);
  rcs3    = new THaScalerBank("rcs3");   AddBank(rcs3);
  edtm    = new THaScalerBank("edtm");   AddBank(edtm);
  nplus   = new THaNormScaler("nplus");  AddBank(nplus);
  nminus  = new THaNormScaler("nminus"); AddBank(nminus);
  norm    = new THaNormScaler("norm");   AddBank(norm);
  clockrate = SCAL_CLOCKRATE;
};

THaScaler::~THaScaler() {
  delete [] rawdata;
  delete s1L;
  delete s2L;
  delete s1R;
  delete s2R;
  delete s0;
  delete s1;
  delete s2;
  delete misc1;
  delete misc2;
  delete misc3;
  delete misc4;
  delete gasC;
  delete a1L;
  delete a2L;
  delete a1R;
  delete a2R;
  delete leadgl;
  delete rcs1;
  delete rcs2;
  delete rcs3;
  delete edtm;
  delete nplus;
  delete nminus;
  delete norm;  
  delete fcodafile;
};

void THaScaler::AddBank(THaScalerBank *bk) {
  //   pair <string, THaScalerBank*> psc;
  //   psc.first = bk->name;  psc.second = bk;
  typedef map<string, THaScalerBank* >::value_type valType;
  scalerbankmap.insert(valType(bk->name,bk));
  scalerbanks.push_back(bk);
};

Int_t THaScaler::Init( const TDatime& time ) 
{
  // Initialize scalers for given date/time.
  // Accuracy is 1 day. Only date is used, time is ignored.

  Int_t i = time.GetDate();
  char date[11];
  sprintf( date, "%2.2d-%2.2d-%4.4d",i%100,(i%10000-i%100)/100,i/10000 );
  return Init( date );
};

//Int_t THaScaler::Init(const char* bankgr) {
//    return Init("now",bankgr);
//};

Int_t THaScaler::Init(const char* thetime ) 
{
  // Init() is required to be run once in the life of object.
  // 'thetime' is the Day-Month-Year to look up scaler map related to data.
  // e.g. thetime = '21-05-1999' --> 21st of May 1999.
  // 'thetime' can also be "now"

  string strtime(thetime);

  int year, month, day;
  if (strtime == "now") {
    time_t t1 = time(0);
    struct tm* btm = gmtime(&t1);
    year =  1900 + btm->tm_year;  
    month = 1 + btm->tm_mon;  // Jan,Feb,Mar = 1,2,3...
    day = btm->tm_mday;
  } else {
    int pos1 = strtime.find_first_of("-");
    int pos2 = strtime.find_last_of("-");
    if (pos1 > 0 && pos2 > pos1) {
      string sday= strtime.substr(0,pos1);
      string smonth = strtime.substr(pos1+1,pos2-pos1-1);
      string syear = strtime.substr(pos2+1,strtime.size()-pos2);
      day = atoi(sday.c_str());
      month = atoi(smonth.c_str());
      year = atoi(syear.c_str());
    } else {
      return SCAL_ERROR;
    }
  }

  Bdate date_want(day,month,year);    // date when we want data.

// date of detector swap (Hall A specfic, assumed never to repeat)
  Bdate dswap(9,15,2000);   

  if (date_want < dswap) {
    header_left = 0xceb00000;
    header_right = 0xabc00000;
    cratenum_left = 7;        // 7 always goes with 0xceb
    cratenum_right = 8;       // 8 always goes with 0xabc
  } else {
    header_left = 0xabc00000;
    header_right = 0xceb00000;
    cratenum_left = 8;
    cratenum_right = 7;
  }
  header_rcs = 0xbbc00000;
  cratenum_rcs = 9;

  database = new THaScalerDB();

  if ( !database->extract_db(date_want, bmap) ) return SCAL_ERROR;

  int status;
  status = InitMap(bankgroup);
  if (status == SCAL_ERROR) return SCAL_ERROR;

// Calibration of BCMs
  calib_u1 = 1345;  calib_u3 = 4114;  calib_u10 = 12515; 
  calib_d1 = 1303;  calib_d3 = 4034;  calib_d10 = 12728; 
  off_u1 = 92.07;   off_u3 = 167.06;  off_u10 = 102.62;  
  off_d1 = 72.19;   off_d3 = 81.08;   off_d10 = 199.51;

  did_init = kTRUE;
    
  return 0;
};

Int_t THaScaler::InitMap(string bankgrp) {
// Initialize the map between data and channels

  crate = -1;
  if ( bankgrp == "Left" || bankgrp == "left" || bankgrp == "L" ) {
    crate = cratenum_left; 
    header = header_left;
    vme_server = SERVER_LEFT;
    vme_port = PORT_LEFT;
  }
  if ( bankgrp == "Right" || bankgrp == "right" || bankgrp == "R" ) {
    crate = cratenum_right; 
    header = header_right;
    vme_server = SERVER_RIGHT;
    vme_port = PORT_RIGHT;
  }
// Want to add another scaler bank ?  Imitate 'rcs'
  if (bankgrp == "RCS" || bankgrp == "rcs" ) {
    crate = cratenum_rcs; 
    header = header_rcs;
    vme_server = SERVER_RCS;
    vme_port = PORT_RCS;
    clockrate = 10000;
  }

  if (crate == -1) {
    if (SCAL_VERBOSE) {
      cout << "THaScaler:: Warning: Undefined crate"<<endl;
      cout << "Need to Init for 'Left', 'Right', or 'RCS' crate"<<endl;
    }
    return SCAL_ERROR;
  }

  for (vector<THaScalerBank*>::iterator p = scalerbanks.begin();
       p != scalerbanks.end(); p++) {
    (*p)->Init(crate,bmap);
  }

  return 0;
};

Int_t THaScaler::InitPlots() {
// See the class THaScalerGui which inherits from this one.
  return 0;
};

Int_t THaScaler::LoadData(const THaEvData& evdata) {
// Load data from THaEvData object.  Return of 0 is ok.
  if (CheckInit() == SCAL_ERROR) return SCAL_ERROR;
  if ( !evdata.IsScalerEvent() ) return 0;
  LoadPrevious();
  Clear();
  for (int slot = 0; slot < SCAL_NUMBANK; slot++) {
     for (int chan = 0; chan < SCAL_NUMCHAN; chan++) {
        int k = slot*SCAL_NUMCHAN + chan;
        rawdata[k] = evdata.GetScaler(crate, slot, chan);
     }
  }       
  Load();
  return 0;
};

Int_t THaScaler::LoadDataCodaFile(const char* filename) { 
// from CODA file 'filename'.  We'll open it for you. Return of 0 is ok.
  TString file(filename);
  return LoadDataCodaFile(file);
};

Int_t THaScaler::LoadDataCodaFile(TString filename) { 
// from CODA file 'filename'.  We'll open it for you. Return of 0 is ok.
  if (CheckInit() == SCAL_ERROR) return SCAL_ERROR;
  if ( !coda_open ) {
     fcodafile->codaOpen(filename);
     coda_open = kTRUE;
  } 
  return LoadDataCodaFile(fcodafile);
};

Int_t THaScaler::LoadDataCodaFile(THaCodaFile *codafile) {
// Load data from a CODA file, assumed to be already opened. 
// Return of 0 means end of data, return of 1 means there is more data.
  if (CheckInit() == SCAL_ERROR) return SCAL_ERROR;
  int codastat, extstat;
  found_crate = kFALSE;
  first_loop = kTRUE;
  codastat = 0;
  while (codastat == 0) {
    codastat = codafile->codaRead();  
    int *data = codafile->getEvBuffer();
    int evtype = data[1]>>16;
    if (evtype == SCAL_EVTYPE) {
      extstat = ExtractRaw(data);
      if (extstat) return 1;
    }
  }
  return 0;
};

Int_t THaScaler::ExtractRaw(int* data) {
// Extract rawdata from data if this event belongs to this scaler crate.
// This works for CODA event data or VME data format but not 
// for scaler history file (strings).
  int len, ndat, max, i, j, k, slot, numchan;
  len  = data[0] + 1;
  max = fcodafile->getBuffSize();
  ndat = len < max ? len : max;
  for (i = 0; i < ndat; i++) {
    if (((data[i]&0xfff00000) == (unsigned long)header) &&
    ((data[i]&0x0000ff00) == 0) ) { // found this crate's data
      if (first_loop) {
          first_loop = kFALSE;
          LoadPrevious();
          Clear();
      }
      slot = (data[i]&0xf0000)>>16;
      numchan = (data[i]&0xff);
      for (j = i+1; j < i+numchan+1; j++) {
         k = slot*SCAL_NUMCHAN + j-i-1;
         if (k >= 0 && k < SCAL_NUMBANK*SCAL_NUMCHAN) rawdata[k] = data[j];
      }
      found_crate = kTRUE;
    }  
  }
  if (found_crate) {
      Load();
      return 1;
  }
  return 0;
};  

Int_t THaScaler::LoadDataHistoryFile(int run_num) {
// Load data from scaler history file for run number
  return LoadDataHistoryFile("scaler_history.dat", run_num);
};

Int_t THaScaler::LoadDataHistoryFile(const char* filename, int run_num) {
// Load data from scaler history file 'filename' for run number run_num
  if (CheckInit() == SCAL_ERROR) return SCAL_ERROR;
  ClearAll();
  ifstream hfile(filename);
  if ( !hfile ) {
    cout << "ERROR: THaScaler:  Scaler history file "<<filename;
    cout << " does not exist."<<endl;
    cout << "Hence, no data."<<endl;
    return SCAL_ERROR;
  }
  char *crun_num; crun_num = new char[20]; 
  sprintf(crun_num," %d",run_num);
  string runstr("run number");
  string myrun = runstr + crun_num;
  string sinput,dat;
  int foundrun = -1;
  while (getline(hfile, sinput)) {
    foundrun = sinput.find(myrun,0);
    if (foundrun >= 0) {
      while (getline(hfile, dat)) {
        int nextrun = dat.find(runstr,0);
        if (nextrun >= 0) goto decoder;
	UInt_t htst = header_str_to_base16(dat);
        if ((htst&0xfff00000) == (unsigned long)header) {
          int slot = (htst&0xf0000)>>16;
	  int numchan = (htst&0xff);
          for (int j = 0; j < numchan; j++) {
            getline(hfile, dat);
	    int k = slot*SCAL_NUMCHAN + j;
            if (k >= 0 && k < SCAL_NUMBANK*SCAL_NUMCHAN) {
 	      rawdata[k] = atoi(dat.c_str());
	    }
	  }
	}
      }
    }
  }
  if (foundrun < 0 && SCAL_VERBOSE == 1) {
    cout << "WARNING: THaScaler: Did not find run "<<run_num<<endl;
    cout << "in scaler history file"<<endl<<"Hence, no data."<<endl;
    return SCAL_ERROR;
  }
decoder:
  Load();
  delete [] crun_num;
  return 0;
};

Int_t THaScaler::LoadDataOnline() {
// Load data from online VME server and port for this 'Bank Group'
  return LoadDataOnline(vme_server.c_str(), vme_port);
};

Int_t THaScaler::LoadDataOnline(const char* server, int port) {
// Load data from VME 'server' and 'port'.
  if (CheckInit() == SCAL_ERROR) return SCAL_ERROR;

  int   sFd;      //  socket file descriptor 
  int i, k, slot, nchan, ntot, sca;
  struct request myRequest;           //  request to send to server 
  struct request vmeReply;            //  reply from server
  struct rcsrequest rcsRequest;
  struct rcsreply rcsReply;
  struct sockaddr_in serverSockAddr;  //  socket addr of server
  static int lprint   = 0;
  myRequest.clearflag = 0;
  myRequest.checkend  = 0;

// create socket 
  if ((sFd = socket (PF_INET, SOCK_STREAM, 0)) == -1 ) {
      cout << "ERROR: THaScaler: LoadDataOnline: Cannot open socket"<<endl;
      return SCAL_ERROR;
  }
  serverSockAddr.sin_family = PF_INET;
  serverSockAddr.sin_port = htons(port);
  serverSockAddr.sin_addr.s_addr = inet_addr(server);

// connect to server 
  if(connect (sFd, (struct sockaddr *) &serverSockAddr, sizeof(serverSockAddr)) == -1) {
      cout << "ERROR: THaScaler: LoadDataOnline: Cannot connect "<<endl;
      cout << "to VME server"<<endl;
      return SCAL_ERROR;
  }

// send request to server, read reply
  if ( crate != cratenum_rcs ) { 
    myRequest.reply = 1;
    if(write(sFd, (char *)&myRequest, sizeof(myRequest)) == -1) {
       cout << "ERROR: THaScaler: LoadDataOnline: Cannot write "<<endl;
       cout << "request to VME server"<<endl;
       return SCAL_ERROR;
    }
    if (read (sFd, (char *)&vmeReply, sizeof(vmeReply)) < 0) {
       cout << "ERROR: THaScaler: LoadDataOnline: Error reading "<<endl;
       cout << " from VME server"<<endl;
       return SCAL_ERROR;
    }
  } else {
    rcsRequest.reply = 1;
    if(write(sFd, (char *)&rcsRequest, sizeof(rcsRequest)) == -1) {
       cout << "ERROR: THaScaler: LoadDataOnline: Cannot write "<<endl;
       cout << "request to VME server"<<endl;
       return SCAL_ERROR;
    }
    if (read (sFd, (char *)&rcsReply, sizeof(rcsReply)) < 0) {
       cout << "ERROR: THaScaler: LoadDataOnline: Error reading "<<endl;
       cout << " from VME server"<<endl;
       return SCAL_ERROR;
    }
  }
  close (sFd);

  LoadPrevious();
  Clear();

  if (crate != cratenum_rcs) {
    for (k = 0 ; k < 16*MAXBLK; k++) {
       vmeReply.ibuf[k] = ntohl(vmeReply.ibuf[k]);
    }
  } else {
    for (k = 0 ; k < 16*MAXBLK; k++) {
       rcsReply.ibuf[k] = ntohl(rcsReply.ibuf[k]);
    }
  }
  ntot = 0;
  for (slot = 0; slot < SCAL_NUMBANK; slot++) {
    int jslot = slot;
    if (slot >= MSGSIZE) {
      cout << "ERROR: THaScaler: LoadDataOnline:"<<endl;
      cout << "Cannot parse slot "<<slot<<endl;
      return SCAL_ERROR;
    }
    nchan = 0;
    if (crate != cratenum_rcs) {
       if (vmeReply.message[slot]=='0') nchan = 16;
       if (vmeReply.message[slot]=='1') nchan = 32;
    } else {  
       if (slot < 3) nchan = 32;  // assume only 3 slots for RCS, each 32 channels.
    }
    if (nchan == 0) goto onldone;
// FIXME: Kluge for R-arm until I figure out something better. Use 'jslot' to force correct order.
    if (vme_port == PORT_RIGHT) {
       if (slot == 1) jslot = 2;  // because ceb1 skipped
       if (slot == 2) jslot = 3;
       if (slot == 3) jslot = 5;  // because ceb4 skipped
       if (slot == 4) jslot = 6;
       if (slot == 5) jslot = 7;
       if (slot == 6) jslot = 8;
       if (slot == 7) jslot = 9;
    }
    for (k = 0; k < nchan; k++) {
      i = jslot*SCAL_NUMCHAN + k;
      if (i < SCAL_NUMBANK*SCAL_NUMCHAN && ntot < 16*MAXBLK) {
 	 if (crate != cratenum_rcs) {
            rawdata[i] = vmeReply.ibuf[ntot++];
	 } else {
	   if (ntot < 16*RCSMAXBLK) rawdata[i] = rcsReply.ibuf[ntot++];
	 }
      } else {
         if (SCAL_VERBOSE) {
           cout << "WARNING: THaScaler: LoadDataOnline:"<<endl;
           cout << "Truncation of data or improper array index"<<endl;
         }
      }
    }
  }
onldone:
  Load();
  if (lprint == 1) {  // Debug printout
     printf ("\n\n\n  ========    Scalers   ===============\n\n");
     sca = 0;
     for (k = 0; k < ntot/16; k++) {
       sca = 2*k;
       for (i = 0; i < 2; i++) {
 	  if (crate != cratenum_rcs) {
             printf ("%3d :  %8d %8d %8d %8d %8d %8d %8d %8d \n",8*(sca+i)+1,
             (int)vmeReply.ibuf[(sca+i)*8],(int)vmeReply.ibuf[(sca+i)*8+1],
             (int)vmeReply.ibuf[(sca+i)*8+2],(int)vmeReply.ibuf[(sca+i)*8+3],
             (int)vmeReply.ibuf[(sca+i)*8+4],(int)vmeReply.ibuf[(sca+i)*8+5],
             (int)vmeReply.ibuf[(sca+i)*8+6],(int)vmeReply.ibuf[(sca+i)*8+7]);
	  } else {
             printf ("%3d :  %8d %8d %8d %8d %8d %8d %8d %8d \n",8*(sca+i)+1,
             (int)rcsReply.ibuf[(sca+i)*8],(int)rcsReply.ibuf[(sca+i)*8+1],
             (int)rcsReply.ibuf[(sca+i)*8+2],(int)rcsReply.ibuf[(sca+i)*8+3],
             (int)rcsReply.ibuf[(sca+i)*8+4],(int)rcsReply.ibuf[(sca+i)*8+5],
             (int)rcsReply.ibuf[(sca+i)*8+6],(int)rcsReply.ibuf[(sca+i)*8+7]);
	  }
       }
     }
  }
  return 0;
};
   
void THaScaler::Print(Option_t* opt) const {
// Print data contents
  int i,j;
  cout << "\n============== Print out ================"<<endl;
  cout << "THaScaler Data for bankgroup = "<<bankgroup<<endl;
  cout << "Header "<<hex<<header<<"  crate num "<<dec<<crate<<endl;   
  int bk = 0;
  cout << "Raw data = "<<endl;
  for (i = 0; i < (long)scalerbanks.size(); i++) {
    for (j = 0; j < 8; j++) cout << hex << rawdata[i*32+j] << " ";
    cout << endl;
    for (j = 0; j < 8; j++) cout << hex << rawdata[i*32+j+8] << " ";
    cout << endl;
    for (j = 0; j < 8; j++) cout << hex << rawdata[i*32+j+16] << " ";
    cout << endl;
    for (j = 0; j < 8; j++) cout << hex << rawdata[i*32+j+24] << " ";
    cout << "\n-------"<< endl;
  }
  cout << "Banks = "<<endl;
  for (vector<THaScalerBank*>::const_iterator p = scalerbanks.begin();
       p != scalerbanks.end(); p++) {
      cout << "\n Printout for scaler bank "<<++bk<<endl;
      (*p)->Print();
   }
};

void THaScaler::PrintSummary() {
// Print out a summary of important scalers 
  if (CheckInit() == SCAL_ERROR) {
    cout << "THaScaler: ERROR:  You never initialized scalers\n"<<endl;
    cout << "Must call Init method once in the life of object\n"<<endl;
    cout << "Therefore, no results\n"<<endl;
    return;
  }
  Double_t time_sec = GetPulser("clock")/clockrate;
  if (time_sec == 0) {
    cout << "THaScaler: WARNING:  Time of run = ZERO (??)\n"<<endl;
    return;
  } 
  Double_t time_min = time_sec/60;  
  Double_t curr_u1  = ((Double_t)GetBcm("bcm_u1")/time_sec - off_u1)/calib_u1;
  Double_t curr_u3  = ((Double_t)GetBcm("bcm_u3")/time_sec - off_u3)/calib_u3;
  Double_t curr_u10 = ((Double_t)GetBcm("bcm_u10")/time_sec - off_u10)/calib_u10;
  Double_t curr_d1  = ((Double_t)GetBcm("bcm_d1")/time_sec - off_d1)/calib_d1;
  Double_t curr_d3  = ((Double_t)GetBcm("bcm_d3")/time_sec - off_d3)/calib_d3;
  Double_t curr_d10 = ((Double_t)GetBcm("bcm_d10")/time_sec - off_d10)/calib_d10;
  printf("\n ----------------   Scaler Summary   ---------------- \n");
  cout << "Scaler bank  " << bankgroup << endl;
  printf("Time of run  %7.2f min \n",time_min);
  printf("Triggers:     1 = %d    2 = %d    3 = %d   4 = %d    5 = %d\n",
         GetTrig(1),GetTrig(2),GetTrig(3),GetTrig(4),GetTrig(5));
  printf("Accepted triggers:   %d \n",GetNormData(0,"ts-accept"));
  printf("Accepted triggers by helicity:    (-) = %d    (+) = %d\n",
         GetNormData(-1,"ts-accept"),GetNormData(1,"ts-accept"));
  printf("Charge Monitors  (Micro Coulombs)\n");
  printf("Upstream BCM   gain x1 %8.2f     x3 %8.2f     x10 %8.2f\n",
     curr_u1*time_sec,curr_u3*time_sec,curr_u10*time_sec);
  printf("Downstream BCM   gain x1 %8.2f     x3 %8.2f     x10 %8.2f\n",
     curr_d1*time_sec,curr_d3*time_sec,curr_d10*time_sec);
};

Int_t THaScaler::GetScaler(Int_t slot, Int_t chan, Int_t histor) {
// Get data by slot and channel.  
// Histor = 0 = present event.  Histor = 1 = previous event
  Int_t index;
  index = SCAL_NUMCHAN*slot + chan;
  if (histor == 1) index += SCAL_NUMBANK*SCAL_NUMCHAN;
  if (index >=0 && index < 2*SCAL_NUMBANK*SCAL_NUMCHAN) {
    return rawdata[index];
  }
  return 0;
};

Int_t THaScaler::GetScaler(const char* detector, Int_t chan) {
  return GetScaler(detector, "LR", chan);
};

Int_t THaScaler::GetScaler(const char* det, const char* pm, Int_t chan, 
			   Int_t histor) 
{
// Accum. counts on PMTs of detector = "s1", "s2", "gasc", "a1", "a2",
// "leadgl",  "rcs1", "edtm"
// PMT = "left" or "right" or "LR"    
  Int_t ilr = 0;
  string detector = det;
  string PMT = pm;
  if (PMT == "Left"||PMT == "LEFT"||PMT == "left"||PMT == "L") ilr = 1;
  if (PMT == "Right"||PMT == "RIGHT"||PMT == "right"||PMT == "R") ilr = 2;
  if (PMT == "lr" || PMT == "LR") ilr = 3;
  if (detector == "s1" || detector == "S1") {
      if (ilr == 1) return s1L->GetData(chan,histor);   
      if (ilr == 2) return s1R->GetData(chan,histor);   
      if (ilr == 3) return s1->GetData(chan,histor);   
  } else if (detector == "s2" || detector == "S2") {
      if (ilr == 1) return s2L->GetData(chan,histor);   
      if (ilr == 2) return s2R->GetData(chan,histor);   
      if (ilr == 3) return s2->GetData(chan,histor);   
  } else if (detector == "gasc" || detector == "gasC") {
      return gasC->GetData(chan,histor);   
  } else if (detector == "a1" || detector == "A1") {
      if (ilr == 1) return a1L->GetData(chan,histor);   
      if (ilr == 2) return a1R->GetData(chan,histor);   
  } else if (detector == "a2" || detector == "A2") {
      if (ilr == 1) return a2L->GetData(chan,histor);   
      if (ilr == 2) return a2R->GetData(chan,histor);   
  } else if (detector == "leadgl" || detector == "LEADGL") {
      return leadgl->GetData(chan,histor);   
  } else if (detector == "rcs1" || detector == "RCS1") {
      return rcs1->GetData(chan,histor);   
  } else if (detector == "rcs2" || detector == "RCS2") {
      return rcs2->GetData(chan,histor);   
  } else if (detector == "rcs3" || detector == "RCS3") {
      return rcs3->GetData(chan,histor);   
  } else if (detector == "edtm" || detector == "EDTM") {
      return edtm->GetData(chan,histor);   
  }
  return -1;
};
  
Int_t THaScaler::GetTrig(Int_t trigger) {
// Non-helicity gated trigger counts for trig# 1,2,3...
   return GetTrig(0, trigger);
};

Int_t THaScaler::GetTrig(Int_t helicity, Int_t trig, Int_t histor) {
// Accum. counts for trig# 1,2,3,4,5, etc
// by helcity state (-1, 0, +1)  where 0 is non-helicity gated
// histor = 0 = this event.  1 = previous event.
   switch (helicity) {
     case 0:
         return norm->GetTrig(trig, histor);
         break;
     case -1:
         return nminus->GetTrig(trig, histor);
         break;
     case 1:
         return nplus->GetTrig(trig, histor);
         break;
     default:
         return 0;
    }
};

Int_t THaScaler::GetBcm(const char* which) {
// Non-helicity gated BCM counts for 'which' = 'bcm_u1','bcm_d1',
// 'bcm_u3','bcm_d3','bcm_u10','bcm_d10'
   return GetBcm(0, which);
};

Int_t THaScaler::GetBcm(Int_t helicity, const char* which, Int_t histor) {
// Get BCM counts for 'which' = 'bcm_u1','bcm_d1',
// 'bcm_u3','bcm_d3','bcm_u10','bcm_d10'
// by helcity state (-1, 0, +1)  where 0 is non-helicity gated
// histor = 0 = this event.  1 = previous event.
   return GetNormData(helicity, which, histor);
};

Int_t THaScaler::GetPulser(const char* which) {
   return GetPulser(0, which);
};

Int_t THaScaler::GetPulser(Int_t helicity, const char* which, Int_t histor) {
// Obtain pulser values, by 'which' = 'clock', 'edt', 'edtat', 'strobe', etc
  return GetNormData(helicity, which, histor);
};   

Int_t THaScaler::GetNormData(Int_t helicity, const char* which, Int_t histor) {
// Get Normalization Data for channel 'which'
// by helcity state (-1, 0, +1)  where 0 is non-helicity gated
// histor = 0 = this event.  1 = previous event.
   string w = which;
   switch (helicity) {
     case 0:
         return norm->GetData(w, histor);
     case -1:
         return nminus->GetData(w, histor);
     case 1:
         return nplus->GetData(w, histor);
     default:
         return 0;
    }
};

Int_t THaScaler::GetNormData(Int_t helicity, Int_t chan, Int_t histor) {
// Get Normalization Data for channel #chan = 0,1,2,...
// by helcity state (-1, 0, +1)  where 0 is non-helicity gated
// histor = 0 = this event.  1 = previous event.
   switch (helicity) {
     case 0:
         return norm->GetData(chan, histor);
     case -1:
         return nminus->GetData(chan, histor);
     case 1:
         return nplus->GetData(chan, histor);
     default:
         return 0;
   }
};

Double_t THaScaler::GetScalerRate(Int_t slot, Int_t chan) {
// Rates on scaler data, for slot #slot, channel #chan
  Double_t rate,etime;
  etime = GetTimeDiff(0);
  if (etime > 0) { 
      rate = ( GetScaler(slot, chan, 0) -
               GetScaler(slot, chan, 1) ) / etime;
      return rate;
  }
  return 0;
};

Double_t THaScaler::GetScalerRate(const char* detector, Int_t chan) {
   return GetScalerRate(detector, "LR", chan);
};

Double_t THaScaler::GetScalerRate(const char* detector, const char* PMT, Int_t chan) {
// Rates (Hz) since last update for detector 's1', 's2', 'gasC', etc
// PMT = "left", "right", "LR" (coinc),  and channel #chan.
  Double_t rate,etime;
  etime = GetTimeDiff(0);
  if (etime > 0) { 
      rate = ( GetScaler(detector, PMT, chan, 0) -
               GetScaler(detector, PMT, chan, 1) ) / etime;
      return rate;
  }
  return 0;
};

Double_t THaScaler::GetTrigRate(Int_t trigger) {
  return GetTrigRate(0, trigger);
};

Double_t THaScaler::GetTrigRate(Int_t helicity, Int_t trigger) {
  Double_t rate,etime;
  etime = GetTimeDiff(helicity);
  if (etime > 0) { 
      rate = ( GetTrig(helicity, trigger, 0) -
               GetTrig(helicity, trigger, 1) ) / etime;
      return rate;
  }
  return 0;
};

Double_t THaScaler::GetBcmRate(const char* which) {
  return GetBcmRate(0, which);
};      

Double_t THaScaler::GetBcmRate(Int_t helicity, const char* which) {
  Double_t rate,etime;
  etime = GetTimeDiff(helicity);
  if (etime > 0) { 
      rate = ( GetBcm(helicity, which, 0) -
               GetBcm(helicity, which, 1) ) / etime;
      return rate;
  }
  return 0;
};

Double_t THaScaler::GetPulserRate(const char* which) {
  return GetPulserRate(0, which);
};   

Double_t THaScaler::GetPulserRate(Int_t helicity, const char* which) {
  Double_t rate,etime;
  etime = GetTimeDiff(helicity);
  if (etime > 0) { 
      rate = ( GetPulser(helicity, which, 0) -
               GetPulser(helicity, which, 1) ) / etime;
      return rate;
  }
  return 0;
};

Double_t THaScaler::GetNormRate(Int_t helicity, const char* which) {
  Double_t rate,etime;
  etime = GetTimeDiff(helicity);
  if (etime > 0) { 
      rate = ( GetNormData(helicity, which, 0) -
               GetNormData(helicity, which, 1) ) / etime;
      return rate;
  }
  return 0;
};

Double_t THaScaler::GetNormRate(Int_t helicity, Int_t chan) {
  Double_t rate,etime;
  etime = GetTimeDiff(helicity);
  if (etime > 0) { 
      rate = ( GetNormData(helicity, chan, 0) -
               GetNormData(helicity, chan, 1) ) / etime;
      return rate;
  }
  return 0;
};

Double_t THaScaler::GetTimeDiff(Int_t helicity) {
  Double_t etime;
  if (clockrate == 0) return 0;   // Error, shouldn't happen.
  switch (helicity) {
    case 0:
      etime = (norm->GetData("clock", 0) -
               norm->GetData("clock", 1)) / clockrate;
      return etime;
   case -1:
      etime = (nminus->GetData("clock", 0) -
               nminus->GetData("clock", 1)) / clockrate;
      return etime;
   case 1:
      etime = (nplus->GetData("clock", 0) -
               nplus->GetData("clock", 1)) / clockrate;
      return etime;
   default:
      return 0;
  }
};

UInt_t THaScaler::header_str_to_base16(string hdr) {
// Utility to convert string header to base 16 integer
  static bool hs16_first = true;
  static map<char, int> strmap;
  typedef map<char,int>::value_type valType;
  //  pair<char, int> pci;
  static char chex[]="0123456789abcdef";
  static vector<int> numarray; 
  static int linesize = 12;
  if (hs16_first) {
    hs16_first = false;
    for (int i = 0; i < 16; i++) {
      //      pci.first = chex[i];  pci.second = i;  strmap.insert(pci);
      strmap.insert(valType(chex[i],i));
    }
    numarray.reserve(linesize);
  }
  numarray.clear();
  for (string::iterator p = hdr.begin(); p != hdr.end(); p++) {
    map<char, int>::iterator pm = strmap.find(*p);     
    if (pm != strmap.end()) numarray.push_back(pm->second);
    if ((long)numarray.size() > linesize-1) break;
  }
  UInt_t result = 0;  UInt_t power = 1;
  for (vector<int>::reverse_iterator p = numarray.rbegin(); 
      p != numarray.rend(); p++) {
      result += (*p) * power;  power *= 16;
  }
  return result;
};

Int_t THaScaler::CheckInit() {
  if ( !did_init ) {
    if (SCAL_VERBOSE) {
      cout << "WARNING: THaScaler: Unitialized THaScaler object"<<endl;
      cout << "Likely errors are:"<<endl;
      cout << "   1. User did not call Init() method"<<endl;
      cout << "   2. scaler.map file not found "<<endl;
      cout << "(scaler.map is on web and also in scaler source dir)"<<endl;
      cout << "Must be fixed, else THaScaler does not work"<<endl;
    }
    return SCAL_ERROR;
  }
  return 0;
};

void THaScaler::Clear(Option_t* opt) {
  memset(rawdata,0,SCAL_NUMBANK*SCAL_NUMCHAN*sizeof(Int_t));
};

void THaScaler::ClearAll() {
  memset(rawdata,0,2*SCAL_NUMBANK*SCAL_NUMCHAN*sizeof(Int_t));
};

void THaScaler::LoadPrevious() {
  int size = SCAL_NUMBANK*SCAL_NUMCHAN;
  for (int i = 0; i < size; i++) {
    rawdata[i+size] = rawdata[i];
  }
};

Int_t THaScaler::Load() 
{
  int bk = 0;
  for (vector<THaScalerBank*>::iterator p = scalerbanks.begin();
       p != scalerbanks.end(); p++) {
    if ((*p)->Load(rawdata) == SCAL_ERROR) {
      if(SCAL_VERBOSE) cout << "THaScaler::ERROR: loading bank "<<bk<<endl;
    }
    bk++;
  }
  return 0;
};

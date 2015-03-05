#ifndef THaEtClient_
#define THaEtClient_

//////////////////////////////////////////////////////////////////////
//
//   THaEtClient
//   Data from ET Online System
//
//   THaEtClient contains normal CODA data obtained via
//   the ET (Event Transfer) online system invented
//   by the JLab DAQ group.
//   This code works locally or remotely and uses the
//   ET system in a particular mode favored by  hall A.
//
//   Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaCodaData.h"

#define ET_CHUNK_SIZE 50
#ifndef __CINT__
#include "et.h"
#endif

class TString;

// The ET memory file will have this prefix.  The suffix is $SESSION.
#define ETMEM_PREFIX "/tmp/et_sys_"

// Hall A computers that run CODA/ET
//FIXME: un-hardcode these ... sigh
#define ADAQL1 "129.57.164.53"
#define ADAQL2 "129.57.164.59"
#define ADAQEP "129.57.164.78"
#define ADAQCP "129.57.164.79"
#define ADAQS2 "129.57.164.44"
#define ADAQS2 "129.57.164.44"
#define ADAQS3 "129.57.164.45"

namespace Decoder {

class THaEtClient : public THaCodaData
{

public:

    THaEtClient(Int_t mode=1);      // By default, gets data from ADAQS2
// find data on 'computer'.  e.g. computer="129.57.164.44"
    THaEtClient(const char* computer, Int_t mode=1);
    THaEtClient(const char* computer, const char* session, Int_t mode=1);
    ~THaEtClient();

    Int_t codaOpen(const char* computer, Int_t mode=1);
    Int_t codaOpen(const char* computer, const char* session, Int_t mode=1);
    Int_t codaClose();
    Int_t codaRead();            // codaRead() must be called once per event
    virtual bool isOpen() const;

private:

    THaEtClient(const THaEtClient &fn);
    THaEtClient& operator=(const THaEtClient &fn);
    Int_t CHUNK;
    Int_t DEBUG;
    Int_t FAST;
    Int_t SMALL_TIMEOUT;
    Int_t BIG_TIMEOUT;
    Int_t nread, nused, timeout;
#ifndef __CINT__
    et_sys_id id;
    et_statconfig sconfig;
    et_stat_id my_stat;
    et_att_id my_att;
    et_openconfig openconfig;
#endif
    char *daqhost,*session,*etfile;
    Int_t waitflag,didclose,notopened,firstread;
    void initflags();
    Int_t init(const char* computer="hana_sta");

// rate calculation
    Int_t firstRateCalc;
    Int_t evsum, xcnt;
    time_t daqt1;
    double ratesum;

    ClassDef(THaEtClient,0)   // ET client connection for online data

};

}

#endif

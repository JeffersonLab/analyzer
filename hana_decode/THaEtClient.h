#ifndef Podd_THaEtClient_h_
#define Podd_THaEtClient_h_

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
#include <ctime>

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
#define ADAQS3 "129.57.164.45"

namespace Decoder {

class THaEtClient : public THaCodaData
{

public:

    explicit THaEtClient(Int_t mode=1);   // By default, gets data from ADAQS2
// find data on 'computer'.  e.g. computer="129.57.164.44"
    explicit THaEtClient(const char* computer, Int_t mode=1);
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
    Int_t nread, nused, timeout;
#ifndef __CINT__
    et_sys_id id;
    et_att_id my_att;
#endif
    char *daqhost,*session,*etfile;
    Int_t waitflag,didclose,notopened,firstread;
    Int_t init(const char* computer="hana_sta");

// rate calculation
    Int_t firstRateCalc;
    Int_t evsum, xcnt;
    time_t daqt1;
    double ratesum;

	/* 
		ET Data de-chunk-ifying.
		Taken from Bryan Moffit's Repo:
		https://github.com/bmoffit/evet
	 */
	typedef struct etChunkStat
	{
		uint32_t *data;
		size_t length;
		int32_t endian;
		int32_t swap;

		int32_t  evioHandle;
	} etChunkStat_t;
	typedef struct evetHandle
	{
		et_sys_id etSysId;
		et_att_id etAttId;
		et_event **etChunk;      // pointer to array of et_events (pe)
		int32_t  etChunkSize;    // user requested (et_events in a chunk)
		int32_t  etChunkNumRead; // actual read from et_events_get
		int32_t  currentChunkID;  // j
		etChunkStat_t currentChunkStat; // data, len, endian, swap
		int32_t verbose=1; // 0 (none), 1 (data rate), 2+ (verbose)

	} evetHandle_t ;

	int32_t evetOpen(et_sys_id etSysId, int32_t chunk, evetHandle_t &evh);
	int32_t evetClose(evetHandle_t &evh);
	int32_t evetReadNoCopy(evetHandle_t &evh, const uint32_t **outputBuffer, uint32_t *length);
	int32_t evetGetEtChunks(evetHandle_t &evh);
	int32_t evetGetChunk(evetHandle_t &evh);

	evetHandle evh;

  ClassDef(THaEtClient,0)   // ET client connection for online data
};

}

#endif

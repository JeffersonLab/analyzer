#ifndef Podd_THaEpics_h_
#define Podd_THaEpics_h_

/////////////////////////////////////////////////////////////////////
//
//   THaEpics
//   see implementation for description
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <string>
#include <map>
#include <utility>
#include <vector>
//#include "Decoder.h"

namespace Decoder {

class EpicsChan {
// utility class of one epics channel
public:
  EpicsChan() : evnum(0), dvalue(0), timestamp(0) {}
  EpicsChan( std::string _tg, std::string _dt, UInt_t _ev,
             std::string _sv, std::string _un, Double_t _dv ) :
    tag(std::move(_tg)), dtime(std::move(_dt)), evnum(_ev),
    svalue(std::move(_sv)), units(std::move(_un)), dvalue(_dv),
    timestamp(0) { MakeTime(); }
  virtual ~EpicsChan() = default;
  void Load( char *tg, char *dt, UInt_t ev,
             char *sv, char *un, Double_t dv ) {
    tag = tg; dtime = dt; evnum = ev; svalue = sv; units = un; dvalue = dv;
    MakeTime();
  };
  Double_t    GetData()      const { return dvalue; };
  UInt_t      GetEvNum()     const { return evnum;  };
  std::string GetTag()       const { return tag;    };
  std::string GetDate()      const { return dtime;  };
  Double_t    GetTimeStamp() const { return timestamp; };
  std::string GetString()    const { return svalue; };
  std::string GetUnits()     const { return units;  };
    
private:
  std::string tag,dtime;
  UInt_t evnum;
  std::string svalue, units;
  Double_t dvalue,timestamp;

  void MakeTime() {
    // time is a continuous parameter.  funny things happen
    // at midnight or new month, but you'll figure it out.
    char t1[41],t2[41],t3[41],t4[41];
    int day = 0, hour = 0, min = 0, sec = 0;
    sscanf(dtime.c_str(),"%40s %40s %6d %6d:%6d:%6d %40s %40s",
           t1,t2,&day,&hour,&min,&sec,t3,t4);
    timestamp = 3600*24*day + 3600*hour + 60*min + sec;
  }
};

class THaEpics {

public:

   THaEpics() = default;
   virtual ~THaEpics() = default;
// Get tagged value nearest 'event'
   Double_t GetData( const char* tag, UInt_t event= 0 ) const;
// Get tagged string value nearest 'event'
   std::string GetString( const char* tag, UInt_t event= 0 ) const;
   Double_t GetTimeStamp( const char* tag, UInt_t event= 0 ) const;
   Int_t LoadData( const UInt_t* evbuffer, UInt_t event= 0 );  // load the data
   Bool_t IsLoaded(const char* tag) const;
   void Print();

private:

   std::map< std::string, std::vector<EpicsChan> > epicsData;
   std::vector<EpicsChan> GetChan(const char *tag) const;
   static UInt_t FindEvent( const std::vector<EpicsChan>& ep, UInt_t event );

   ClassDef(THaEpics,0)  // EPICS data

};

}

#endif

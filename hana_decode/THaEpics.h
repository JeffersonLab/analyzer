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
#include <ctime>
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
  time_t      GetTimeStamp() const { return timestamp; };
  std::string GetString()    const { return svalue; };
  std::string GetUnits()     const { return units;  };
    
private:
  std::string tag;       // Variable name
  std::string dtime;     // Timestamp as string
  UInt_t      evnum;     // Most recent physics event number seen
  std::string svalue;    // String data
  std::string units;     // Data units
  Double_t    dvalue;    // Numerical value of string data (0 if cannot convert)
  time_t      timestamp; // Unix time representation of dtime

  void MakeTime();
};

class THaEpics {

public:

   THaEpics() = default;
   virtual ~THaEpics() = default;
// Get tagged value nearest 'event'
   Double_t GetData( const char* tag, UInt_t event= 0 ) const;
// Get tagged string value nearest 'event'
   std::string GetString( const char* tag, UInt_t event= 0 ) const;
   time_t GetTimeStamp( const char* tag, UInt_t event= 0 ) const;
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

#ifndef THaEpics_
#define THaEpics_

/////////////////////////////////////////////////////////////////////
//
//   THaEpics
//   see implementation for description
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"

class EpicsChan {
// utility class of one epics channel
public:
  EpicsChan()  : tag(""), dtime(""), evnum(0), 
          svalue(""), units(""), dvalue(0) {}
  virtual ~EpicsChan() {}
  void Load(char *tg, char *dt, Int_t ev, 
            char *sv, char *un, Double_t dv) {
    tag = tg; dtime = dt;  evnum = ev;  
    svalue = sv; units = un;  dvalue = dv;
    MakeTime();
  };
  Double_t GetData() const { return dvalue; };
  Int_t GetEvNum() const   { return evnum;  };
  std::string GetTag() const    { return tag;    };
  std::string GetDate() const   { return dtime;  };
  Double_t GetTimeStamp() const { return timestamp; };
  void MakeTime() {
    // time is a continuous parameter.  funny things happen
    // at midnight or new month, but you'll figure it out.
    char t1[40],t2[40],t3[40],t4[40];
    int day, hour, min, sec;
    sscanf(dtime.c_str(),"%s %s %d %d:%d:%d %s %s",
	   t1,t2,&day,&hour,&min,&sec,t3,t4);
    timestamp = 3600*24*day + 3600*hour + 60*min + sec;
  }  
  std::string GetString() const { return svalue; };
  std::string GetUnits() const  { return units;  };
    
private:
  std::string tag,dtime;
  Int_t evnum;
  std::string svalue, units;
  Double_t dvalue,timestamp;
};

typedef std::map< std::string, std::vector<EpicsChan> >::value_type epVal;

class THaEpics {

public:

   THaEpics() { }
   virtual ~THaEpics() {}
// Get tagged value nearest 'event'
   Double_t GetData (const char* tag, int event=0) const;
// Get tagged string value nearest 'event'
   std::string GetString (const char* tag, int event=0) const;
   Double_t GetTimeStamp(const char* tag, int event=0) const;
   int LoadData (const int* evbuffer, int event=0);  // load the data
   Bool_t IsLoaded(const char* tag) const;
   void Print();

private:

   std::map< std::string, std::vector<EpicsChan> > epicsData;
   std::vector<EpicsChan> GetChan(const char *tag) const;
   Int_t FindEvent(const std::vector<EpicsChan> ep, int event) const;

   ClassDef(THaEpics,0)  // EPICS data 

};

#endif

#ifndef ROOT_THaEvent
#define ROOT_THaEvent

//////////////////////////////////////////////////////////////////////////
//
// THaEvent
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "Htypes.h"

class TH1;
class THaVar;
class THaArrayString;

class THaEventHeader {

public:
  THaEventHeader() : fEvtNum(0), fRun(0), fDate(0) { }
  virtual ~THaEventHeader() { }

  void   Set(Int_t i, Int_t r, Int_t d) { fEvtNum = i; fRun = r; fDate = d; }
  Int_t  GetEvtNum() const  { return fEvtNum; }
  Int_t  GetRun()    const  { return fRun; }
  Int_t  GetDate()   const  { return fDate; }

private:
  Int_t   fEvtNum;
  Int_t   fRun;
  Int_t   fDate;

  ClassDef(THaEventHeader,1)  //THaEvent Header
};


class THaEvent : public TObject {

public:
  static const char* const kDefaultHistFile;

  THaEvent();
  virtual ~THaEvent();

  void              SetHeader( Int_t i, Int_t run, Int_t date )
                                { fEvtHdr.Set( i, run, date ); }
  THaEventHeader*   GetHeader() { return &fEvtHdr; }

  Int_t             DefineHist( const char* var, 
				const char* histname, const char* title, 
				Int_t nbins, Axis_t xlo, Axis_t xhi );
  virtual void      Clear( Option_t *opt = "" );
  virtual Int_t     Init();
  virtual Int_t     Fill();
  void              PrintHist( Option_t* opt="" ) const;
  Int_t             LoadHist( const char* filename=kDefaultHistFile );
  virtual void      Reset( Option_t *opt = "" );

protected:

  Int_t             HistInit();
  Int_t             HistFill();

  THaEventHeader    fEvtHdr;     // Event header

  Bool_t            fInit;       //! Fill() initialized

  struct DataMap {
    Int_t        ncopy;          //! Number of elements to copy
    const char*  name;           //! Global variable name
    void*        dest;           //! Address of corresponding member variable
    Int_t*       ncopyvar;       //! Variable holding value of ncopy (if ncopy=-1)
    size_t       size;           //! Size per element (bytes)
    const void*  src;            //! Address of global variable
  };
  DataMap*       fDataMap;       //! Map of global variables to copy

private:
  struct HistDef {
    const char*  name;           //! Global variable name
    const char*  hname;          //! Histogram name
    const char*  title;          //! Histogram title
    Int_t        nbins;          //! Number of bins
    Axis_t       xlo;            //! Low edge
    Axis_t       xhi;            //! High edge
    TH1*         h1;             //! Pointer to histogram object
    THaVar*      pvar;           //! Pointer to global variable object
    Int_t        type;           //! Histogram type (0=C,1=S,2=F,3=D)
    THaArrayString* parsed_name; //! Pointer to parsed name object
  };
  HistDef*       fHistDef;       //! Table of 1-d histogram definitions

  //FIXME: These belong in a utility class
  UInt_t         IntDigits( Int_t n ) const;
  UInt_t         FloatDigits( Float_t f ) const;

  ClassDef(THaEvent,2)  //Base class for event structure definition
};


#endif


#ifndef ROOT_THaRun
#define ROOT_THaRun

//////////////////////////////////////////////////////////////////////////
//
// THaRun
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "TDatime.h"

class THaCodaData;
class THaRunParameters;
class THaEvData;

class THaRun : public TNamed {
  
public:
  THaRun( const char* filename="", const char* description="" );
  THaRun( const THaRun& run );
  virtual THaRun& operator=( const THaRun& rhs );
  virtual ~THaRun();
  
  virtual bool operator==( const THaRun& ) const;
  virtual bool operator!=( const THaRun& ) const;
  virtual bool operator< ( const THaRun& ) const;
  virtual bool operator> ( const THaRun& ) const;
  virtual bool operator<=( const THaRun& ) const;
  virtual bool operator>=( const THaRun& ) const;

  virtual void         ClearDate();
          void         ClearEventRange();
  virtual Int_t        CloseFile();
  virtual Int_t        Compare( const TObject* obj ) const;
          Bool_t       DBRead()         const { return fDBRead; }
          const char*  GetFilename()    const { return fFilename.Data(); }
          const TDatime& GetDate()      const { return fDate; }
          Int_t        GetNumber()      const { return fNumber; }
          Int_t        GetType()        const { return fType; }
          UInt_t       GetFirstEvent()  const { return fEvtRange[0]; }
          UInt_t       GetLastEvent()   const { return fEvtRange[1]; }
          const Int_t* GetEvBuffer()    const;
  THaRunParameters*    GetParameters()  const { return fParam; }
  virtual Bool_t       HasInfo( UInt_t bits ) const;
  virtual Int_t        Init();
          Bool_t       IsInit()         const { return fIsInit; }
  virtual bool         IsOpen()         const;
  virtual Int_t        OpenFile();
  virtual Int_t        OpenFile( const char* filename );
  virtual void         Print( Option_t* opt="" ) const;
  virtual Int_t        ReadEvent();
  virtual void         SetDate( const TDatime& date );
          void         SetDate( UInt_t tloc );
  virtual void         SetFilename( const char* name );
          void         SetFirstEvent( UInt_t first )   { fEvtRange[0] = first; }
          void         SetLastEvent( UInt_t last )     { fEvtRange[1] = last; }
          void         SetEventRange( UInt_t first, UInt_t last )
    { SetFirstEvent(first); SetLastEvent(last); }
  virtual void         SetNumber( Int_t number );
  virtual void         SetType( Int_t type );
  virtual Int_t        Update( const THaEvData* evdata );

  enum EInfoType { kDate      = BIT(0), 
		   kRunNumber = BIT(1),
		   kRunType   = BIT(2),
		   kPrescales = BIT(3) };

protected:
  Int_t         fNumber;       //  Run number
  Int_t         fType;         //  Run type/mode/etc.
  TString       fFilename;     //  File name
  TDatime       fDate;         //  Run date and time
  UInt_t        fEvtRange[2];  //  Event range to analyze
  UInt_t        fAnalyzed[2];  //  Event range actually analyzed
  Bool_t        fDBRead;       //  True if database successfully read.
  Bool_t        fIsInit;       //  True if run successfully initialized
  Bool_t        fAssumeDate;   //  True if run date explicitly set
  Int_t         fMaxScan;      //  Max. no. of events to prescan (0=don't scan)
  UInt_t        fDataSet;      //  Flags for info that is valid (see EInfotype)
  UInt_t        fDataRead;     //  Flags for info found in data (see EInfoType)
  UInt_t        fDataRequired; //  Info required for Init() to succeed
  THaCodaData*  fCodaData;     //! CODA data associated with this run
  THaRunParameters* fParam;    //  Run parameters

  virtual Int_t        ReadDatabase();

  ClassDef(THaRun,4)       // Control information for a run
};


#endif

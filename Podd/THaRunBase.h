#ifndef Podd_THaRunBase_h_
#define Podd_THaRunBase_h_

//////////////////////////////////////////////////////////////////////////
//
// THaRunBase
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "TDatime.h"
#include <cstdio>        // for EOF
#include <memory>
#include <string>

class THaRunParameters;
class THaEvData;

class THaRunBase : public TNamed {

public:
  explicit THaRunBase( const char* description="" );
  THaRunBase( const THaRunBase& run );
  virtual THaRunBase& operator=( const THaRunBase& rhs );
  virtual ~THaRunBase();

  virtual bool operator==( const THaRunBase& ) const;
  virtual bool operator!=( const THaRunBase& ) const;
  virtual bool operator< ( const THaRunBase& ) const;
  virtual bool operator> ( const THaRunBase& ) const;
  virtual bool operator<=( const THaRunBase& ) const;
  virtual bool operator>=( const THaRunBase& ) const;

  // Return codes for Init/Open/ReadEvent/Close
  enum { READ_OK = 0, READ_EOF = EOF, READ_ERROR = -32, READ_FATAL = -64 };

  // Main functions
  virtual const UInt_t* GetEvBuffer() const = 0;
  virtual Int_t        Init();
  virtual Int_t        Open() = 0;
  virtual Int_t        ReadEvent() = 0;
  virtual Int_t        Close() = 0;

  // Auxiliary functions
  virtual void         Clear( Option_t* opt="" );
  virtual void         ClearDate();
          void         ClearEventRange();
  virtual Int_t        Compare( const TObject* obj ) const;
          Bool_t       DBRead()         const { return fDBRead; }
          void         IncrNumAnalyzed( Int_t n=1 ) { fNumAnalyzed += n; }
  const   TDatime&     GetDate()        const { return fDate; }
          UInt_t       GetDataRequired() const { return fDataRequired; }
  // GetDataVersion is intentionally not const; derived classes modify members
  virtual Int_t        GetDataVersion()       { return fDataVersion; }
          UInt_t       GetNumAnalyzed() const { return fNumAnalyzed; }
          UInt_t       GetNumber()      const { return fNumber; }
          UInt_t       GetType()        const { return fType; }
          UInt_t       GetFirstEvent()  const { return fEvtRange[0]; }
          UInt_t       GetLastEvent()   const { return fEvtRange[1]; }
  THaRunParameters*    GetParameters()  const { return fParam.get(); }
  virtual Bool_t       HasInfo( UInt_t bits ) const;
  virtual Bool_t       HasInfoRead( UInt_t bits ) const;
          Bool_t       IsInit()         const { return fIsInit; }
  virtual Bool_t       IsOpen()         const;
  virtual void         Print( Option_t* opt="" ) const;
  virtual void         SetDate( const TDatime& date );
          void         SetDate( UInt_t tloc );
	  void         SetDataRequired( UInt_t mask ); // mask is OR of EInfoType
  virtual Int_t        SetDataVersion( Int_t version );
          void         SetFirstEvent( UInt_t n );
          void         SetLastEvent(  UInt_t n );
          void         SetEventRange( UInt_t first, UInt_t last );
  virtual void         SetNumber( UInt_t number );
          void         SetRunParamClass( const char* classname );
  virtual void         SetType( UInt_t type );
  virtual Int_t        Update( const THaEvData* evdata );

  enum EInfoType { kDate      = BIT(0),
		   kRunNumber = BIT(1),
		   kRunType   = BIT(2),
		   kPrescales = BIT(3),
                   kDAQInfo   = BIT(4) };

  // DAQ configuration strings (usually database file dumps) from user event
  size_t               GetNConfig() const;
  const std::string&   GetDAQConfig( size_t i ) const;
  const std::string&   GetDAQInfo( const std::string& key ) const;

protected:
  UInt_t        fNumber;        // Run number
  UInt_t        fType;          // Run type/mode/etc.
  TDatime       fDate;          // Run date and time
  UInt_t        fEvtRange[2];   // Event range to analyze
  UInt_t        fNumAnalyzed;   // Number of physics events actually analyzed
  Bool_t        fDBRead;        // True if database successfully read.
  Bool_t        fIsInit;        // True if run successfully initialized
  Bool_t        fOpened;        // True if opened successfully
  Bool_t        fAssumeDate;    // True if run date explicitly set
  UInt_t        fDataSet;       // Flags for info that is valid (see EInfoType)
  UInt_t        fDataRead;      // Flags for info found in data (see EInfoType)
  UInt_t        fDataRequired;  // Info required for Init() to succeed
  std::unique_ptr<THaRunParameters> fParam; // Run parameters
  TString       fRunParamClass; // Class of object in fParam
  Int_t         fDataVersion;   // Data format version (implementation-dependent)
  TObject*      fExtra;         // Additional member data (for binary compat.)

  virtual Int_t ReadDatabase();
  virtual Int_t ReadInitInfo( Int_t level = 0);

  ClassDef(THaRunBase,6)       // Base class for run objects
};


#endif

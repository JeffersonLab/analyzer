#ifndef ROOT_THaRun
#define ROOT_THaRun

//////////////////////////////////////////////////////////////////////////
//
// THaRun
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "TDatime.h"
#include <limits.h>

class THaCodaFile;

class THaRun : public TNamed {
  
public:
  THaRun();
  THaRun( const char* filename, const char* description="" );
  THaRun( const THaRun& run );
  THaRun& operator=( const THaRun& rhs );
  virtual ~THaRun();
  
  virtual Int_t        CloseFile();
  virtual Int_t        Compare( const TObject* obj ) const;
  virtual void         Copy( TObject& obj );
  virtual void         FillBuffer( char*& buffer );
          const char*  GetFilename()    const { return fFilename.Data(); }
          const TDatime& GetDate()      const { return fDate; }
          UInt_t       GetNumber()      const { return fNumber; }
          UInt_t       GetFirstEvent()  const { return fFirstEvent; }
          UInt_t       GetLastEvent()   const { return fLastEvent; }
          const Int_t* GetEvBuffer()    const;
  virtual Int_t        OpenFile();
  virtual Int_t        OpenFile( const char* filename );
  virtual void         Print( Option_t* opt="" ) const;
  virtual Int_t        ReadEvent();
          void         SetDate( TDatime& date )        { fDate = date; }
          void         SetDate( UInt_t tloc );
          void         SetFilename( const char* name ) { fFilename = name; }
          void         SetFirstEvent( UInt_t first )   { fFirstEvent = first; }
          void         SetLastEvent( UInt_t last )     { fLastEvent = last; }
          void         SetEventRange( UInt_t first, UInt_t last )
    { fFirstEvent = first; fLastEvent = last; }
          void         ClearEventRange() 
    { fFirstEvent = 0; fLastEvent = UINT_MAX; }
  virtual void         SetNumber( UInt_t number );

protected:
  UInt_t        fNumber;       //  Run number
  TString       fFilename;     //  File name
  TDatime       fDate;         //  Run date and time
  UInt_t        fFirstEvent;   //  First event to analyze
  UInt_t        fLastEvent;    //  Last event to analyze
  THaCodaFile*  fCodaFile;     //! CODA file associated with this run

  ClassDef(THaRun,1)   //Description of a run
};


#endif


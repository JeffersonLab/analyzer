#ifndef ROOT_THaRun
#define ROOT_THaRun

//////////////////////////////////////////////////////////////////////////
//
// THaRun
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "TDatime.h"
#include <climits>

class THaCodaData;
class THaTarget;

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
  virtual Int_t        CloseFile();
  virtual Int_t        Compare( const TObject* obj ) const;
          Bool_t       DBRead()         const { return fDBRead; }
          Double_t     GetBeamE()       const { return fBeamE; }
          Double_t     GetBeamdE()      const { return fBeamdE; }
          Double_t     GetBeamM()       const { return fBeamM; }
          Double_t     GetBeamP()       const { return fBeamP; }
          Int_t        GetBeamQ()       const { return fBeamQ; }
          const char*  GetFilename()    const { return fFilename.Data(); }
          const TDatime& GetDate()      const { return fDate; }
          Int_t        GetNumber()      const { return fNumber; }
          UInt_t       GetFirstEvent()  const { return fFirstEvent; }
          UInt_t       GetLastEvent()   const { return fLastEvent; }
          const Int_t* GetEvBuffer()    const;
          THaTarget*   GetTarget()      const { return fTarget; }
  virtual Int_t        Init();
          Bool_t       IsInit()         const { return fIsInit; }
  virtual bool         IsOpen()         const;
  virtual Int_t        OpenFile();
  virtual Int_t        OpenFile( const char* filename );
  virtual void         Print( Option_t* opt="" ) const;
  virtual Int_t        ReadDatabase();
  virtual Int_t        ReadEvent();
          void         SetBeam( Double_t E, Double_t M, Int_t Q, 
				Double_t dE = 0.0 );
  virtual void         SetDate( const TDatime& date );
          void         SetDate( UInt_t tloc );
  virtual void         SetFilename( const char* name );
          void         SetFirstEvent( UInt_t first )   { fFirstEvent = first; }
          void         SetLastEvent( UInt_t last )     { fLastEvent = last; }
          void         SetEventRange( UInt_t first, UInt_t last )
    { fFirstEvent = first; fLastEvent = last; }
          void         SetTarget( THaTarget* tgt )     { fTarget = tgt; }
          void         ClearEventRange() 
    { fFirstEvent = 0; fLastEvent = UINT_MAX; }
  virtual void         SetNumber( Int_t number );

protected:
  Int_t         fNumber;       //  Run number
  TString       fFilename;     //  File name
  TDatime       fDate;         //  Run date and time
  Bool_t        fAssumeDate;   //  True if run date explicitly set
  UInt_t        fFirstEvent;   //  First event to analyze
  UInt_t        fLastEvent;    //  Last event to analyze
  THaCodaData*  fCodaData;     //! CODA data associated with this run
  Bool_t        fDBRead;       //  True if database successfully read.
  Bool_t        fIsInit;       //  True if run successfully initialized

  // The following comes from the run database
  Double_t      fBeamE;        //  Total nominal beam energy (GeV)
  Double_t      fBeamP;        //  Calculated Beam momentum (GeV/c)
  Double_t      fBeamM;        //  Rest mass of beam particles (GeV/c^2)
  Int_t         fBeamQ;        //  Charge of beam particles (electron: -1)
  Double_t      fBeamdE;       //  Beam energy uncertainty (GeV)
  THaTarget*    fTarget;       //! Pointer to target description

  ClassDef(THaRun,4)   //Description of a run
};


#endif

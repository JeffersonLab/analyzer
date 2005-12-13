#ifndef HallA_THaDBFile
#define HallA_THaDBFile

//////////////////////////////////////////////////////////////////////////
//
// THaDBFile
//
//  A simple class to read database from a key - value format txtfile
//
//  Date/time-based granularity is implemented, though clumsy in the
//  file at the moment.
// 
//////////////////////////////////////////////////////////////////////////

#include "TDatime.h"
#include "THaDB.h"

#include<string>
#include<vector>
#include<iostream>

class THaDBFile : public THaDB {

 public:
  // constructor to connect to specific datafiles
  THaDBFile(const char* calib="default.db",
	     const char* detcfg="det.config",
	     std::vector<THaDetConfig>* detmap=0);
  
  virtual ~THaDBFile();

  Int_t GetValue( const char* system, const char* attr,
		  Int_t& value, const TDatime &date );
  Int_t GetValue( const char* system, const char* attr,
		  Double_t& value, const TDatime &date );
  Int_t GetValue( const char* system, const char* attr,
		  std::string& value, const TDatime &date );

  Int_t GetArray( const char* system, const char* attr,
		  std::vector<Int_t>& array, const TDatime &date );
  Int_t GetArray( const char* system, const char* attr,
		  std::vector<Double_t>& array, const TDatime &date );

  Int_t GetArray( const char* system, const char* attr,
		  Int_t* array, Int_t size, const TDatime &date );
  Int_t GetArray( const char* system, const char* attr,
		  Double_t* array, Int_t size, const TDatime &date );

  Int_t LoadValues ( const char* system, const TagDef* list,
		     const TDatime& date );

  Int_t GetMatrix( const char* system, const char* name,
		   std::vector<std::vector<Int_t> >& rows,
		   const TDatime& date );
  Int_t GetMatrix( const char* system, const char* name,
		   std::vector<std::vector<Double_t> >& rows,
		   const TDatime& date );

  // SPECIALIZED for the transport matrix with named rows
  Int_t GetMatrix( const char* systemC,
		   std::vector<std::string>& mtr_name,
		   std::vector<std::vector<Double_t> >& mtr_rows,
		   const TDatime& date );

  Int_t PutValue( const char* system, const char* attr,
		  const Int_t& value, const TDatime& date );
  Int_t PutValue( const char* system, const char* attr,
		  const Double_t& value, const TDatime& date );
  Int_t PutValue( const char* system, const char* attr,
		  const std::string& value, const TDatime& date );
  
  Int_t PutArray( const char* system, const char* attr,
		  const std::vector<Int_t>& array, const TDatime& date );
  Int_t PutArray( const char* system, const char* attr,
		  const std::vector<Double_t>& array, const TDatime& date );
  
  Int_t PutArray( const char* system, const char* attr,
		  const Int_t* array, Int_t size, const TDatime& date );
  Int_t PutArray( const char* system, const char* attr,
		  const Double_t* array, Int_t size, const TDatime& date );
  
  Int_t StoreValues( const char* system, const TagDef* list,
		     const TDatime& date );
  
  Int_t PutMatrix( const char* system, const char* name,
		   const std::vector<std::vector<Double_t> >& rows,
		   const TDatime& date );
  
  Int_t PutMatrix( const char* system, const char* name,
		   const std::vector<std::vector<Int_t> >& rows,
		   const TDatime& date );
  
  // SPECIALIZED for the transport matrix with named rows
  Int_t PutMatrix( const char* systemC,
		   const std::vector<std::string>& mtr_name,
		   const std::vector<std::vector<Double_t> >& mtr_rows,
		   const TDatime& date );
  
  void  LoadDetCfgFile( const char* detcfg );
  void  SetCalibFile( const char* calib );

  void  SetDescription( const char* comment);

  void  SetOutFile( const char* outfname );

  Int_t PutDetMap( const TDatime& date );

  int LoadDB();       // force reloading of database from file, what is in memory
  
  int FlushDB();      // Flush DB in memory to file

  friend std::ostream& operator<<(std::ostream&, THaDBFile&);

  void PrintDB();
  
 protected:
  Int_t LoadDetMap(const TDatime& date);
  
  std::string fInFileName;      // filename to read the calibration DB from
  std::string fOutFileName;     // filename to write the DB to
  std::string fDetFile;         // file to read the detector configuration from
  std::string fDescription;     // tmp string to hold the description of current system

 private:
  bool find_constant(std::istream& from, int linebreak=0);
  bool FindEntry( std::string& system, std::string& attr, std::istream& from,
		  TDatime& date);

  void WriteDate(std::ostream& to, const TDatime& date);
  bool IsDate(std::istream& from, std::streampos &pos, TDatime& date );
  
  template<class T>
    Int_t ReadValue( const char* systemC, const char* attrC,
		     T& value, TDatime date);
  template<class T>
    Int_t ReadArray( const char* systemC, const char* attrC,
		     std::vector<T>& array, TDatime date);
  template<class T>
    Int_t ReadArray( const char* systemC, const char* attrC,
		     T* array, Int_t size, TDatime date );
  template<class T>
    Int_t ReadMatrix( const char* systemC, const char* attrC, 
		      std::vector<std::vector<T> >& rows, TDatime date );
  template<class T>
    Int_t WriteValue( const char* systemC, const char* attrC,
		      const T& value, TDatime date);
  template<class T>
    Int_t WriteArray( const char* system, const char* attr,
		      const std::vector<T>& v, TDatime date);
  template<class T>
    Int_t WriteArray( const char* system, const char* attr,
		      const T* array, Int_t size, 
		      TDatime date);
  template <class T>
    Int_t WriteMatrix( const char* system, const char* attr,
		       const std::vector<std::vector<T> >& matrix,
		       TDatime date );

  std::string db_contents; // everything in this database -- buffer for reading/writing
  

  int modified;
  bool NextLine( std::istream& from );
  bool CopyDB(std::istream& from, std::ostream& to, std::streampos pos=-1);
  
 public:
  ClassDef(THaDBFile,0) //  An ASCII file-based implementation of THaDB


};

#endif  // HallA_THaDBFile


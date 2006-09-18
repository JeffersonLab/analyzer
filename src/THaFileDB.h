#ifndef HallA_THaFileDB
#define HallA_THaFileDB

//////////////////////////////////////////////////////////////////////////
//
// THaFileDB
//
//  Interface to a database stored in key-value format textfile(s).
//
//////////////////////////////////////////////////////////////////////////

#include "THaDB.h"
#include "TDatime.h"

#include<string>
#include<vector>
#include<iostream>


class THaFileDB : public THaDB {

 public:
  // constructor to connect to specific datafiles
  THaFileDB( const char* calib="default" );
  
  virtual ~THaFileDB();

  virtual UInt_t GetValue( const char* system, const char* attr,
			   Int_t& value, const TDatime& date );
  virtual UInt_t GetValue( const char* system, const char* attr,
			   Double_t& value, const TDatime& date );
  virtual UInt_t GetValue( const char* system, const char* attr,
			   std::string& value, const TDatime& date );

  virtual UInt_t GetArray( const char* system, const char* attr,
			   std::vector<Int_t>& array, const TDatime& date );
  virtual UInt_t GetArray( const char* system, const char* attr,
			   std::vector<Double_t>& array, const TDatime& date );

  virtual UInt_t GetArray( const char* system, const char* attr,
			   Int_t* array, UInt_t size, const TDatime& date );
  virtual UInt_t GetArray( const char* system, const char* attr,
			   Double_t* array, UInt_t size, const TDatime& date );

  virtual UInt_t LoadValues( const char* system, const TagDef* list,
			     const TDatime& date );

  virtual UInt_t GetMatrix( const char* system, const char* name,
			    std::vector<std::vector<Int_t> >& rows,
			    const TDatime& date );
  virtual UInt_t GetMatrix( const char* system, const char* name,
			    std::vector<std::vector<Double_t> >& rows,
			    const TDatime& date );
  virtual UInt_t GetMatrix( const char* systemC, const char* name,
			    std::vector<std::string>& row_names,
			    std::vector<std::vector<Double_t> >& rows,
			    const TDatime& date );

  virtual UInt_t PutValue( const char* system, const char* attr,
			   const Int_t& value, const TDatime& date );
  virtual UInt_t PutValue( const char* system, const char* attr,
			   const Double_t& value, const TDatime& date );
  virtual UInt_t PutValue( const char* system, const char* attr,
			   const std::string& value, const TDatime& date );
  
  virtual UInt_t PutArray( const char* system, const char* attr,
			   const std::vector<Int_t>& array, 
			   const TDatime& date );
  virtual UInt_t PutArray( const char* system, const char* attr,
			   const std::vector<Double_t>& array, 
			   const TDatime& date );
  
  virtual UInt_t PutArray( const char* system, const char* attr,
			   const Int_t* array, UInt_t size, 
			   const TDatime& date );
  virtual UInt_t PutArray( const char* system, const char* attr,
			   const Double_t* array, UInt_t size,
			   const TDatime& date );
  
  virtual UInt_t StoreValues( const char* system, const TagDef* list,
			      const TDatime& date );
  
  virtual UInt_t PutMatrix( const char* system, const char* name,
			    const std::vector<std::vector<Double_t> >& rows,
			    const TDatime& date );
  
  virtual UInt_t PutMatrix( const char* system, const char* name,
			    const std::vector<std::vector<Int_t> >& rows,
			    const TDatime& date );
  
  virtual UInt_t PutMatrix( const char* systemC, const char* name,
			    const std::vector<std::string>& row_names,
			    const std::vector<std::vector<Double_t> >& rows,
			    const TDatime& date );
  
  void  LoadDetCfgFile( const char* detcfg );

  void  SetDescription( const char* comment);

  UInt_t PutDetMap( const TDatime& date );

  int FlushDB();  // Flush DB in memory to file

  friend std::ostream& operator<<(std::ostream&, const THaFileDB&);

  void PrintDB() const;

  virtual Int_t SetDBDir( const char* name );
  virtual void  SetInFile( const char* name );
  virtual void  SetOutFile( const char* outfname );

 protected:
  UInt_t LoadDetMap(const TDatime& date);
  
  static void WriteDate(std::ostream& to, const TDatime& date);

  static bool find_constant(std::istream& from, int linebreak=0);

  static bool IsDate( const std::string& line, TDatime& date );
  static bool IsKey( const std::string& line, const std::string& key, 
		     std::string::size_type& offset ); 
  static bool FindEntry( const char* systemC, const char* attrC, 
			 std::istream& from, TDatime& date );
  static bool SeekDate( std::istream& from, TDatime& date );

  template<class T>
  UInt_t ReadValue( const char* systemC, const char* attrC,
		    T& value, const TDatime& date);
  template<class T>
  UInt_t ReadArray( const char* systemC, const char* attrC,
		    std::vector<T>& array, const TDatime& date,
		    UInt_t size = 0 );
  template<class T>
  UInt_t ReadMatrix( const char* systemC, const char* attrC, 
		     std::vector<std::vector<T> >& rows,
		     const TDatime& date );
  template<class T>
  UInt_t WriteValue( const char* systemC, const char* attrC,
		     const T& value, const TDatime& date);
  template<class T>
  UInt_t WriteArray( const char* system, const char* attr,
		     const std::vector<T>& v, const TDatime& date );
  template <class T>
  UInt_t WriteMatrix( const char* system, const char* attr,
		      const std::vector<std::vector<T> >& matrix,
		      const TDatime& date );

  struct DBFile {
    std::string system_name;
    TDatime     date;
    std::string contents;
    bool        good;
    bool        tried;
  } db[3];

  std::string fDBDir;         // Database directory root. If unset, use $DB_DIR.
  std::string fOutFileName;   // filename to write the DB to
  std::string fDetFile;       // file to read the detector configuration from
  std::string fDescription;   // tmp string to hold the description of current system

  Int_t fDebug;               // Verbosity level for diagnostic messages


  int modified;
  static bool CopyDB(std::istream& from, std::ostream& to, std::streampos pos=-1);
  
  int LoadFile( const char* systemC, const TDatime& date,
		std::string& contents );
  int LoadFile( DBFile& db );
  bool LoadDB( const char* systemC, const TDatime& date );


  ClassDef(THaFileDB,0) //  An ASCII file-based implementation of THaDB


};

#endif  // HallA_THaFileDB



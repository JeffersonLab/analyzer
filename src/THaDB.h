#ifndef HallA_THaDB
#define HallA_THaDB

//////////////////////////////////////////////////////////////////////////
//
// THaDB
//
// Simple abstract class to define interface for Database access.
//   Mostly general, but with specialized members for dealing with the
//   detector maps.
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TDatime.h"

#include<string>
#include<vector>

class THaDetMap;

class THaDB;
class THaDetConfig;

extern THaDB* gHaDB;

struct TagDef {
  const char*  name;          //name of data field
  void*        data;          //location of data to put/get
  int          type;          //data type (kDouble, kInt, etc.)
  int          expected;      //optional: number of elements to write/read
  int          fatal;         //optional: error code to return if not found
};

class THaDB : public TObject {

 public:
  THaDB();
  
  virtual ~THaDB();

  enum EDtype { kDouble=0, kInt=1, kString };


  // For all 'Get' routines, the returned Int_t is the number of elements read
  // and should be >0 for success.
  virtual Int_t GetValue( const char* system, const char* attr, Double_t& value,
			  const TDatime& date ) = 0;
  
  virtual Int_t GetValue( const char* system, const char* attr, Int_t&    value,
			  const TDatime& date ) = 0;
  
  virtual Int_t GetValue( const char* system, const char* attr, std::string& value,
			  const TDatime& date ) = 0;
  
  virtual Int_t GetArray( const char* system, const char* attr, 
			  std::vector<Double_t>& array,
			  const TDatime& date, Int_t expected = -1 ) = 0;
  
  virtual Int_t GetArray( const char* system, const char* attr, 
			  std::vector<Int_t>&          array,
			  const TDatime& date, Int_t expected = -1 ) = 0;
  
  virtual Int_t GetArray( const char* system, const char* attr, 
			  std::vector<Int_t>&          array,
			  const TDatime& date, Int_t expected = -1 ) = 0;
  
  virtual Int_t GetArray( const char* system, const char* attr, 
			  std::vector<std::string>&    array,
			  const TDatime& date, Int_t expected = -1 ) = 0;
  
  virtual Int_t GetArray( const char* system, const char* attr, Double_t*   array,
			  Int_t size, const TDatime& date ) = 0;
  
  virtual Int_t GetArray( const char* system, const char* attr, Int_t*      array,
			  const TDatime& date, Int_t size ) = 0;
  
  virtual Int_t GetArray( const char* system, const char* attr, std:string* array,
			  const TDatime& date, Int_t size ) = 0;
  
  virtual Int_t LoadValues ( const char* system, const TagDef* list,
			     const TDatime& date ) = 0;
  
  virtual Int_t GetMatrix( const char* system, const char* name,
			   std::vector<std::vector<Double_t> >& rows,
			   const TDatime& date ) = 0;
  
  // specialized for elements with named rows -- TRANSPORT MATRIX
  virtual Int_t GetMatrix( const char* system, const char* name,
			   std::vector<std::string>& row_name,
			   std::vector<std::vector<Double_t> >& row_data,
			   const TDatime& date ) = 0;
  
  virtual Int_t WriteValue( const char* system, const char* attr,
			    Double_t&    value, const TDatime& date ) = 0;
  
  virtual Int_t WriteValue( const char* system, const char* attr,
			    Int_t&       value, const TDatime& date ) = 0;
  
  virtual Int_t WriteValue( const char* system, const char* attr,
			    std::string& value, const TDatime& date ) = 0;
  
  virtual Int_t WriteValue( const char* system, const char* attr,
			    const char*  value, const TDatime& date ) = 0;
  
  virtual Int_t WriteArray( const char* system, const char* attr,
			    std::vector<Double_t>& array, const TDatime& date ) = 0;

  virtual Int_t WriteArray( const char* system, const char* attr,
			    std::vector<Int_t>& array,    const TDatime& date ) = 0;

  virtual Int_t WriteArray( const char* system, const char* attr,
			    std::vector<std::string>& array,
			    const TDatime& date ) = 0;

  virtual Int_t WriteArray( const char* system, const char* attr,
			    std::vector<const char*>& array,
			    const TDatime& date ) = 0;
  
  virtual Int_t WriteArray( const char* system, const char* attr,
			    Double_t* array, Int_t size, const TDatime& date ) = 0;
  
  virtual Int_t WriteArray( const char* system, const char* attr,
			    Int_t*    array, Int_t size, const TDatime& date ) = 0;
  

  // specialized for elements with named rows, such as the TRANSPORT MATRIX
  virtual  Int_t  WriteMatrix( const char* system, const char* name,
			       std::vector<std::string>& row_name,
			       std::vector<std::vector<Double_t> >& row_data,
			       const TDatime& date ) = 0;

  virtual  Int_t  WriteMatrix( const char* system, const char* name,
			       std::vector<std::vector<Double_t> >& rows,
			       const TDatime& date ) = 0;

  virtual  Int_t  WriteMatrix( const char* system, const char* name,
			       std::vector<std::vector<Int_t> >& rows,
			       const TDatime& date ) = 0;

  virtual  Int_t  StoreValues( const char* system, const TagDef* list,
			       const TDatime& date ) = 0;

  // Set the comment associated with future DB writes
  // weakly breaks the state-lessness of the class
  virtual  void  SetDescription(const char* comment) { /* do nothing */ }

  Int_t GetDetMap( const char* sysname, THaDetMap& detmap, const TDatime& date );

  virtual  UInt_t WriteDetMap( const TDatime& date ) = 0;

 protected:
  virtual  Int_t LoadDetMap(const TDatime& date) = 0;
  std::vector<THaDetConfig> fDetectorMap; // contains the detector configurations


 public:
  
  ClassDef(THaDB,0)
};

class THaDetConfig {
  
 public:
  THaDetConfig(std::string line);
  bool IsGood() const;
  
 private:
  std::string name;
  int wir, roc, crate, slot;
  int chan_lo, chan_hi;
  std::string model;
  
 public:
  friend class THaDB;
  friend std::ostream& operator<<(std::ostream&, const THaDetConfig& );
  
};



#endif  // HallA_THaDB

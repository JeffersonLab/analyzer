#ifndef _USRSTRUTILS_INCLUDED
#define _USRSTRUTILS_INCLUDED

/////////////////////////////////////////////////////////////////////
//
// R. Michaels, March 2000
// THaUsrstrutils = USeR STRing UTILitieS.
// The code below is what is used by DAQ to interpret strings
// like prescale factors
// Yes, this is mostly old-style C, but it
// has the advantage that the interpretation should
// be identical to what the DAQ used.
//
/////////////////////////////////////////////////////////////////////

#include "TString.h"

namespace Decoder {

class THaUsrstrutils {

/* usrstrutils

   utilities to extract information from configuration string passed
   to ROC in the *.config file in rcDatabase.

   The config line can be of the form

   keyword[=value][,keyword[=value]] ...

   CRL code can use the following three routines to look for keywords and
   the associated values.

   int getflag(char *s) - Return 0 if s not present as a keyword
                                 1 if keyword is present with no value
				 2 if keyword is present with a value
   int getint(char *s) - If keyword present, interpret value as an integer.
                         Value assumed deximal, unless preceeded by 0x for hex
			 Return 0 if keyword not present or has no value.

   char *getstr(char *s) - Return ptr to string value associated with
                           the keyword.  Return null if keyword not present.
			   return null string if keyword has no value.
			   Caller must delete the string.

    string_from_evbuffer(int evbuffer) - load the confuguration
                         string using event buffer 'evbuffer'

    string_from_file(char *ffile) -- read file *ffile to get config string.
                                 This is closest to the original init_strings
                                 of S. Wood

*/
/* Define some common keywords as symbols, so we have just one place to
   change them*/
#define BUFFERED "buf"
#define PARALLEL "par"
#define THRESHOLDS "tfile"
#define FLAG_FILE "ffile"
#define SCALERS "sfile"
#define SCALER_PERIOD "tscaler"
#define BASE_SCALER "bscaler"
#define FILE_CONFIG "fconfig"
#define SYNC "sync"
#define PS1 "ps1"
#define PS2 "ps2"
#define PS3 "ps3"
#define PS4 "ps4"
#define PS5 "ps5"
#define PS6 "ps6"
#define PS7 "ps7"
#define PS8 "ps8"
#define PEDESTALS "nped"
#define PEDSUP "pedsup"
#define ELECTRON "electron"
#define HADRON "hadron"
#define ECOINC "ecoinc"
#define HCOINC "hcoinc"
#define NOFPP "nofpp"
#define DISABLE "disable"
#define ESCALE "escale"
#define HSCALE "hscale"
#define RETIME "retime"
#define PHELICITY "phelicity"
#define BPM "bpm"
#define CNTRM "cntrm"
#define COUNTING_HOUSE "ch"
#define COMMENT_CHAR ';'

public:

  static const int MAX;
  static const unsigned long LONGMAX;

  THaUsrstrutils() {}
  virtual ~THaUsrstrutils() {}
  int getflag(const char *s) const;
  char *getstr(const char *s) const;
  unsigned int getint(const char *s) const;
  void string_from_evbuffer(const UInt_t* evbuffer, int nlen=MAX);
  void string_from_file(const char *ffile_name);

protected:

  TString configstr;
  void getflagpos(const char *s, const char **pos_ret,
		  const char **val_ret) const;
  static void getflagpos_instring(const char *constr, const char *s,
				  const char **pos_ret, const char **val_ret);

   ClassDef(THaUsrstrutils,0)   //  User string utilities, DAQ parsing code.

};

//=============== inline functions ================================

inline
void THaUsrstrutils::getflagpos(const char *s, const char **pos_ret,
				const char **val_ret) const
{
  getflagpos_instring(configstr,s,pos_ret,val_ret);
  return;
}

}

#endif  /* _USRSTRUTILS_INCLUDED */



















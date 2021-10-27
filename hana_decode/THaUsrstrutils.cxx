/////////////////////////////////////////////////////////////////////
//
// R. Michaels, March 2000
// Updated string_from_buffer, May 2007/
// THaUsrstrutils = USeR STRing UTILitieS.
// The code below is what is used by DAQ to interpret strings
// like prescale factors 
// Yes, this is mostly old-style C, but it
// has the advantage that the interpretation should
// be identical to what the DAQ used.  
//
/////////////////////////////////////////////////////////////////////

#include "THaUsrstrutils.h"
#include <cctype>    // for isspace
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm> // find_if_not
#include <iterator>  // for std::distance
#include <iostream>

using namespace std;

namespace Decoder {

static const int BUFLEN=256;

int THaUsrstrutils::getflag(const char *s) const
{
  const char *pos,*val;

  getflagpos(s,&pos,&val);
  if(!pos) return(0);
  if(!val) return(1);
  return(2);
}

char* THaUsrstrutils::getstr(const char *s) const
{
  const char *pos,*val;

  getflagpos(s,&pos,&val);
  if(!val){
    return nullptr;
  }
  const char* end = strchr(val,',');	/* value string ends at next keyword */
  size_t slen = (end) ? end - val
                      :	strlen(val); /* No more keywords, value is rest of string */
  char* ret = new char[slen+1];
  strncpy(ret,val,slen);
  ret[slen] = '\0';
  return(ret);
}

unsigned int THaUsrstrutils::getint(const char *s) const
{
  char* sval = getstr(s);
  if(!sval) return(0);     /* Just return zero if no value string */
  long retval = strtol(sval,nullptr,0);
  if( retval < 0 || retval > kMaxUInt )
    retval = 0;            /* Return zero if out of range of unsigned int */
  delete [] sval;
  return(retval);
}

long THaUsrstrutils::getSignedInt(const char *s) const
{
  char* sval = getstr(s);
  if(!sval) return(-1);    /* Return -1 if no value string */
  long retval = strtol(sval,nullptr,0);
  delete [] sval;
  return(retval);
}

void THaUsrstrutils::getflagpos_instring( const char* confstr,
                                          const char* s,
                                          const char** pos_ret,
                                          const char** val_ret )
{
  size_t slen = strlen(s);
  const char *pos = confstr;
  *val_ret = nullptr;

  while(pos) {
    pos = strstr(pos,s);
    if(pos) {
      /* Make sure it is an isolated keyword */
      if((pos != confstr && pos[-1] != ',') ||
	 (pos[slen] != '=' && pos[slen] != ',' && pos[slen] != '\0')) {
	pos++; continue;
      } else break;		/* It's good */
    }
  }
  *pos_ret = pos;
  if( pos && pos[slen] == '=' ) {
    *val_ret = pos + slen + 1;
  }
}
  
void THaUsrstrutils::string_from_evbuffer( const UInt_t* evbuffer, UInt_t nlen )
{
// Routine string_from_evbuffer loads the configstr from the event buffer.
// It has the same strengths and weaknesses as the DAQ code,
// hence the interpretation should be the same.
//
// nlen is the length of the string data (from CODA) in longwords (4 bytes)
//
// R. Michaels, updated to make the algorithm closely resemble the
//              usrstrutils.c used by trigger supervisor.
// O. Hansen,   converted the C code to C++, painstakingly preserving the
//              original algorithm.

  // The following could be written in C without requiring any new memory
  // allocations, albeit it would be much less readable and more error-prone

  // Copy the entire input buffer to a std::string, which is safe even if the
  // buffer isn't null-terminated. This usually costs less than 500 bytes.
  string strbuff(reinterpret_cast<const char*>(evbuffer), sizeof(UInt_t) * nlen);

  // Remove trailing zero padding, if any
  string::size_type pos = strbuff.find('\0');
  if( pos != string::npos )
    strbuff.erase(pos);

  #ifdef USRSTRDEBUG
  cout << "Check THaUsrstrutils::string_from_buffer "<<endl;
  cout << "strbuff length "<<strbuff.length()<<endl;
  cout << "strbuff == \n"<<strbuff<<endl<<endl;
#endif

  configstr.clear();

// In order to make this as nearly identical to the online VME code,
// we unpack the buffer into lines separated by '\n'.
// This is like the reading of the prescale.dat file done
// by fgets(s,255,fd) in usrstrutils.c
// For each line, we extract the configuration strings using the same
// algorithm as in usrstrutils.c
  pos = 0;
  string::size_type buflen = strbuff.length();
  while( pos < buflen ) {
    // Inspect each line until an uncommented, non-empty line is found
    string::size_type pos1 = strbuff.find('\n', pos);
#ifdef USRSTRDEBUG
    cout << "strbuff parse pos,pos1,line=" << pos << "," << pos1
         << ",\"" << strbuff.substr(pos, pos1 - pos) << "\"" << endl;
#endif
    string sline = strbuff.substr(pos, pos1 - pos);
    // Skip lines starting with a comment character
    auto p = sline.find(COMMENT_CHAR);
    if( p > 0 ) {
      // Blow away trailing comments
      if( p != string::npos )
        sline.erase(p);
      // Skip leading whitespace using isspace(), like the C version does
      auto it = find_if_not(sline.begin(), sline.end(),
                            static_cast<int(*)(int)>(isspace));
      if( it != sline.end() ) {
        // We have a config string
        auto textstart = distance(sline.begin(), it);
        configstr = sline.substr(textstart);
        break;
      }
    }
    if( pos1 == string::npos )
      // Last line did not end with \n
      break;
    pos = pos1 + 1;
  }
#ifdef USRSTRDEBUG
  cout << "configstr =  " << configstr << endl;
#endif
}
      
// This routine reads a file to load file_configustr.
// It is like what is used by the DAQ code to load prescale factors, etc.

void THaUsrstrutils::string_from_file(const char *ffile_name)
{
  /* check that filename exists */
  configstr.clear();
  FILE* fd = fopen(ffile_name,"r");
  if(fd == nullptr) {
    cerr << "Failed to open usr flag file " << ffile_name << endl;
    return;
  }
  /* Read till an uncommented line is found */
  char buf[BUFLEN];
  char* flag_line = nullptr;
  while( fgets(buf, BUFLEN, fd) ) {
    char* arg = strchr(buf, COMMENT_CHAR);
    if( arg ) *arg = '\0';   /* Blow away comments */
    arg = buf;               /* Skip whitespace */
    while( *arg && isspace(*arg) ) {
      arg++;
    }
    if( *arg ) {
      flag_line = arg;
      break;
    }
  }
  if( flag_line ) {            /* We have a config string */
    configstr = string(flag_line);
#ifdef USRSTRDEBUG
    cout << "configstr =  " << configstr << endl;
#endif
  }
  fclose(fd);
}

}

ClassImp(Decoder::THaUsrstrutils)


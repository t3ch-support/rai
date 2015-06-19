/*  ---------------------------------------------------------------------
    Copyright 2014 Marc Toussaint
    email: marc.toussaint@informatik.uni-stuttgart.de
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a COPYING file of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>
    -----------------------------------------------------------------  */


/// @file
/// @ingroup group_Core
/// @addtogroup group_Core
/// @{

#ifndef MT_util_h
#define MT_util_h

#include <iostream>
#include <fstream>
#include <typeinfo>
#include <stdint.h>

#ifdef MT_ROS
#  include <ros/ros.h>
#endif

//----- if no system flag, I assume Linux
#if !defined MT_MSVC && !defined MT_Cygwin && !defined MT_Linux && !defined MT_MinGW && !defined MT_Darwin
#  define MT_Linux
#endif


//----- basic defs:
#define MT_PI 3.14159265358979323846
#define MT_LN2 0.69314718055994528622676398299518041312694549560546875
#define MT_2PI 6.283195307179587
#define MT_LnSqrt2Pi -0.9189385332046727417803296
#define MT_SQRT2 1.414213562373095049
#define MT_SQRTPI 1.772453850905516027
typedef unsigned char byte;            ///< byte
typedef unsigned int uint;             ///< unsigned integer
typedef const char* charp;

//----- macros to define the standard <<and >>operatos for most my classes:
#define stdInPipe(type)\
  inline std::istream& operator>>(std::istream& is, type& x){ x.read(is);return is; }
#define stdOutPipe(type)\
  inline std::ostream& operator<<(std::ostream& os, const type& x){ x.write(os); return os; }
#define stdPipes(type)\
  inline std::istream& operator>>(std::istream& is, type& x){ x.read(is); return is; }\
  inline std::ostream& operator<<(std::ostream& os, const type& x){ x.write(os); return os; }
#define inPipe(type)\
  inline type& operator<<(type& x, const char* str){ std::istringstream ss(str); ss >>x; return x; }
#define niyPipes(type)\
  inline std::istream& operator>>(std::istream& is, type& x){ NIY; return is; }\
  inline std::ostream& operator<<(std::ostream& os, const type& x){ NIY; return os; }




//----- macros for piping doubles EXACTLY (without rounding errors) in hex coding:
#define OUTHEX(y) "0x" <<std::hex <<*((unsigned long*)&y) <<std::dec
#define INHEX(y)  std::hex >>*((unsigned long*)&y) >>std::dec


//===========================================================================
//
// standard little methods in my namespace
//

namespace MT {
extern int argc;
extern char** argv;
extern bool IOraw;  ///< stream modifier for some classes (Mem in particular)
extern uint lineCount;
extern int verboseLevel;
extern int interactivity;

//----- files
void open(std::ofstream& fs, const char *name, const char *errmsg="");
void open(std::ifstream& fs, const char *name, const char *errmsg="");

//----- strings and streams
bool contains(const char *s, char c);
char skip(std::istream& is, const char *skipSymbols=" \n\r\t", const char *stopSymbols=NULL, bool skipCommentLines=true);
void skipRestOfLine(std::istream& is);
void skipOne(std::istream& is);
char getNextChar(std::istream& is, const char *skipSymbols=" \n\r\t", bool skipCommentLines=true);
char peerNextChar(std::istream& is, const char *skipSymbols=" \n\r\t", bool skipCommentLines=true);
bool parse(std::istream& is, const char *str, bool silent=false);
bool skipUntil(std::istream& is, const char *tag);

//----- functions
byte bit(byte *str, uint i);
void flip(byte& b, uint i);
void flip(int& b, uint i);
double MIN(double a, double b);
double MAX(double a, double b);
uint MAX(uint a, uint b);
double indicate(bool expr);
double modMetric(double x, double y, double mod);
double sign(double x);
double sign0(double x);
double linsig(double x);
//void   clip(double& x, double a, double b);
double phi(double dx, double dy);
double dphi(double x, double y, double dx, double dy);
double DIV(double x, double y, bool force=false);
double sigmoid11(double x);
double sigmoid(double x);
double dsigmoid(double x);
double approxExp(double x);
double Log(double x);
uint   Log2(uint n);
double sqr(double x);
double sinc(double x);
double cosc(double x);
double erf(double x);
double gaussInt(double x);
double gaussIntExpectation(double x);
double NNsdv(const double& a, const double& b, double sdv);
double NNsdv(double x, double sdv);
double smoothRamp(double x, double eps, double power);
double d_smoothRamp(double x, double eps, double power);

double ineqConstraintCost(double g, double margin, double power);
double d_ineqConstraintCost(double g, double margin, double power);
double eqConstraintCost(double h, double margin, double power);
double d_eqConstraintCost(double h, double margin, double power);

//----- time access
double clockTime(); //(really on the clock)
double realTime(); //(since process start)
double cpuTime();
double sysTime();
double totalTime();
double toTime(const tm& t);
char *date();
char *date(const struct timeval& tv);
void wait(double sec, bool msg_on_fail=true);
bool wait();

//----- memory
long mem();

//----- timer functions
void   timerStart(bool useRealTime=false);
double timerRead(bool reset=false);
double timerRead(bool reset, double startTime);
double timerPause();
void   timerResume();

//----- command line handling
void initCmdLine(int _argc, char *_argv[]);
bool checkCmdLineTag(const char *tag);
char *getCmdLineArgument(const char *tag);

//----- config file handling
void openConfigFile(const char *name=0);

//----- parameter grabbing from command line, config file, or default value
template<class T> T getParameter(const char *tag);
template<class T> T getParameter(const char *tag, const T& Default);
template<class T> void getParameter(T& x, const char *tag, const T& Default);
template<class T> void getParameter(T& x, const char *tag);
template<class T> bool checkParameter(const char *tag);

template<class T> void putParameter(const char* tag, const T& x);
template<class T> bool getFromMap(T& x, const char* tag);

//----- get verbose level
uint getVerboseLevel();
bool getInteractivity();
}

//----- stream parsing
struct PARSE { const char *str; PARSE(const char* _str):str(_str) {} };
std::istream& operator>>(std::istream& is, const PARSE&);


//===========================================================================
//
// String class
//

#define STRING(x) (((MT::String&)(MT::String().stream() <<x)))
#define STREAM(x) (((MT::String&)(MT::String().stream() <<x)).stream())

namespace MT {
/** @brief String implements the functionalities of an ostream and an
istream, but also can be send to an ostream or read from an
istream. It is based on a simple streambuf derived from the
MT::Mem class */
struct String:public std::iostream {
private:
  struct StringBuf:std::streambuf {
    String *string;
    virtual int overflow(int C = traits_type::eof());
    virtual int sync();
    void setIpos(char *p);
    char *getIpos();
  } buffer;
  void init();
  
public:
  /// @name data fields
  char *p;    ///< pointer to memory
  uint N;     ///< \# elements (excluding zero)
  uint M;     ///< actual buffer size (in terms of # elements)
  static const char *readSkipSymbols; ///< default argument to read method (also called by operator>>)
  static const char *readStopSymbols; ///< default argument to read method (also called by operator>>)
  static int readEatStopSymbol;       ///< default argument to read method (also called by operator>>)
  void (*flushCallback)(String&);
  
  /// @name constructors
  String();
  String(const String& s);
  String(const char *s);
  explicit String(const std::string& s);
  ~String();
  
  /// @name access
  operator char*();
  operator const char*() const;
  char &operator()(uint i) const;
  std::iostream& stream();            ///< explicitly returns this as an std::iostream&
  String& operator()();               ///< explicitly return this as a (non-const!) String&
  String getSubString(uint start, uint end) const;
  String getFirstN(uint n) const;
  String getLastN(uint n) const;
  
  /// @name setting
  String& operator=(const String& s);
  void operator=(const char *s);
  void set(const char *s, uint n);
  void resize(uint n, bool copy); //low-level resizing the string buffer - fully uninitialized but with final 0
  void append(char x); //low-level append a char
  String& setRandom();
  
  /// @name resetting
  String& clear();       //as with Array: resize(0)
  String& clearStream(); //call IOstream::clear();
  String& resetIstream();
  
  /// @name equality
  bool operator==(const char *s) const;
  bool operator==(const String& s) const;
  bool operator!=(const char *s) const;
  bool operator!=(const String& s) const;
  bool operator<(const String& s) const;
  
  /// @name misc
  bool contains(const String& substring) const;
  bool startsWith(const String& substring) const;
  bool startsWith(const char* substring) const;
  bool endsWith(const String& substring) const;
  bool endsWith(const char* substring) const;
  
  /// @name I/O
  void write(std::ostream& os) const;
  uint read(std::istream& is, const char* skipSymbols=NULL, const char *stopSymbols=NULL, int eatStopSymbol=-1);
};
stdPipes(String)
}


//===========================================================================
//
// string-filling routines

namespace MT {
  MT::String getNowString();
}


//===========================================================================
//
// logging

namespace MT {
struct LogToken{
  MT::String msg;
  int log_level;
  int logCoutLevel, logFileLevel;
  const char *filename, *function;
  uint line;
  std::ofstream *fil;
  LogToken(int log_level, int logCoutLevel, int logFileLevel, const char* filename, const char* function, uint line, std::ofstream* fil)
    : log_level(log_level), logCoutLevel(logCoutLevel), logFileLevel(logFileLevel), filename(filename), function(function), line(line), fil(fil) {}
  ~LogToken();
  std::ostream& os(){ return msg; }
};
}
//template<class T> MT::LogToken& operator<<(MT::LogToken& log, const T& x){ log.os() <<x;  return log; }

struct Log{
  int logFileLevel, logCoutLevel;
  const char* key;
  std::ofstream fil;
  Log(const char* key, int defaultLogCoutLevel=0, int defaultLogFileLevel=0){
    logCoutLevel = MT::getParameter<int>(STRING("logCoutLevel_"<<key), defaultLogCoutLevel);
    logFileLevel = MT::getParameter<int>(STRING("logFileLevel_"<<key), defaultLogFileLevel);
    MT::open(fil, STRING("z.log."<<key));
  }
  ~Log(){ fil.close(); }
  MT::LogToken operator()(int log_level, const char* filename, const char* function, uint line) const{
    return MT::LogToken(log_level, logCoutLevel, logFileLevel, filename, function, line, (std::ofstream*)&fil);
  }
};

extern Log _log;

//inline MT::LogToken LOG(int log_level=0){ return MT::LogToken(log_level, __FILE__, __func__, __LINE__); }
//#define LOG(log_level) (MT::LogToken(log_level, __FILE__, __func__, __LINE__).os())
//#define LOG(log_level) MT::LogToken(log_level, MT::logFileLevel, MT::logCoutLevel, __FILE__, __func__, __LINE__).os()
#define LOG(log_level) _log(log_level, __FILE__, __func__, __LINE__).os()

void setLogLevels(int fileLogLevel=3, int consoleLogLevel=2);

//The destructor ~LogToken writes into the log file and
//console. setLogLevel allows to adjust cout verbosity (0 by default),
//and what is written into the log file (1 by default)


//===========================================================================
//
// macros for halting/MSGs etc
//

//----- declare my namespace for the first time:
/// Marc Toussaint namespace
namespace MT {
extern String errString;
}

//----- error handling:
#  define MT_HERE "@" << __FILE__<<':' <<__FUNCTION__ <<':' <<__LINE__ <<": "
///* #ifdef MT_MSVC */
///* (strrchr(__FILE__, '\\')?strrchr(__FILE__, '\\')+1:__FILE__) */
///* #else */
///* #  define MT_HERE "@" <<(strrchr(__FILE__, '/')?strrchr(__FILE__, '/')+1:__FILE__) <<':' <<__LINE__ <<':' <<__FUNCTION__ <<": " */
///* #endif */
//#ifdef MT_ROS
//#  define MT_WRITE_MSG(str) { std::cerr <<str <<std::endl; ROS_INFO("MLR-MSG: %s",str.p); }
//#else
//#  define MT_WRITE_MSG(str) { std::cerr <<str <<std::endl; }
//#endif
//#ifndef MT_MSG
//#  define MT_MSG(msg){ std::cerr <<MT_HERE <<msg <<std::endl; }
//#endif
//#ifndef HALT
//#  define HALT(msg)  { MT::errString.clear() <<MT_HERE <<msg <<" --- HALT"; MT_WRITE_MSG(MT::errString); throw MT::errString.p; }
//#  define NIY HALT("not implemented yet")
//#  define NICO HALT("not implemented with this compiler options: usually this means that the implementation needs an external library and a corresponding compiler option - see the source code")
//#  define OPS HALT("obsolete")
//#endif

#ifndef HALT
#  define MT_MSG(msg){ LOG(-1) <<msg; }
#  define THROW(msg){ LOG(-2) <<msg; throw MT::errString.p; }
#  define HALT(msg){ LOG(-2) <<msg; exit(1); }
#  define NIY  { LOG(-3) <<"not implemented yet"; exit(1); }
#  define NICO { LOG(-3) <<"not implemented with this compiler options: usually this means that the implementation needs an external library and a corresponding compiler option - see the source code"; exit(1); }
#  define OPS  { LOG(-3) <<"obsolete"; exit(1); }
#endif




//----- check macros:
#ifndef MT_NOCHECK

#define CHECK(cond, msg) \
  if(!(cond)){ LOG(-2) <<"CHECK failed: '" <<#cond <<"' " <<msg;  throw MT::errString.p; }\

#define CHECK_ZERO(expr, tolerance, msg) \
  if(fabs((double)(expr))>tolerance){ LOG(-2) <<"CHECK_ZERO failed: '" <<#expr<<"'=" <<expr <<" > " <<tolerance <<" -- " <<msg; throw MT::errString.p; } \

#define CHECK_EQ(A, B, msg) \
  if(!(A==B)){ LOG(-2) <<"CHECK_EQ failed: '" <<#A<<"'=" <<A <<" '" <<#B <<"'=" <<B <<" -- " <<msg; throw MT::errString.p; } \

#define CHECK_GE(A, B, msg) \
  if(!(A>=B)){ LOG(-2) <<"CHECK_GE failed: '" <<#A<<"'=" <<A <<" '" <<#B <<"'=" <<B <<" -- " <<msg; throw MT::errString.p; } \

#define CHECK_LE(A, B, msg) \
  if(!(A<=B)){ LOG(-2) <<"CHECK_LE failed: '" <<#A<<"'=" <<A <<" '" <<#B <<"'=" <<B <<" -- " <<msg; throw MT::errString.p; } \

#else
#  define CHECK(cond, msg)
#define CHECK_ZERO(expr, tolerance, msg)
#define CHECK_EQ(A, B, msg)
#define CHECK_GE(A, B, msg)
#define CHECK_LE(A, B, msg)
#endif


//----- TESTING
#ifndef EXAMPLES_AS_TESTS
#  define TEST(name) test##name()
#  define MAIN main
#else
#  define GTEST_DONT_DEFINE_TEST 1
#  include <gtest/gtest.h>
#  define TEST(name) test##name(){} GTEST_TEST(examples, name)
#  define MAIN \
     main(int argc, char** argv){ \
       MT::initCmdLine(argc,argv);              \
       testing::InitGoogleTest(&argc, argv);	\
       return RUN_ALL_TESTS();			\
     }                                          \
     inline int obsolete_main //this starts a method declaration
#endif


//----- verbose:
#define VERBOSE(l, x) if(l<=MT::getVerboseLevel()) x;


//----- other macros:
#define MEM_COPY_OPERATOR(x) memmove(this, &x, sizeof(this));


//===========================================================================
//
// FileToken
//

namespace MT {
  /** @brief A ostream/istream wrapper that allows easier initialization of objects, like:
arr X = FILE("inname");
X >>FILE("outfile");
 etc
*/
struct FileToken{
  MT::String path, name, cwd;
  std::ofstream *os;
  std::ifstream *is;

  FileToken(const char* _filename, bool change_dir=true);
  ~FileToken();
  FileToken& operator()(){ return *this; }

  void decomposeFilename();
  void changeDir();
  bool exists();
  std::ofstream& getOs();
  std::ifstream& getIs(bool change_dir=false);
  operator std::istream&(){ return getIs(); }
  operator std::ostream&(){ return getOs(); }
};
template<class T> FileToken& operator>>(FileToken& fil, T& x){ fil.getIs() >>x;  return fil; }
template<class T> FileToken& operator<<(FileToken& fil, const T& x){ fil.getOs() <<x;  return fil; }
inline std::ostream& operator<<(std::ostream& os, FileToken& fil){ return os <<fil.name; }
template<class T> void operator<<(T& x, FileToken& fil){ fil.getIs() >>x; }
template<class T> void operator>>(const T& x, FileToken& fil){ fil.getOs() <<x; }
}
#define FILE(filename) (MT::FileToken(filename)()) //it needs to return a REFERENCE to a local scope object


//===========================================================================
//
// random number generator
//

namespace MT {
/** @brief A random number generator. An global instantiation \c
  MT::rnd of a \c Rnd object is created. Use this one object to get
  random numbers.*/
class Rnd {
private:
  bool ready;
  int32_t rpoint;     /* Feldindex    */
  int32_t rfield[256];   /* Schieberegisterfeld  */
  
  
public:
  /// ...
  Rnd() { ready=false; };
  
  
public:/// @name initialization
  /// initialize with a specific seed
  uint32_t seed(uint32_t n);
  
  /// use Parameter<uint>("seed") as seed
  uint32_t seed();
  
  /// uses the internal clock to generate a seed
  uint32_t clockSeed();
  
public:/// @name access
  /// a initeger random number uniformly distributed in [0, ?]
  uint32_t num() { if(!ready) seed(); return (uint32_t)rnd250() >>5; }
  /// same as \c num()
  uint32_t operator()() { return num(); }
  /// a initeger random number uniformly distributed in [0, \c i-1]
  uint32_t num(uint32_t limit) {
    CHECK(limit, "zero limit in rnd.num()"); return num() % limit;
  }
  uint32_t num(int32_t lo, int32_t hi) { return lo+num(hi-lo+1); }
  /// same as \c num(i)
  uint32_t operator()(uint32_t i) { return num(i); }
  uint32_t operator()(int32_t lo, int32_t hi) { return num(lo, hi); }
  /// a random variable uniformly distributed in [0, 1]
  double uni() { return ((double)num(1 <<22))/(1 <<22); }
  /// a random variable uniformly distributed in [\c low, \c high]
  double uni(double low, double high) { return low+uni()*(high-low); }
  /// a gaussian random variable with mean zero
  double gauss();
  /** @brief a positive integer drawn from a poisson distribution with given
    \c mean; is case \c mean>100, a (positive) gauss number
    \c floor(mean+gauss(sqrt(mean))+.5) is returned */
  uint32_t poisson(double mean);
  
private:
  int32_t rnd250() {
    rpoint = (rpoint+1) & 255;          // Index erhoehen
    return rfield[rpoint] =  rfield[(rpoint-250) & 255]
                             ^ rfield[(rpoint-103) & 255];
  }
  
  void seed250(int32_t seed);
};

/// The global Rnd object
extern Rnd rnd;
}


//===========================================================================
//
/// a little inotify wrapper
//

struct Inotify{
  int fd, wd;
  char *buffer;
  uint buffer_size;
  MT::FileToken *fil;
  Inotify(const char *filename);
  ~Inotify();
  bool pollForModification(bool block=false, bool verbose=false);

  void waitAndReport(){ pollForModification(false, true); }
  void waitForModification(bool verbose=false){ while(!pollForModification(true, verbose)); }
};


//===========================================================================
//
/// a basic mutex lock
//

struct Mutex {
#ifndef MT_MSVC
  pthread_mutex_t mutex;
#endif
  int state; ///< 0=unlocked, otherwise=syscall(SYS_gettid)
  uint recursive; ///< number of times it's been locked
  Mutex();
  ~Mutex();
  void lock();
  void unlock();
};


//===========================================================================
//
/// a generic singleton
//

template<class T>
struct Singleton {
  static T *singleton;

  T *getSingleton() const {
    if(!singleton) {
      static Mutex m;
      m.lock();
      if(!singleton) singleton = new T;
      m.unlock();
    }
    return singleton;
  }

  ~Singleton(){
    if(singleton) {
//      static Mutex m; //pthread might already be deinitialized...
//      m.lock();
      T *mine=singleton;
      singleton=NULL;
      if(mine) delete mine;
//      m.unlock();
    }
  }

  T& operator()() const{ return *getSingleton(); }
};
template<class T> T *Singleton<T>::singleton=NULL;


//===========================================================================
//
/// a mutexed cout
//

extern Mutex coutMutex;
struct CoutToken{
  CoutToken(){ coutMutex.lock(); }
  ~CoutToken(){ coutMutex.unlock(); }
  std::ostream& getOs(){ return std::cout; }
};
#define COUT (CoutToken().getOs())


//===========================================================================

struct GlobalThings {
  std::ifstream cfgFile;
  bool cfgFileOpen=false;
  Mutex cfgFileMutex;
};

extern Singleton<GlobalThings> globalThings;

//===========================================================================
//
/// running code on init (in cpp files)
//

#define RUN_ON_INIT_BEGIN(key) struct key##_RUN_ON_INIT{ key##_RUN_ON_INIT(){
#define RUN_ON_INIT_END(key)   } } key##_RUN_ON_INIT_dummy;


//===========================================================================
//
// gnuplot calls
//

void gnuplot(const char *command, bool pauseMouse=false, bool persist=false, const char* PDFfile=NULL);
void gnuplotClose();


//===========================================================================
//
// Stefan's misc
//

/// Clip the `value` of n between `lower` and `upper`.
template <typename T> T clip(T& x, const T& lower, const T& upper) {
  if(x<lower) x=lower; else if(x>upper) x=upper; return x;
}

std::string getcwd_string();

//===========================================================================
//
// USING's
//

using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::ostream;
using std::istream;
using std::ofstream;
using std::ifstream;
using MT::rnd;
using MT::String;

#endif

/// @} //end group

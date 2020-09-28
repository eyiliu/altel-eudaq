#ifndef _JADE_JADEREAD_HH_
#define _JADE_JADEREAD_HH_

#include "JadeSystem.hh"
#include "JadeFactory.hh"
#include "JadeOption.hh"
#include "JadeUtils.hh"

#include "JadeDataFrame.hh"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <string>
#include <chrono>
#include <mutex>
#include <queue>

class JadeReader;
using JadeReaderSP = JadeFactory<JadeReader>::SP;
using JadeReaderUP = JadeFactory<JadeReader>::UP;

#ifndef JADE_DLL_EXPORT
extern template class DLLEXPORT JadeFactory<JadeReader>;
extern template DLLEXPORT
std::unordered_map<std::type_index, typename JadeFactory<JadeReader>::UP (*)(const JadeOption&)>&
JadeFactory<JadeReader>::Instance<const JadeOption&>();
#endif

class DLLEXPORT JadeReader{
 public:
  JadeReader(const JadeOption &opt);
  virtual ~JadeReader();
  static JadeReaderSP Make(const std::string&name, const JadeOption &opt);

  //open data device for read
  virtual void Open(){};
  //close data device
  virtual void Close(){};
  //read a data frame with 'timeout'
  virtual JadeDataFrameSP Read(const std::chrono::milliseconds &timeout);
  virtual std::vector<JadeDataFrameSP> Read(size_t size_max_pkg,
                                            const std::chrono::milliseconds &timeout_idel,
                                            const std::chrono::milliseconds &timeout_total) {return std::vector<JadeDataFrameSP>();};  
};


#endif

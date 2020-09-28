#ifndef _JADE_JADEUTILS_HH_
#define _JADE_JADEUTILS_HH_

#include <typeindex>
#include <unordered_map>
#include <map>
#include <string>
#include <chrono>
#include <memory>
#include <iostream>
#include <string>
#include <cstdio>

#include "JadeSystem.hh"

class DLLEXPORT JadeUtils{
public:
  static std::type_index GetTypeIndex(const std::string& name);
  static bool SetTypeIndex(const std::type_index& index);
  static bool SetTypeIndex(const std::string& name, const std::type_index& index);
  static std::string NameDemangle( const std::string&  man);
  static std::unordered_map<std::string, std::type_index>& TypeIndexMap();
  static void PrintTypeIndexMap();
  static std::string LoadFileToString(const std::string &path);
  static std::string GetNowStr();
  static std::string GetNowStr(const std::string &format);
  static std::string GetBinaryPath();
  static bool LoadBinary(const std::string& file);

  static std::string ToHexString(const char *bin, int len);
  static std::string ToHexString(const std::string &bin);
  static std::string ToHexString(char bin);
  
  
  inline std::string GetThisBinaryPath(){
#ifdef _WIN32
    void* address_return = _ReturnAddress();
    HMODULE handle = NULL;
    ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
			|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			static_cast<LPCSTR>(address_return), &handle);
    char modpath[MAX_PATH] = {'\0'};
    ::GetModuleFileNameA(handle, modpath, MAX_PATH);
    return modpath;
#else
    void* address_return = (void*)(__builtin_return_address(0));
    Dl_info dli;
    dli.dli_fname = 0;
    dladdr(address_return, &dli);
    return dli.dli_fname;
#endif
  }
  
  template<typename ... Args>
  static std::string FormatString( const std::string& format, Args ... args ){
    std::size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    std::unique_ptr<char[]> buf( new char[ size ] ); 
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 );
  }

  template<typename ... Args>
  static std::size_t FormatPrint(std::ostream &os, const std::string& format, Args ... args ){
    std::size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    std::unique_ptr<char[]> buf( new char[ size ] ); 
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    std::string formated_string( buf.get(), buf.get() + size - 1 );
    os<<formated_string<<std::flush;
    return formated_string.size();
  }
  
};

#endif

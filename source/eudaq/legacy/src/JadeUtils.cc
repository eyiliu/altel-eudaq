#include "JadeUtils.hh"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "demangle.hh"

std::unordered_map<std::string, std::type_index>& JadeUtils::TypeIndexMap(){
  static std::unordered_map<std::string, std::type_index> s_um_name_typeindex;
  return s_um_name_typeindex;
}

std::type_index JadeUtils::GetTypeIndex(const std::string& name){
  return TypeIndexMap().at(name);
}

bool JadeUtils::SetTypeIndex(const std::string& name, const std::type_index& index){
  if(TypeIndexMap().count(name)) {
    std::cerr<<"JadeUtil:: ERROR, name "
             << name
             << " have been registered by "
             << TypeIndexMap().at(name).name()<<"\n";
    return false;
  }
  TypeIndexMap().count(name);
  TypeIndexMap().insert({name, index});
  return true;  
}

bool JadeUtils::SetTypeIndex(const std::type_index& index){
  std::string demangled_name = NameDemangle(index.name());
  return SetTypeIndex(demangled_name, index);
}

std::string JadeUtils::NameDemangle(const std::string& mang) {
  const static std::string zprefix_s("__Z");
  const static std::string zprefix_l("____Z");

  const char* s = mang.c_str();
  const char* cxa_in = s;
  
  if (mang.compare(0, zprefix_s.size() , zprefix_s) == 0 ||
      mang.compare(0, zprefix_l.size() , zprefix_l) == 0 ){
    cxa_in += 1;
  }
  //if(0){
  if (char* lx = __cxa_demangle(cxa_in, NULL, NULL, NULL)) {
    std::string demang(lx);    
    free(lx);
    return demang;
  } else if (char* ms = __unDName(NULL, s, 0, &malloc, &free, 0)) {
    std::string demang(ms);
    free(ms);
    return demang;
  } else {
    const static std::string win_class_prefix("class "); //hotfix
    if(mang.compare(0, win_class_prefix.size(), win_class_prefix)==0){
      return mang.substr(win_class_prefix.size());
    }
    return mang;
  }  
}

void JadeUtils::PrintTypeIndexMap(){
  auto& um_name_typeindex = TypeIndexMap();
  std::cout<<"\n\n===list of type index===\n";
  for(auto &item: um_name_typeindex){
    std::cout<<item.first<<" = "<<item.second.name()<<"\n";
  }
  std::cout<<"\n\n"<<std::endl;
}


std::string JadeUtils::LoadFileToString(const std::string& path){
  std::ifstream ifs(path);
  if(!ifs.good()){
    std::cerr<<"JadeUtils: ERROR, unable to load file<"<<path<<">\n";
    throw;
  }
  
  std::string str;
  str.assign((std::istreambuf_iterator<char>(ifs) ),
             (std::istreambuf_iterator<char>()));
  return str;
}

std::string JadeUtils::GetNowStr(){
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss<<std::put_time(std::localtime(&now_c), "%c");
    return ss.str();
}

std::string JadeUtils::GetNowStr(const std::string &format){
  // "%y%m%d%H%M%S"
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss<<std::put_time(std::localtime(&now_c), format.c_str());
  return ss.str();
}


std::string JadeUtils::GetBinaryPath(){
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


bool JadeUtils::LoadBinary(const std::string& file){
  void *handle;
#ifdef _WIN32
  handle = (void *)LoadLibrary(file.c_str());
#else
  handle = dlopen(file.c_str(), RTLD_NOW);
#endif
  if(handle){
    // m_modules[file]=handle;
    return true;
  }
  else{
    std::cerr<<"JadeUtils: Fail to load module binary<"<<file<<">\n";
    char *errstr;
#ifdef _WIN32
    //copied from https://mail.python.org/pipermail//pypy-commit/2012-October/066804.html
    static char buf[32];
    DWORD dw = GetLastError();
    if (dw == 0)
      return NULL;
    sprintf(buf, "error 0x%x", (unsigned int)dw);
    //TODO get human readable https://msdn.microsoft.com/en-us/library/windows/desktop/ms679351(v=vs.85).aspx
    errstr = buf;
#else
    errstr = dlerror();
#endif	    
    printf ("A dynamic linking error occurred: (%s)\n", errstr); //Give the poor user some way to know what went wrong...
    return false;
  }
}




std::string JadeUtils::ToHexString(const char *bin, int len){
  constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
			     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  const unsigned char* data = (const unsigned char*)bin;
  std::string s(len * 2, ' ');
  for (int i = 0; i < len; ++i) {
    s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
    s[2 * i + 1] = hexmap[data[i] & 0x0F];
  }
  return s;
}


std::string JadeUtils::ToHexString(const std::string& bin){
  constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
			     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  const unsigned char *data = (const unsigned char*)bin.data();
  int len = bin.size();
  std::string s(len * 2, ' ');
  for (int i = 0; i < len; ++i) {
    s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
    s[2 * i + 1] = hexmap[data[i] & 0x0F];
  }
  return s;
}


std::string JadeUtils::ToHexString(char bin){
  constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
			     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  unsigned char data = (unsigned char)bin;
  std::string s(2, ' ');
  s[0] = hexmap[(data & 0xF0) >> 4];
  s[1] = hexmap[data & 0x0F];
  return s;
}

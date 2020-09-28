#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>

#include <signal.h>

#include "FirmwarePortal.hh"
#include "Telescope.hh"

#include "getopt.h"
#include "linenoise.h"


static  const std::string help_usage
(R"(
Usage:
-c json_file: path to json file
-i ip_address: eg. 131.169.133.170 for alpide_0 \n\
-h : print usage information, and then quit
)"
 );


static  const std::string help_usage_linenoise
(R"(

keyword: help, info, quit, sensor, firmware, set, get

example: 
  1) get firmware regiester
   > firmware get FW_REG_NAME

  2) set firmware regiester
   > firmware set FW_REG_NAME 10

  3) get sensor regiester
   > sensor get SN_REG_NAME

  4) set sensor regiester
   > sensor set SN_REG_NAME 10

  5) exit/quit command line
   > quit

  6) get ip address (base) from firmware regiesters
   > info

)"
 );


int main(int argc, char **argv){
  
  std::string c_opt;
  std::string i_opt;
  int c;
  while ( (c = getopt(argc, argv, "c:i:h")) != -1) {
    switch (c) {
    case 'c':
      c_opt = optarg;
      break;
    case 'i':
      i_opt = optarg;
      break;
    case 'h':
      fprintf(stdout, "%s", help_usage.c_str());
      return 0;
      break;
    default:
      fprintf(stderr, "%s", help_usage.c_str());
      return 1;
    }
  }
  
  if (optind < argc) {
    fprintf(stderr, "\ninvalid options: ");
    while (optind < argc)
      fprintf(stderr, "%s\n", argv[optind++]);;
    return 1;
  }

  ////////////////////////
  //test if all opts
  if(c_opt.empty() || i_opt.empty()){
    fprintf(stderr, "\ninsufficient options.\n%s\n\n\n",help_usage.c_str());
    return 1;
  }
  ///////////////////////
  
  std::string json_file_path = c_opt;
  std::string ip_address_str = i_opt;
  
  ///////////////////////
  std::string file_context = FirmwarePortal::LoadFileToString(json_file_path);

  FirmwarePortal fw(file_context, ip_address_str);
  
  const char* linenoise_history_path = "/tmp/.alpide_cmd_history";
  linenoiseHistoryLoad(linenoise_history_path);
  linenoiseSetCompletionCallback([](const char* prefix, linenoiseCompletions* lc)
                                 {
                                   static const char* examples[] =
                                     {"help", "info",
                                      "quit", "sensor", "firmware", "set", "get",
                                      NULL};
                                   size_t i;
                                   for (i = 0;  examples[i] != NULL; ++i) {
                                     if (strncmp(prefix, examples[i], strlen(prefix)) == 0) {
                                       linenoiseAddCompletion(lc, examples[i]);
                                     }
                                   }
                                 } );
  
  const char* prompt = "\x1b[1;32malpide\x1b[0m> ";
  while (1) {
    char* result = linenoise(prompt);
    if (result == NULL) {
      break;
    }    
    if ( std::regex_match(result, std::regex("\\s*(quit)\\s*")) ){
      printf("quiting \n");
      linenoiseHistoryAdd(result);
      free(result);
      break;
    }
    else if ( std::regex_match(result, std::regex("\\s*(help)\\s*")) ){
      fprintf(stdout, "%s", help_usage_linenoise.c_str());
      linenoiseHistoryAdd(result);
      free(result);
      break;
    }

    else if ( std::regex_match(result, std::regex("\\s*(sensor)\\s+(set)\\s+(\\w+)\\s+(?:(0[Xx])?([0-9]+))\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(sensor)\\s+(set)\\s+(\\w+)\\s+(?:(0[Xx])?([0-9]+))\\s*"));
      std::string name = mt[3].str();
      uint64_t value = std::stoull(mt[5].str(), 0, mt[4].str().empty()?10:16);
      fw.SetAlpideRegister(name, value);
    }
    else if ( std::regex_match(result, std::regex("\\s*(sensor)\\s+(get)\\s+(\\w+)\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(sensor)\\s+(get)\\s+(\\w+)\\s*"));
      std::string name = mt[3].str();
      uint64_t value = fw.GetAlpideRegister(name);
      fprintf(stderr, "%s = %u, %#x\n", name.c_str(), value, value);
    }
    
    else if ( std::regex_match(result, std::regex("\\s*(firmware)\\s+(set)\\s+(\\w+)\\s+(?:(0[Xx])?([0-9]+))\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(firmware)\\s+(set)\\s+(\\w+)\\s+(?:(0[Xx])?([0-9]+))\\s*"));
      std::string name = mt[3].str();
      uint64_t value = std::stoull(mt[5].str(), 0, mt[4].str().empty()?10:16);
      fw.SetFirmwareRegister(name, value);
    }
    else if ( std::regex_match(result, std::regex("\\s*(firmware)\\s+(get)\\s+(\\w+)\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(firmware)\\s+(get)\\s+(\\w+)\\s*"));
      std::string name = mt[3].str();
      uint64_t value = fw.GetFirmwareRegister(name);
      fprintf(stderr, "%s = %u, %#x\n", name.c_str(), value, value);
    }
    else if (!strncmp(result, "info", 5)){
      uint32_t ip0 = fw.GetFirmwareRegister("IP0");
      uint32_t ip1 = fw.GetFirmwareRegister("IP1");
      uint32_t ip2 = fw.GetFirmwareRegister("IP2");
      uint32_t ip3 = fw.GetFirmwareRegister("IP3");
      std::cout<<"\n\ncurrent ip  " <<ip0<<":"<<ip1<<":"<<ip2<<":"<<ip3<<"\n\n"<<std::endl;
    }
    linenoiseHistoryAdd(result);
    free(result);
  }

  linenoiseHistorySave(linenoise_history_path);
  linenoiseHistoryFree();
}

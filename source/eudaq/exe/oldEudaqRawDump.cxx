#include <cstdio>
#include <csignal>

#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>
#include <thread>
#include <regex>

#include <unistd.h>
#include <getopt.h>

#include "eudaq/FileReader.hh"

namespace{
  std::string CStringToHexString(const char *bin, int len){
    constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                               '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    const unsigned char* data = (const unsigned char*)(bin);
    std::string s(len * 2, ' ');
    for (int i = 0; i < len; ++i) {
      s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
      s[2 * i + 1] = hexmap[data[i] & 0x0F];
    }
    return s;
  }

  std::string StringToHexString(const std::string bin){
    return CStringToHexString(bin.data(), bin.size());
  }
}


static const std::string help_usage = R"(
Usage:
  -help            help message
  -verbose         verbose flag

  -file     [path] input data file
  -eventNumber [n] event Number to dump hex

quick examples:
  oldEudaqRawDump -file ~/testbeam/run002012_200227184925.raw -eventNumber 100
)";


static sig_atomic_t g_done = 0;
int main(int argc, char ** argv) {

  int do_quit = false;
  int do_help = false;
  uint32_t verbose_level = 0;

  struct option longopts[] =
    {
     { "help",        no_argument,       &do_help,      1  },
     { "verbose",     optional_argument, NULL,         'v' },
     { "file",        required_argument, NULL,         'f' },
     { "quit",        no_argument,       &do_quit,      1  },
     { "eventNumber", required_argument, NULL,         'e' },
     { 0, 0, 0, 0 }};

  std::string opt_str_file;
  uint32_t opt_event_number;
  uint32_t opt_has_event_number = false;

  int c;
  opterr = 1;
  while ((c = getopt_long_only(argc, argv, "", longopts, NULL))!= -1) {
    switch (c) {
    case 'v':{
      verbose_level = 2;
      if(optarg != NULL){
        verbose_level = static_cast<uint32_t>(std::stoull(optarg, 0, 10));
      }
      break;
    }
    case 'f':
        opt_str_file = optarg;
      break;
    case 'e':
      opt_event_number = static_cast<uint32_t>(std::stoull(optarg, 0, 10));
      opt_has_event_number = true;
      break;
      ////////////////
    case 0: /* getopt_long() set a variable, just keep going */
      break;
    case 1:
      std::fprintf(stderr,"case 1\n");
      exit(1);
      break;
    case ':':
      std::fprintf(stderr,"case :\n");
      exit(1);
      break;
    case '?':
      std::fprintf(stderr,"case ?\n");
      std::fprintf(stdout,"%s\n", help_usage.c_str());
      exit(1);
      break;
    default:
      std::fprintf(stderr, "case default, missing branch in switch-case\n");
      exit(1);
      break;
    }
  }

  if(do_help){
    std::fprintf(stdout, "%s\n", help_usage.c_str());
    exit(0);
  }

  if(opt_str_file.empty()){
    std::fprintf(stderr, "Error. please specify a input data file by option\n");
    std::fprintf(stderr, "%s\n", help_usage.c_str());
    exit(1);
  }

  if(!opt_has_event_number){
    std::fprintf(stderr, "Error. please specify an event number by option\n");
    std::fprintf(stderr, "%s\n", help_usage.c_str());
    exit(1);
  }

  signal(SIGINT, [](int){g_done+=1;});

  auto reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), opt_str_file);

  while (!g_done) {
    auto ev = reader->GetNextEvent();
    if(!ev){
      std::fprintf(stdout, "end of file\n");
      break;
    }
    uint32_t eventN = ev->GetEventN();
    if(eventN == opt_event_number){
      auto sub_evs =  ev->GetSubEvents();
      std::fprintf(stdout, "eventN:%d\n", eventN);
      for(const auto& sub_ev: sub_evs ){
        if(sub_ev->GetDescription()=="TluRawDataEvent"){
          uint32_t triggerN = sub_ev->GetTriggerN();
          std::fprintf(stdout, "triggerN_TLU:%d\n", triggerN);
        }
      }
      for(const auto& sub_ev: sub_evs ){
        if(sub_ev->GetDescription()=="JadeRaw"){
          uint32_t streamN = sub_ev->GetStreamN();
          std::vector<uint8_t> block_raw = sub_ev->GetBlock(0);
          std::string raw_hex = CStringToHexString( reinterpret_cast<const char*>(block_raw.data()), block_raw.size());
          std::fprintf(stdout, "streamN:%d  dataHex:\n%s\n", streamN, raw_hex.c_str());
        }
      }
      // ev->Print(std::cout);
      break;
    }
  }
  return 0;
}

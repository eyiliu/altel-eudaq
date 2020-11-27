#include "getopt.h"

#include <iostream>
#include <memory>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <csignal>

#include "eudaq/FileReader.hh"
#include "eudaq/RawEvent.hh"

#include "TelEvent.hpp"
#include "FEI4Helper.hh"

#include "myrapidjson.h"

template<typename T>
  static void PrintJson(const T& o){
    rapidjson::StringBuffer sb;
    // rapidjson::PrettyWriter<rapidjson::StringBuffer> w(sb);
    rapidjson::Writer<rapidjson::StringBuffer> w(sb);
    o.Accept(w);
    std::fwrite(sb.GetString(), 1, sb.GetSize(), stdout);
    std::fputc('\n', stdout);
}

static  const std::string help_usage=R"(
  Usage:
  -help                          help message
  -verbose                       verbose flag
  -rawFile  [rawfile0 <rawfile1> <rawfile2> ... ]
                                 paths to input eudaq raw data files (input)
  -hitFile  [jsonfile]           path to output hit file (output)
)";

int main(int argc, char* argv[]) {
  rapidjson::CrtAllocator jsa;

  int do_help = false;
  int do_verbose = false;
  struct option longopts[] =
    {
     { "help",       no_argument,       &do_help,      1  },
     { "verbose",    no_argument,       &do_verbose,   1  },
     { "rawFile",    required_argument, NULL,          'f' },
     { "hitFile",    required_argument, NULL,          'o' },
     { 0, 0, 0, 0 }};

  std::vector<std::string> rawFilePathCol;
  std::string hitFilePath;

  int c;
  opterr = 1;
  while ((c = getopt_long_only(argc, argv, "", longopts, NULL))!= -1) {
    switch (c) {
    case 'h':
      do_help = 1;
      std::fprintf(stdout, "%s\n", help_usage.c_str());
      exit(0);
      break;
    case 'f':{
      optind--;
      for( ;optind < argc && *argv[optind] != '-'; optind++){
        // printf("optind %d\n", optind);
        // for(int i = optind; i<argc; i++){
        //   printf("%s ", argv[i]);
        // }
        // printf("\n", optind);
        const char* fileStr = argv[optind];
        rawFilePathCol.push_back(std::string(fileStr));
      }
      break;
    }
    case 'o':
      hitFilePath = optarg;
      break;

      /////generic part below///////////
    case 0: /* getopt_long() set a variable, just keep going */
      break;
    case 1:
      fprintf(stderr,"case 1\n");
      exit(1);
      break;
    case ':':
      fprintf(stderr,"case :\n");
      exit(1);
      break;
    case '?':
      fprintf(stderr,"case ?\n");
      exit(1);
      break;
    default:
      fprintf(stderr, "case default, missing branch in switch-case\n");
      exit(1);
      break;
    }
  }

  if(rawFilePathCol.empty() || hitFilePath.empty()){
    std::fprintf(stderr, "%s\n", help_usage.c_str());
    exit(0);
  }

  std::fprintf(stdout, "%d input raw files:\n", rawFilePathCol.size());
  for(auto &rawfilepath: rawFilePathCol){
    std::fprintf(stdout, "  %s  ", rawfilepath.c_str());
  }
  std::fprintf(stdout, "\n");

  eudaq::FileReaderUP reader;
  std::FILE* fd = std::fopen(hitFilePath.c_str(), "wb");

  uint32_t event_count = 0;
  uint32_t rawFileN=0;
  uint32_t hitFile_eventN=0;

  bool isFirstEvent = true;
  while(1){
    if(!reader){
      if(rawFileN<rawFilePathCol.size()){
        std::fprintf(stdout, "processing raw file: %s\n", rawFilePathCol[rawFileN].c_str());
        reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), rawFilePathCol[rawFileN]);
        rawFileN++;
      }
      else{
        std::fprintf(stdout, "processed %d raw files, quit\n", rawFileN);
        break;
      }
    }
    auto ev = reader->GetNextEvent();
    if(!ev){
      reader.reset();
      continue; // goto for next raw file
    }
    event_count++;

    eudaq::EventSPC ev_altel;
    eudaq::EventSPC ev_fei4;

    uint32_t eventN = ev->GetEventN();
    uint32_t triggerN = ev->GetTriggerN();
    altel::TelEvent telev(0, eventN, 0, triggerN);

    if(ev->IsFlagPacket()){
      auto subev_col = ev->GetSubEvents();
      for(auto& subev: subev_col){
        if(subev->GetDescription() == "AltelRaw"){
          ev_altel = subev;
        }
        if(subev->GetDescription() == "USBPIXI4"){
          ev_fei4 = subev;
        }
      }
      subev_col.clear();
    }
    else if(ev->GetDescription() == "AltelRaw"){
      ev_altel = ev;
    }
    if(!ev_altel){
      continue;
    }

    if(ev_fei4){
      auto ev_raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(ev_fei4);
      auto block_n_list = ev_raw->GetBlockNumList();
      if(block_n_list.size()>1){
        throw;
      }
      if(!block_n_list.empty()){
        auto data = ev_raw->GetBlock(block_n_list[0]);

        std::vector<std::pair<uint16_t, uint16_t>> uvs = FEI4Helper::GetMeasRawUVs(data);
        std::vector<altel::TelMeasRaw> feiMeasRaws;
        for(auto & [uraw, vraw] : uvs){
          feiMeasRaws.emplace_back(uraw, vraw, 101, triggerN); //fei4 detN=101
        }
        auto feiMeasHits = altel::TelMeasHit::clustering_UVDCus(feiMeasRaws,
                                                                FEI4Helper::pitchU,
                                                                FEI4Helper::pitchV,
                                                                -FEI4Helper::pitchU*(FEI4Helper::numPixelU-1)*0.5,
                                                                -FEI4Helper::pitchV*(FEI4Helper::numPixelV-1)*0.5);

        telev.measRaws().insert(telev.measRaws().end(), feiMeasRaws.begin(), feiMeasRaws.end());
        telev.measHits().insert(telev.measHits().end(), feiMeasHits.begin(), feiMeasHits.end());
      }
    }

    auto ev_altelraw = std::dynamic_pointer_cast<const eudaq::RawEvent>(ev_altel);
    size_t nblocks= ev_altelraw->NumBlocks();
    auto block_n_list = ev_altelraw->GetBlockNumList();
    if(!nblocks)
      throw;

    for(const auto& blockNum: block_n_list){
      auto rawblock = ev_altelraw->GetBlock(blockNum);
      uint32_t* p_block = reinterpret_cast<uint32_t*>(rawblock.data());
      uint32_t layerID = *p_block;

      p_block++;
      uint32_t cluster_size =  *p_block;
      p_block++;

      std::vector<altel::TelMeasRaw> alpideMeasRaws;
      for(size_t i = 0; i < cluster_size; i++){
        float cluster_x_mm = *(reinterpret_cast<float*>(p_block));
        p_block++;
        float cluster_y_mm = *(reinterpret_cast<float*>(p_block));
        p_block++;
        uint32_t pixel_size =  *p_block;
        p_block++;

        for(size_t j = 0; j < pixel_size; j++){
          uint32_t pixelXY =  *p_block;
          p_block++;
          uint16_t pixelX = static_cast<uint16_t>(pixelXY);
          uint16_t pixelY = static_cast<uint16_t>(pixelXY>>16);
          alpideMeasRaws.emplace_back(pixelX, pixelY, layerID, triggerN);
        }
      }
      auto alpideMeasHits = altel::TelMeasHit::clustering_UVDCus(alpideMeasRaws,
                                                                 0.02924,
                                                                 0.02688,
                                                                 -0.02924*(1024-1)*0.5,
                                                                 -0.02688*(512-1)*0.5);

      telev.measRaws().insert(telev.measRaws().end(), alpideMeasRaws.begin(), alpideMeasRaws.end());
      telev.measHits().insert(telev.measHits().end(), alpideMeasHits.begin(), alpideMeasHits.end());
    }

    if(!isFirstEvent){
      std::fprintf(fd, ",\n");
    }
    else{
      isFirstEvent=false;
    }
    // std::fprintf(stdout, "\nevent #%d trigger #%d: ", telev.eveN(), telev.clkN());
    std::fprintf(fd, "{\"layers\":[\n");
    uint16_t detN_last = -1;
    for(auto &aMeasHit : telev.measHits()){
      uint16_t detN = aMeasHit->detN();
      bool isFirstHit = false;

      if(detN!= detN_last){
        if(detN_last!= uint16_t(-1) ){
          std::fprintf(fd, " ]},\n");
        }
        detN_last = detN;
        std::fprintf(fd," {\"det\":\"%s\",\"ver\":5,\"tri\":%d,\"cnt\":%d,\"ext\":%d,\"hit\":[",
                     (detN==101)?"fei4":"alpide", telev.clkN(), telev.eveN(), detN);
        isFirstHit = true;
      }
      if(!isFirstHit){
        std::fprintf(fd, ",");
      }
      std::fprintf(fd, "{\"pos\":[%f,%f,%d],", aMeasHit->u(), aMeasHit->v(), aMeasHit->detN());
      std::fprintf(fd, "\"pix\":[");
      bool isFirstPixel = true;
      for(auto &aMeasRaw: aMeasHit->measRaws()){
        if(!isFirstPixel){
          std::fprintf(fd, ",");
        }
        else{
          isFirstPixel = false;
        }
        std::fprintf(fd, "[%d,%d,%d]", aMeasRaw.u(), aMeasRaw.v(), aMeasRaw.detN());
      }
      std::fprintf(fd, "]}");
    }
    std::fprintf(fd, " ]}\n");
    std::fprintf(fd, "]}");
    hitFile_eventN++;
  }
  std::fclose(fd);
  std::fprintf(stdout, "input events %d, output events %d\n", event_count,  hitFile_eventN);
  return 0;
}

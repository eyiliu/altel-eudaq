#include "getopt.h"

#include <iostream>
#include <memory>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <csignal>

#include "DataFrame.hh"

#include "eudaq/FileReader.hh"
#include "eudaq/RawEvent.hh"

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
  rapidjson::StringBuffer js_sb;
  rapidjson::Writer<rapidjson::StringBuffer> js_writer;
  js_writer.SetMaxDecimalPlaces(5);
  bool is_first_write = true;
  std::fwrite(reinterpret_cast<const char *>("[\n"), 1, 2, fd);

  uint32_t event_count = 0;

  uint32_t rawFileN=0;
  uint32_t hitFile_eventN=0;

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
    if(do_verbose)
      ev->Print(std::cout);

    eudaq::EventSPC ev_altel;

    if(ev->IsFlagPacket()){
      auto subev_col = ev->GetSubEvents();
      for(auto& subev: subev_col){
        if(subev->GetDescription() == "AltelRaw"){
          ev_altel = subev;
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

    auto ev_altelraw = std::dynamic_pointer_cast<const eudaq::RawEvent>(ev_altel);
    size_t nblocks= ev_altelraw->NumBlocks();
    auto block_n_list = ev_altelraw->GetBlockNumList();
    if(!nblocks)
      throw;

    uint32_t eventN = ev_altelraw->GetEventN();
    uint32_t triggerN = ev_altelraw->GetTriggerN();

    std::vector<altel::DataFrame> df_col;
    df_col.reserve(block_n_list.size());
    for(const auto& blockNum: block_n_list){
      auto rawblock = ev->GetBlock(blockNum);
      uint32_t* p_block = reinterpret_cast<uint32_t*>(rawblock.data());
      uint32_t layerID = *p_block;

      altel::DataFrame df;
      df.m_counter=eventN;
      df.m_trigger=triggerN;
      df.m_level_decode = 5;
      df.m_extension = layerID;

      p_block++;
      uint32_t cluster_size =  *p_block;
      p_block++;

      for(size_t i = 0; i < cluster_size; i++){
        float cluster_x_mm = *(reinterpret_cast<float*>(p_block));
        p_block++;
        float cluster_y_mm = *(reinterpret_cast<float*>(p_block));
        p_block++;
        uint32_t pixel_size =  *p_block;
        p_block++;
        std::vector<altel::PixelHit> pixelhits;
        pixelhits.reserve(pixel_size);
        for(size_t j = 0; j < pixel_size; j++){
          uint32_t pixelXY =  *p_block;
          p_block++;
          uint16_t pixelX = static_cast<uint16_t>(pixelXY);
          uint16_t pixelY = static_cast<uint16_t>(pixelXY>>16);
          pixelhits.emplace_back(pixelX,pixelY,layerID);
        }
        altel::ClusterHit clusterhit(std::move(pixelhits));
        clusterhit.buildClusterCenter();
        df.m_clusters.push_back(std::move(clusterhit));
      }
      df_col.push_back(std::move(df));
    }

    js_sb.Clear();
    if(!is_first_write){
      std::fwrite(reinterpret_cast<const char *>(",\n"), 1, 2, fd);
    }
    js_writer.Reset(js_sb);
    js_writer.StartObject();

    rapidjson::PutN(js_sb, '\n', 1);
    js_writer.String("layers");
    js_writer.StartArray();
    for(auto& df: df_col){
      auto js_df = df.JSON(jsa);
      if(do_verbose)
        PrintJson(js_df);
      js_df.Accept(js_writer);
      rapidjson::PutN(js_sb, '\n', 1);
    }
    js_writer.EndArray();
    js_writer.EndObject();
    rapidjson::PutN(js_sb, '\n', 1);
    auto p_ch = js_sb.GetString();
    auto n_ch = js_sb.GetSize();
    std::fwrite(reinterpret_cast<const char *>(p_ch), 1, n_ch, fd);
    is_first_write=false;

    hitFile_eventN++;
  }

  std::fwrite(reinterpret_cast<const char *>("]\n"), 1, 2, fd);
  std::fclose(fd);
 
  std::fprintf(stdout, "input events %d, output events %d\n", event_count,  hitFile_eventN);
  return 0;
}

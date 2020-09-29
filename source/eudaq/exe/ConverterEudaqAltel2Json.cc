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
  -help              help message
  -verbose           verbose flag
  -file       [rawfile]    path to input eudaq raw data file
  -energy     [float]      energy of beam particle (not yet used)
  -geometry   [jsonfile]   puth to input geometry file (not yet used)
  -out        [jsonfile]   path to output json file
)";

int main(int argc, char* argv[]) {
  rapidjson::CrtAllocator jsa;

  int do_help = false;
  int do_verbose = false;
  struct option longopts[] =
    {
     { "help",       no_argument,       &do_help,      1  },
     { "verbose",    no_argument,       &do_verbose,   1  },
     { "file",       required_argument, NULL,          'f' },
     { "energy",     required_argument, NULL,          's' },
     { "geomerty",   required_argument, NULL,          'g' },
     { "out",        required_argument, NULL,          'o' },
     { 0, 0, 0, 0 }};


  std::string datafile_name;
  std::string outputfile_name;
  std::string geofile_name;
  double energy = 5;

  int c;
  opterr = 1;
  while ((c = getopt_long_only(argc, argv, "", longopts, NULL))!= -1) {
    switch (c) {
    case 'h':
      do_help = 1;
      std::fprintf(stdout, "%s\n", help_usage.c_str());
      exit(0);
      break;
    case 'f':
      datafile_name = optarg;
      break;
    case 'o':
      outputfile_name = optarg;
      break;
    case 'g':
      geofile_name = optarg;
      break;
    case 's':
      energy = std::stod(optarg);
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

  if(datafile_name.empty() || outputfile_name.empty()){
    std::fprintf(stderr, "%s\n", help_usage.c_str());
    exit(0);
  }

  eudaq::FileReaderUP reader;
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), datafile_name);

  std::FILE* fd = std::fopen(outputfile_name.c_str(), "wb");
  rapidjson::StringBuffer js_sb;
  rapidjson::Writer<rapidjson::StringBuffer> js_writer;
  js_writer.SetMaxDecimalPlaces(5);
  bool is_first_write = true;
  std::fwrite(reinterpret_cast<const char *>("[\n"), 1, 2, fd);

  uint32_t event_count = 0;

  while(1){
    auto ev = reader->GetNextEvent();
    if(!ev)
      break;
    event_count++;

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
  }

  std::fwrite(reinterpret_cast<const char *>("]\n"), 1, 2, fd);
  std::fclose(fd);

  return 0;
}

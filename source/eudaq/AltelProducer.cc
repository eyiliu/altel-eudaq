#include "eudaq/Producer.hh"

#include <list>
#include <iostream>
#include <chrono>
#include <thread>

#include "Telescope.hh"
using namespace std::chrono_literals;

namespace altel{
  class AltelProducer : public eudaq::Producer {
  public:
    using eudaq::Producer::Producer;
    ~AltelProducer() override {};
    void DoInitialise() override;
    void DoConfigure() override;
    void DoStartRun() override;
    void DoStopRun() override;
    void DoTerminate() override;
    void DoReset() override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("AltelProducer");

    void RunLoop() override;
  private:
    bool m_exit_of_run;
    std::unique_ptr<altel::Telescope> m_tel;
  };
}

namespace{
  auto _reg_ = eudaq::Factory<eudaq::Producer>::
    Register<altel::AltelProducer, const std::string&, const std::string&>(altel::AltelProducer::m_id_factory);
}


namespace{
  std::string LoadFileToString(const std::string& path){
    std::ifstream ifs(path);
    if(!ifs.good()){
      std::string bin_path_str = binaryPath()+"/"+path;
      std::ifstream ifs_bin(bin_path_str);
      if(ifs_bin.good()){
        ifs = std::move(ifs_bin);
      }
      else{
        std::cerr<<"LoadFileToString:: ERROR, unable to load file<"<<path<<">\n";
        std::cerr<<"LoadFileToString:: ERROR, unable to load file<"<<bin_path_str<<">\n";
        throw;
      }
    }

    std::string str;
    str.assign((std::istreambuf_iterator<char>(ifs) ),
               (std::istreambuf_iterator<char>()));
    return str;
  }
}

void altel::AltelProducer::DoInitialise(){
  std::cout<< "it is AltelProducer "<<std::endl;
  auto ini = GetInitConfiguration();
  std::string tel_json_conf_path = ini->Get("ALTEL_EUDAQ_CONF_JSON_FILE", "/tmp/altel_run_conf.json");
  m_tel.reset(new altel::Telescope(LoadFileToString(tel_json_conf_path)));
  m_tel->Init();
}

void altel::AltelProducer::DoConfigure(){
  //do nothing here
}

void altel::AltelProducer::DoStartRun(){
  m_tel->Start_no_tel_reading();
}

void altel::AltelProducer::DoStopRun(){
  m_tel->Stop();
  m_exit_of_run = true;
}

void altel::AltelProducer::DoReset(){
  m_tel.reset();
}

void altel::AltelProducer::DoTerminate(){
  std::terminate();
}

void altel::AltelProducer::RunLoop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  m_exit_of_run = false;
  while(!m_exit_of_run){
    auto ev_tel = m_tel->ReadEvent();
    if(ev_tel.empty()){
      std::this_thread::sleep_for(100us);
      continue;
    }

    auto ev_eudaq = eudaq::Event::MakeUnique("AltelRaw");
    uint64_t trigger_n = ev_tel.front()->GetTrigger();
    ev_eudaq->SetTriggerN(trigger_n);
    //ev_eudaq->SetTag("status", "something status");

    for(auto& e: ev_tel){
      uint32_t word32_count  = 2; // layerID_uint32, cluster_n_uint32
      for(auto &ch : e->m_clusters){
        word32_count += 3; // x_float, y_float , pixel_n_uint32
        word32_count += ch.pixelHits.size(); // pixel_xy_uint32,
      }
      std::vector<uint32_t> layer_block(word32_count);
      uint32_t* p_block = layer_block.data();
      uint32_t layerID = e->GetExtension();
      *p_block =  layerID;
      p_block++;
      uint32_t clsuter_size = e->m_clusters.size();
      *p_block = e->m_clusters.size();
      p_block ++;
      for(auto &ch : e->m_clusters){
        *(reinterpret_cast<float*>(p_block)) = ch.x();
        p_block ++;
        *(reinterpret_cast<float*>(p_block)) = ch.y();
        p_block ++;
        *p_block = ch.pixelHits.size();
        p_block ++;
        for(auto &ph : ch.pixelHits){
          // Y<< 16 + X
          *p_block =  uint32_t(ph.x()) + (uint32_t(ph.y())<<16);
          p_block ++;
        }
      }
      if(p_block - layer_block.data() != layer_block.size()){
        std::cerr<<"error altel data block"<<std::endl;
        throw;
      }
      ev_eudaq->AddBlock(layerID, layer_block);
    }
    SendEvent(std::move(ev_eudaq));
  }
}

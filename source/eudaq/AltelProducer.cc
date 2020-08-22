#include "eudaq/Producer.hh"

#include <list>
#include <iostream>
#include <chrono>
#include <thread>

#include "Telescope.hh"



template<typename ... Args>
static std::string FormatString( const std::string& format, Args ... args ){
  std::size_t size = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
  std::unique_ptr<char[]> buf( new char[ size ] ); 
  std::snprintf( buf.get(), size, format.c_str(), args ... );
  return std::string( buf.get(), buf.get() + size - 1 );
}


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
    void DoStatus() override;

    static const uint32_t m_id_factory = eudaq::cstr2hash("AltelProducer");

    void RunLoop() override;
  private:
    bool m_exit_of_run;
    std::unique_ptr<altel::Telescope> m_tel;

    std::atomic<uint64_t> m_tg_n_begin;
    std::atomic<uint64_t> m_tg_n_last;

    std::chrono::system_clock::time_point m_tp_run_begin;


    uint64_t m_st_n_tg_old;
    std::chrono::system_clock::time_point m_st_tp_old;
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
        std::cerr<<"LoadFileToString:: ERROR, unable to load file<"<<path<<">\n";
	throw;
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

  m_tp_run_begin = std::chrono::system_clock::now();
  auto tp_start_run = std::chrono::steady_clock::now();
  m_exit_of_run = false;
  bool is_first_event = true;
  while(!m_exit_of_run){
    auto ev_tel = m_tel->ReadEvent();
    if(ev_tel.empty()){
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      continue;
    }
    auto ev_eudaq = eudaq::Event::MakeUnique("AltelRaw");
    uint64_t trigger_n = ev_tel.front()->GetTrigger();
    ev_eudaq->SetTriggerN(trigger_n);

    std::map<uint32_t, uint32_t> map_layer_clusterN;

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
      uint32_t clusters_size = e->m_clusters.size();
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
      map_layer_clusterN[layerID]= clusters_size;
      ev_eudaq->AddBlock(layerID, layer_block);
    }
    SendEvent(std::move(ev_eudaq));
    
    if(is_first_event){
      is_first_event = false;
      m_tg_n_begin = trigger_n;
    }
    m_tg_n_last = trigger_n;    
  }
}

void AltelProducer::DoStatus() {
  if(m_exit_of_run){
    auto tp_now = std::chrono::system_clock::now();
    m_st_tp_old = tp_now;
    m_st_n_tg_old = 0;
  }

  if (!m_exit_of_run) {
    auto tp_now = std::chrono::system_clock::now();
    std::chrono::duration<double> dur_period_sec = tp_now - m_st_tp_old;
    std::chrono::duration<double> dur_accu_sec = tp_now - m_tp_run_begin;
    uint64_t st_n_tg_now = m_tg_n_last;
    uint64_t st_n_tg_begin = m_tg_n_begin;

    double sec_accu = dur_accu_sec.count();
    double sec_period = dur_period_sec.count();

    double st_hz_tg_accu = (st_n_tg_now - st_n_tg_begin) / sec_accu ;
    double st_hz_tg_period = (st_n_tg_now - m_st_n_tg_old) / sec_period ;

    SetStatusTag("TriggerID(latest:first)", FormatString("%u:%u", st_n_tg_now, st_n_tg_begin));
    SetStatusTag("TriggerHz(per:avg)", FormatString("%.1f:%.1f", st_hz_tg_period, st_hz_tg_accu));
    // SetStatusTag("EventHz(per,avg)", std::to_string());
    // SetStatusTag("Cluster([layer:avg:per])", std::to_string());

    m_st_tp_old = tp_now;
    m_st_n_tg_old = st_n_tg_now;
  }
}

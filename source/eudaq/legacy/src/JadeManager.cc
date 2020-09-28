#include "JadeManager.hh"
#include "JadeFactory.hh"
#include "JadeUtils.hh"

using _base_c_ = JadeManager;
using _index_c_ = JadeManager;

template class DLLEXPORT JadeFactory<_base_c_>;
template DLLEXPORT
std::unordered_map<std::type_index, typename JadeFactory<_base_c_>::UP (*)(const JadeOption&)>&
JadeFactory<_base_c_>::Instance<const JadeOption&>();

namespace{
  auto _loading_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(_index_c_)));
  auto _loading_ = JadeFactory<_base_c_>::Register<_index_c_, const JadeOption&>(typeid(_index_c_));
}

using namespace std::chrono_literals;

JadeManager::JadeManager(const JadeOption &opt)
  : m_is_running(false), m_opt(opt){
  if(opt.GetIntValue("version") < 3){
    std::cerr<<"JadeManager: ERROR version missmatch with json configure file\n";
    throw;
  }  
}

JadeManager::~JadeManager(){
  if(m_is_running)
    StopThread();
}

JadeManagerSP JadeManager::Make(const std::string& name, const JadeOption& opt){  
  try{
    JadeUtils::PrintTypeIndexMap();  

    std::type_index index = JadeUtils::GetTypeIndex(name);
    JadeManagerSP wrt =  JadeFactory<JadeManager>::MakeUnique<const JadeOption&>(index, opt);
    return wrt;
  }
  catch(std::exception& e){
    std::cout<<"JadeManager::Make  exception  "<<e.what() <<std::endl;
    return nullptr;
  }
}

void JadeManager::MakeComponent(){
  auto rd_opt = m_opt.GetSubOption("JadeReader");
  if(!rd_opt.IsNull()){
    m_rd = JadeReader::Make(rd_opt.GetStringValue("type"), rd_opt.GetSubOption("parameter"));
  }
  auto ctrl_opt = m_opt.GetSubOption("JadeRegCtrl");
  if(!ctrl_opt.IsNull()){
    m_ctrl = JadeRegCtrl::Make(ctrl_opt.GetStringValue("type"), ctrl_opt.GetSubOption("parameter"));
  }  
  auto wrt_opt = m_opt.GetSubOption("JadeWriter");
  if(!wrt_opt.IsNull()){
    m_wrt = JadeWriter::Make(wrt_opt.GetStringValue("type"), wrt_opt.GetSubOption("parameter"));
  }
}

void JadeManager::RemoveComponent(){
  m_ctrl.reset();
  m_rd.reset();
  m_wrt.reset();
  //TODO: report if someone holds the any instances of released shared_ptr.
}

void JadeManager::Init(){
  MakeComponent();  
}

void JadeManager::StartDataTaking(){
  std::cout<<"openning rd"<<std::endl;
  if(m_rd){
    std::cout<<"openning"<<std::endl;
    m_rd->Open();
  }
  std::cout<<"openning ctrl"<<std::endl;
  if(m_ctrl){
    std::cout<<"openning"<<std::endl;
    m_ctrl->Open();
    
  }
  std::cout<<"openning wrt"<<std::endl;
  if(m_wrt) {
    std::cout<<"openning"<<std::endl;
    m_wrt->Open();
  }
  std::cout<<"open done"<<std::endl;
  StartThread();
  std::cout<<"thread started"<<std::endl;  
}

void JadeManager::StopDataTaking(){
  StopThread();
  if(m_rd) m_rd->Close();
  if(m_ctrl) m_ctrl->Close();
  if(m_wrt) m_wrt->Close();
}

void JadeManager::DeviceControl(const std::string &cmd)
{
  if(m_ctrl) m_ctrl->SendCommand(cmd);
}

uint64_t JadeManager::AsyncReading(){
  auto tp_start = std::chrono::system_clock::now();
  auto tp_print_prev = std::chrono::system_clock::now();
  uint64_t ndf_print_next = 10000;
  uint64_t ndf_print_prev = 0;
  uint64_t n_df = 0;
  while (m_is_running){
    auto df = m_rd? m_rd->Read(1000ms):nullptr;
    if(!df){
      continue;
    }
    std::unique_lock<std::mutex> lk_out(m_mx_ev_to_wrt);
    m_qu_ev_to_wrt.push(df);
    n_df ++;
    lk_out.unlock();
    m_cv_valid_ev_to_wrt.notify_all();
    if(n_df >= ndf_print_next){
      auto tp_now = std::chrono::system_clock::now();
      auto dur = tp_now - tp_start;
      double dur_sec_c = dur.count() * decltype(dur)::period::num * 1.0/ decltype(dur)::period::den;
      double av_hz = n_df/dur_sec_c;
      dur = tp_now - tp_print_prev;
      dur_sec_c = dur.count() * decltype(dur)::period::num * 1.0/ decltype(dur)::period::den;
      double cr_hz = (n_df-ndf_print_prev) / dur_sec_c;
      // std::cout<<"JadeManager:: data "<<n_df <<"  average data rate (reading)<"<< av_hz<<"> current data rate<"<<cr_hz<<">  ndf "<<n_df<<std::endl;
      ndf_print_next += 1000;
      ndf_print_prev = n_df;
      tp_print_prev = tp_now;
    }
  }
  return n_df;
}

uint64_t JadeManager::AsyncWriting(){
  auto tp_start = std::chrono::system_clock::now();
  auto tp_print_prev = std::chrono::system_clock::now();
  uint64_t ndf_print_next = 10000;
  uint64_t ndf_print_prev = 0;
  uint64_t n_df = 0;
  while(m_is_running){
    std::unique_lock<std::mutex> lk_in(m_mx_ev_to_wrt);
    while(m_qu_ev_to_wrt.empty()){
      while(m_cv_valid_ev_to_wrt.wait_for(lk_in, 10ms)
          ==std::cv_status::timeout){
        if(!m_is_running){
          return n_df;
        }
      }
    }
    auto df = m_qu_ev_to_wrt.front();
    m_qu_ev_to_wrt.pop();
    lk_in.unlock();
    // df->Decode();
    df->Decode(1); //level 1, header-only
    uint16_t tg =  df->GetCounter();
    // std::cout<<">>>>>>>>>>>tg>"<< tg<<"   <<<<n_df>"<<n_df <<"\n";
    if(m_wrt) m_wrt->Write(df);
    n_df ++;
    if(n_df >= ndf_print_next){
      auto tp_now = std::chrono::system_clock::now();
      auto dur = tp_now - tp_start;
      double dur_sec_c = dur.count() * decltype(dur)::period::num * 1.0/ decltype(dur)::period::den;
      double av_hz = n_df/dur_sec_c;
      dur = tp_now - tp_print_prev;
      dur_sec_c = dur.count() * decltype(dur)::period::num * 1.0/ decltype(dur)::period::den;
      double cr_hz = (n_df-ndf_print_prev) / dur_sec_c;
      //std::cout<<"JadeManager:: average data rate (Writing)<"<< av_hz<<"> current data rate<"<<cr_hz<<">"<<std::endl;
      ndf_print_next += 10000;
      ndf_print_prev = n_df;
      tp_print_prev = tp_now;
    }
    
  }
  return n_df;
}

void JadeManager::StartThread(){
  if(!m_rd || !m_wrt || !m_ctrl) {
    std::cerr<<"JadeManager: m_rd, m_wrt, or m_ctrl is not set"<<std::endl;
    throw;
  }
  m_is_running = true;
  m_fut_async_rd = std::async(std::launch::async,
			      &JadeManager::AsyncReading, this);

  m_fut_async_wrt = std::async(std::launch::async,
			       &JadeManager::AsyncWriting, this);
}

void JadeManager::StopThread(){
  m_is_running = false;
  if(m_fut_async_rd.valid())
    m_fut_async_rd.get();
  
  if(m_fut_async_wrt.valid())
    m_fut_async_wrt.get();
  
  m_qu_ev_to_wrt=decltype(m_qu_ev_to_wrt)();
}

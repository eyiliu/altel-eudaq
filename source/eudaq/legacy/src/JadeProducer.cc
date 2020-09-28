#define WIN32_LEAN_AND_MEAN

#include "JadeCore.hh"

#include "eudaq/Producer.hh"
#include "EudaqWriter.hh"

#include <list>
#include <iostream>
#include <chrono>
#include <thread>
#include "base64.hh"

namespace{
  auto _loading_print_
    = JadeUtils::FormatPrint(std::cout, "<INFO> %s  this eudaq module is loaded from %s\n\n",
                             JadeUtils::GetNowStr().c_str(), JadeUtils::GetBinaryPath().c_str())
    ;
}


class JadeProducer : public eudaq::Producer {
public:
  using eudaq::Producer::Producer;
  ~JadeProducer() override {};
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("JadeProducer");
private:
  JadeManagerSP m_jade_man;
};

namespace{
  auto _reg_ = eudaq::Factory<eudaq::Producer>::
    Register<JadeProducer, const std::string&, const std::string&>(JadeProducer::m_id_factory);
}

void JadeProducer::DoInitialise(){
  auto ini = GetInitConfiguration();

  std::string json_str;
  std::string json_base64  = ini->Get("JSON_BASE64", "");
  std::string json_path  = ini->Get("JSON_PATH", "");
  std::string ip_addr= ini->Get("IP_ADDR", ""); 
  std::string writer_name = ini->Get("WRITER_NAME", "EudaqWriter_v3");
  if(!json_base64.empty()){
    json_str = base64_decode(json_base64);
  }
  else if(!json_path.empty()){
    json_str = JadeUtils::LoadFileToString(json_path);
  }
  else if(!ip_addr.empty()){
    json_str +="{\"JadeManager\":{\"type\":\"JadeManager\",\"parameter\":{\"version\":3,\"JadeRegCtrl\":{\"type\":\"AltelRegCtrl\",\"parameter\":{\"IP_ADDRESS\": \"";
    json_str +=ip_addr;
    json_str +="\",\"IP_UDP_PORT\": 4660}},\"JadeReader\":{\"type\":\"AltelReader\",\"parameter\":{\"IS_DISK_FILE\":false,\"TERMINATE_AT_FILE_END\":true,\"FILE_PATH\":\"nothing\",\"IP_ADDRESS\": \"";
    json_str +=ip_addr;
    json_str +="\",\"IP_TCP_PORT\":24}},\"JadeWriter\":{\"type\":\"";
    json_str +=writer_name;
    json_str +="\",	\"parameter\":{\"nothing\":\"nothing\"}}}}}";
  }
  else{
    std::cerr<<"JadeProducer: no ini section for "<<GetFullName()<<std::endl;
    std::cerr<<"terminating..."<<std::endl;
    std::terminate();
  }
  
  JadeOption opt_conf(json_str);
  JadeOption opt_man = opt_conf.GetSubOption("JadeManager");
  std::cout<<opt_man.DumpString()<<std::endl;
  std::string man_type = opt_man.GetStringValue("type");
  JadeOption opt_man_para = opt_man.GetSubOption("parameter");
  m_jade_man = JadeManager::Make(man_type, opt_man_para);

  m_jade_man->Init();
  auto writer = m_jade_man->GetWriter();
  auto eudaq_writer = std::dynamic_pointer_cast<EudaqWriter>(writer);
  if(!eudaq_writer){
    std::cerr<<"JadeProducer: there is no instance of EudaqWriter in "<<man_type<<std::endl;
  }
  else{
    eudaq_writer->SetProducerCallback(this);
  }
}

void JadeProducer::DoConfigure(){
  //do nothing here
}

void JadeProducer::DoStartRun(){
  m_jade_man->StartDataTaking();
}

void JadeProducer::DoStopRun(){
  m_jade_man->StopDataTaking();
}

void JadeProducer::DoReset(){
  m_jade_man.reset();
}

void JadeProducer::DoTerminate(){
  m_jade_man.reset();
  std::terminate();
}

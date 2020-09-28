#define WIN32_LEAN_AND_MEAN

#include "JadeCore.hh"
#include "EudaqWriter.hh"

#include "eudaq/Producer.hh"

#include <list>
#include <iostream>
#include <chrono>
#include <thread>

class EudaqWriter_v4: public EudaqWriter {
public:
  using EudaqWriter::EudaqWriter;
  ~EudaqWriter_v4()override {};
  void Write(JadeDataFrameSP df) override;
  void Open() override;
  void Close() override;

  void SetProducerCallback(eudaq::Producer *producer) override;
private:
  eudaq::Producer *m_producer{nullptr};
  uint32_t m_tg_expected{0};
  uint32_t m_tg_last_send{0};
  uint32_t m_flag_wait_first_event{true};
  bool m_flag_print{true};
  
};

//+++++++++++++++++++++++++++++++++++++++++
//TestWrite.cc
namespace{
  auto _test_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(EudaqWriter_v4)));
  auto _test_ = JadeFactory<JadeWriter>::Register<EudaqWriter_v4, const JadeOption&>(typeid(EudaqWriter_v4));
}

void EudaqWriter_v4::SetProducerCallback(eudaq::Producer *producer){
  m_producer = producer;
}


void EudaqWriter_v4::Open(){
  m_tg_last_send = 0;
  m_tg_expected = 0;
  m_flag_wait_first_event = true;
  m_flag_print = false;
}

void EudaqWriter_v4::Close(){
  
}

void EudaqWriter_v4::Write(JadeDataFrameSP df){
  if(!m_producer){
    std::cerr<<"EudaqWriter_v4: ERROR, eudaq producer is not avalible";
    throw;
  }
  
  df->Decode(1); //level 1, header-only
  uint16_t tg_l15 = 0x7fff & df->GetCounter();
  if(m_flag_print)
    std::cout<<"id "<<tg_l15<<"\n";
  if(m_flag_wait_first_event){
    m_flag_wait_first_event = false;
    m_tg_expected = tg_l15;
  }

  if(tg_l15 != (m_tg_expected & 0x7fff)){
    // std::cout<<(m_tg_expected & 0x7fff)<< " " << tg_l15<<"\n";
    uint32_t tg_guess_0 = (m_tg_expected & 0xffff8000) + tg_l15;
    uint32_t tg_guess_1 = (m_tg_expected & 0xffff8000) + 0x8000 + tg_l15;
    if(tg_guess_0 > m_tg_expected && tg_guess_0 - m_tg_expected < 200){
      std::cout<< "missing trigger, expecting : provided "<< (m_tg_expected & 0x7fff) << " : "<< tg_l15<<"\n";
      m_flag_print = false;
      m_tg_expected =tg_guess_0;
    }
    else if (tg_guess_1 > m_tg_expected && tg_guess_1 - m_tg_expected < 200){
      std::cout<< "missing trigger, expecting : provided "<< (m_tg_expected & 0x7fff) << " : "<< tg_l15<<"\n";
      m_flag_print = false;
      m_tg_expected =tg_guess_1;
    }
    else{
      std::cout<< "broken trigger ID, expecting : provided "<< (m_tg_expected & 0x7fff) << " : "<< tg_l15<<"\n";
      m_flag_print = false;
      m_tg_expected ++;
      return;
    }
  }

  auto evup_to_send = eudaq::Event::MakeUnique("JadeRaw");
  evup_to_send->SetTriggerN(m_tg_expected - 1);
  evup_to_send->AddBlock<char>( (uint32_t)0, df->Raw().data(), (size_t)(df->Raw().size()) );
  m_producer->SendEvent(std::move(evup_to_send));
  m_tg_last_send = m_tg_expected;
  m_tg_expected++;
}

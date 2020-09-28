#include "JadeWriter.hh"

//++++++++++++++++++++++++++++++++++
//AltelWriter.hh
class AltelWriter: public JadeWriter{
 public:
  AltelWriter(const JadeOption &opt);
  ~AltelWriter() override {}; 

  void Open() override;
  void Close() override;
  void Write(JadeDataFrameSP df) override;
 private:
  FILE* m_fd;
  std::string m_path;
  JadeOption m_opt;
  bool m_disable_file_write;
  bool m_enable_decode;
  int m_n_ev;
};

//+++++++++++++++++++++++++++++++++++++++++
//AltelWriter.cc
namespace{
  auto _test_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(AltelWriter)));
  auto _test_ = JadeFactory<JadeWriter>::Register<AltelWriter, const JadeOption&>(typeid(AltelWriter));
}

AltelWriter::AltelWriter(const JadeOption &opt)
  :m_opt(opt), m_fd(0), m_disable_file_write(false), m_enable_decode(false), JadeWriter(opt){
  m_disable_file_write = m_opt.GetBoolValue("DISABLE_FILE_WRITE");
  m_enable_decode = m_opt.GetBoolValue("ENABLE_DECODE");
}

void AltelWriter::Open(){
  if(m_path.empty())
    m_path = m_opt.GetStringValue("PATH"); 

  if(m_disable_file_write)
    return;
  std::string time_str=JadeUtils::GetNowStr("%y%m%d%H%M%S");
  std::string data_path = m_path+"_"+time_str +".df";
  m_fd = std::fopen(data_path.c_str(), "wb" );
  if(m_fd == NULL){
    std::cerr<<"JadeWrite: Failed to open/create file: "<<data_path<<"\n";
    throw;
  }
  m_n_ev= 0;
}

void AltelWriter::Close(){
  if(m_disable_file_write)
    return;
  if(m_fd){
    std::fclose(m_fd);
    m_fd = 0;
  }
  std::cout<< "AltelWriter::Close m_n_ev "<< m_n_ev<<std::endl;
}

void AltelWriter::Write(JadeDataFrameSP df){
  if(m_disable_file_write)
    return;
  if(!df){
    std::cerr<<"JadeWrite: File is not opened/created before writing\n";
    throw;
  }
  //TODO
  if(m_enable_decode)
    df->Decode(3);
  
  m_n_ev++;
  std::string &rawstring = df->Raw();
  if(rawstring.size()){
    std::fwrite(&(rawstring.at(0)), 1, rawstring.size(), m_fd);
  }
}

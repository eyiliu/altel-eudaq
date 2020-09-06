#define WIN32_LEAN_AND_MEAN
#include "JadeDataFrame.hh"

#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

#define PLANE_NUMBER_OFFSET 50

class JadeRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("JadeRaw");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<JadeRawEvent2StdEventConverter>(JadeRawEvent2StdEventConverter::m_id_factory);
}

bool JadeRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  if(nblocks !=1 || block_n_list.front()!=0 )
    EUDAQ_THROW("Unknown data");

  auto rawblock = ev->GetBlock(0);
  char* block_raw = reinterpret_cast<char*>(rawblock.data());
  JadeDataFrame df(std::string(block_raw, rawblock.size()));
  df.Decode(3);
  
  size_t x_n_pixel = df.GetMatrixSizeX();
  size_t y_n_pixel = df.GetMatrixSizeY();
  size_t z_n_pixel = df.GetMatrixDepth();
  size_t n_pixel = x_n_pixel*y_n_pixel;
  uint64_t ext = df.GetExtension();
  
  const std::vector<uint16_t> &data_x = df.Data_X();
  const std::vector<uint16_t> &data_y = df.Data_Y();
  const std::vector<uint16_t> &data_z = df.Data_D();

  size_t n_hit = data_x.size();
  // std::cout<<"x_n_pixel:y_n_pixel:z_n_pixel:n_pixel:n_hit "<<x_n_pixel<<":"<<y_n_pixel<<":"<<z_n_pixel<<":"<<n_pixel<<":"<<n_hit<<std::endl;

  if(n_hit!=data_y.size() || n_hit!=data_z.size()){
    std::cerr<<"converter: wrong data\n";
    throw;
  }
  
  uint16_t bn = 0;//TODO, multiple-planes/producer
  std::vector<eudaq::StandardPlane*> v_planes;
  for(size_t i=0; i<z_n_pixel; i++){
    eudaq::StandardPlane* p = &(d2->AddPlane(eudaq::StandardPlane(PLANE_NUMBER_OFFSET+bn+i+ext, "alpide", "alpide")));
    p->SetSizeZS(x_n_pixel, y_n_pixel, 0); //TODO: check this function for its real meaning
    //p->SetSizeZS(y_n_pixel, x_n_pixel, 0); //TODO: check this function for its real meaning
    v_planes.push_back(p);
  }
  auto it_x = data_x.begin();
  auto it_y = data_y.begin();
  auto it_z = data_z.begin();
  while(it_x!=data_x.end()){
    v_planes[*it_z]->PushPixel(*it_x ,*it_y,  1);
    //v_planes[*it_z]->PushPixel(*it_y, 1023 - *it_x , 1);
    // std::cout<<"n:x:y "<<d1->GetEventN()<<":"<<*it_x<<":"<<*it_y<<std::endl;
    it_x ++;
    it_y ++;
    it_z ++;
  }

  return true;
}

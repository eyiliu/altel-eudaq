#define WIN32_LEAN_AND_MEAN
#include "JadeDataFrame.hh"
#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "UTIL/CellIDEncoder.h"

#define PLANE_ID_OFFSET_LICO 0

//         X     Y
//pitch  29.24 x 26.88
//matrix 1024  x 512

class JadeRawEvent2LCEventConverter: public eudaq::LCEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("JadeRaw");
};
  
namespace{
  auto dummy0 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<JadeRawEvent2LCEventConverter>(JadeRawEvent2LCEventConverter::m_id_factory);
}

bool JadeRawEvent2LCEventConverter::Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2,
						 eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  if(nblocks !=1 || block_n_list.front()!=0 )
    EUDAQ_THROW("Unknown data");

  auto rawblock = ev->GetBlock(0);
  char* block_raw = reinterpret_cast<char*>(rawblock.data());
  JadeDataFrame df(std::string(block_raw, ev->GetBlock(0).size()));
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
  if(n_hit!=data_y.size() || n_hit!=data_z.size()){
    std::cerr<<"converter: wrong data\n";
    throw;
  }


  lcio::LCCollectionVec *zsDataCollection = nullptr;
  auto p_col_names = d2->getCollectionNames();
  if(std::find(p_col_names->begin(), p_col_names->end(), "zsdata_alpide") != p_col_names->end()){
    zsDataCollection = dynamic_cast<lcio::LCCollectionVec*>(d2->getCollection("zsdata_alpide"));
    if(!zsDataCollection)
      EUDAQ_THROW("dynamic_cast error: from lcio::LCCollection to lcio::LCCollectionVec");
  }
  else{
    zsDataCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERDATA);
    d2->addCollection(zsDataCollection, "zsdata_alpide");
  }
  
  lcio::CellIDEncoder<lcio::TrackerDataImpl> zsDataEncoder("sensorID:10,sparsePixelType:10", zsDataCollection);
  uint16_t bn = 0;//TODO, multiple-planes/producer
  std::vector<lcio::TrackerDataImpl*> v_zsFrames;
  for(size_t i=0; i<z_n_pixel; i++){
    lcio::TrackerDataImpl* zsFrame = new lcio::TrackerDataImpl;
    zsDataEncoder["sensorID"] = PLANE_ID_OFFSET_LICO + bn + i + ext;
    zsDataEncoder["sparsePixelType"] = 2;
    zsDataEncoder.setCellID(zsFrame);
    v_zsFrames.push_back(zsFrame);
  }

  auto it_x = data_x.begin();
  auto it_y = data_y.begin();
  auto it_z = data_z.begin();
  while(it_x!=data_x.end()){
    auto zsFrame = v_zsFrames[*it_z];
    zsFrame->chargeValues().push_back(*it_x);// swap
    zsFrame->chargeValues().push_back(*it_y);//y
    zsFrame->chargeValues().push_back(1);//signal
    zsFrame->chargeValues().push_back(0);//time
    it_x ++;
    it_y ++;
    it_z ++;
  }

  for(auto &zsFrame: v_zsFrames){
    zsDataCollection->push_back(zsFrame);
  }

  return true; 
}

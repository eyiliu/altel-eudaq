#define WIN32_LEAN_AND_MEAN
#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "UTIL/CellIDEncoder.h"

#define PLANE_ID_OFFSET_LICO 0

//         X     Y
//pitch  29.24 x 26.88
//matrix 1024  x 512

class  AltelRawEvent2LCEventConverter: public eudaq::LCEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("AltelRaw");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::LCEventConverter>::
    Register< AltelRawEvent2LCEventConverter>( AltelRawEvent2LCEventConverter::m_id_factory);
}

bool AltelRawEvent2LCEventConverter::Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2,
                                                eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  if(!nblocks)
    EUDAQ_THROW("Unknown data");

  lcio::LCCollectionVec *zsDataCollection = nullptr;
  auto p_col_names = d2->getCollectionNames();
  if(std::find(p_col_names->begin(), p_col_names->end(), "zsdata_altel") != p_col_names->end()){
    zsDataCollection = dynamic_cast<lcio::LCCollectionVec*>(d2->getCollection("zsdata_altel"));
    if(!zsDataCollection)
      EUDAQ_THROW("dynamic_cast error: from lcio::LCCollection to lcio::LCCollectionVec");
  }
  else{
    zsDataCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERDATA);
    d2->addCollection(zsDataCollection, "zsdata_altel");
  }

  lcio::CellIDEncoder<lcio::TrackerDataImpl> zsDataEncoder("sensorID:10,sparsePixelType:10", zsDataCollection);
  std::vector<lcio::TrackerDataImpl*> v_zsFrames;

  for(const auto& blockNum: block_n_list){
    auto rawblock = ev->GetBlock(blockNum);
    uint32_t* p_block = reinterpret_cast<uint32_t*>(rawblock.data());
    uint32_t layerID = *p_block;
    p_block++;
    lcio::TrackerDataImpl* zsFrame = new lcio::TrackerDataImpl;
    v_zsFrames.push_back(zsFrame);
    zsDataEncoder["sensorID"] = PLANE_ID_OFFSET_LICO + layerID;
    zsDataEncoder["sparsePixelType"] = 2;
    zsDataEncoder.setCellID(zsFrame);

    uint32_t cluster_size =  *p_block;
    p_block++;
    for(size_t i = 0; i < cluster_size; i++){
      float cluster_x_mm = *(reinterpret_cast<float*>(p_block));
      p_block++;
      float cluster_y_mm = *(reinterpret_cast<float*>(p_block));
      p_block++;
      uint32_t pixel_size =  *p_block;
      p_block++;
      for(size_t j = 0; j < pixel_size; j++){
        uint32_t pixelXY =  *p_block;
        p_block++;
        uint16_t pixelX = static_cast<uint16_t>(pixelXY);
        uint16_t pixelY = static_cast<uint16_t>(pixelXY>>16);
        zsFrame->chargeValues().push_back(pixelX);// swap
        zsFrame->chargeValues().push_back(pixelY);//y
        zsFrame->chargeValues().push_back(1);//signal
        zsFrame->chargeValues().push_back(0);//time
      }
    }
  }

  for(auto &zsFrame: v_zsFrames){
    zsDataCollection->push_back(zsFrame);
  }

  return true;
}

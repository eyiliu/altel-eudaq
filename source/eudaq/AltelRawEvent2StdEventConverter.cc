#define WIN32_LEAN_AND_MEAN
#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

#define PLANE_NUMBER_OFFSET 50

class AltelRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("AltelRaw");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<AltelRawEvent2StdEventConverter>(AltelRawEvent2StdEventConverter::m_id_factory);
}

bool AltelRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  if(!nblocks)
    EUDAQ_THROW("Unknown data");

  for(const auto& blockNum: block_n_list){
    auto rawblock = ev->GetBlock(blockNum);
    uint32_t* p_block = reinterpret_cast<uint32_t*>(rawblock.data());
    uint32_t layerID = *p_block;
    p_block++;
    eudaq::StandardPlane* layer = &(d2->AddPlane(eudaq::StandardPlane(PLANE_NUMBER_OFFSET+layerID, "altel", "altel")));
    uint32_t cluster_size =  *p_block;
    p_block++;
    layer->SetSizeZS(1024, 512, 0); //TODO: check this function for its real meaning
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
        layer->PushPixel(pixelX , pixelY,  1);

      }
    }
  }
  return true;
}

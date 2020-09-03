#ifndef JADEPIX_JADEDATAFRAME___
#define JADEPIX_JADEDATAFRAME___

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

class JadeDataFrame;
using JadeDataFrameSP = std::shared_ptr<JadeDataFrame>;

class JadeDataFrame {
  public:
  JadeDataFrame(std::string& data);
  JadeDataFrame(std::string&& data);
  // JadeDataFrame(size_t nraw);
  JadeDataFrame() = delete;
  // JadeDataFrame();
  virtual ~JadeDataFrame();
  virtual void Decode(uint32_t level);

  //const version
  const std::string& Raw() const;
  const std::string& Meta() const;

  //none const version
  std::string& Raw();
  std::string& Meta();

  uint32_t GetMatrixDepth() const;
  uint32_t GetMatrixSizeX() const; //x row, y column
  uint32_t GetMatrixSizeY() const;
  uint64_t GetCounter() const;
  uint64_t GetExtension() const;

  const std::string& Data_Flat() const {return m_data_flat; }
  const std::vector<uint16_t>& Data_X() const {return m_data_x; }
  const std::vector<uint16_t>& Data_Y() const {return m_data_y; }
  const std::vector<uint16_t>& Data_D() const {return m_data_d; }
  const std::vector<uint32_t>& Data_T() const {return m_data_t; }
  const std::vector<uint32_t>& Data_V() const {return m_data_v; }
  
  void Print(std::ostream& os, size_t ws = 0) const;

 private:
  std::string m_data_raw;
  std::string m_meta;
  
  uint16_t m_level_decode;  

  uint64_t m_counter;
  uint64_t m_extension;
  
  uint16_t m_n_x;
  uint16_t m_n_y;
  uint16_t m_n_d; //Z
  std::string m_data_flat;

  std::vector<uint16_t> m_data_x;
  std::vector<uint16_t> m_data_y;
  std::vector<uint16_t> m_data_d;
  std::vector<uint32_t> m_data_t;
  std::vector<uint32_t> m_data_v;
};

#endif

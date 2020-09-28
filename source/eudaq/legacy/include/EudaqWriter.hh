#include "JadeCore.hh"
#include "eudaq/Producer.hh"

class EudaqWriter: public JadeWriter {
public:
  using JadeWriter::JadeWriter;
  virtual void SetProducerCallback(eudaq::Producer *producer) = 0;
};

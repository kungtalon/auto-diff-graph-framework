#ifndef ADGC_AUTODIFF_LAYER_DENSE_H_
#define ADGC_AUTODIFF_LAYER_DENSE_H_

#include "layers.h"

namespace graph_component {

namespace layer {

class Dense : public Layer {
public:
  Dense(){};
  Dense(const size_t &input_channel, const size_t &output_channel,
        const std::string &activation = "relu", bool use_bias = true,
        Graph *graph = nullptr);
  ~Dense(){};

  Parameter &get_weight();
  Parameter &get_bias();
  Node &operator()(const Node &input);
  void assign_weight(const DTensor &value);
  void assign_bias(const DTensor &value);

private:
  size_t input_channel_, output_channel_;
  std::string activation_;
};

} // namespace layer

} // namespace graph_component

#endif
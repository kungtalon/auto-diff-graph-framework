#include "autodiff/ops.h"

namespace graph_component {
namespace ops {

Add::Add(const std::vector<Node *> &parents)
    : Node(NodeType::ADG_ADD_TYPE, parents) {
  // accept two matrices with same size
  if (parents_.size() != 2) {
    throw adg_exception::OpsParentsNumException("Add ==> Add");
  }

  if (parents_[0]->get_value_shape() != parents_[1]->get_value_shape()) {
    throw adg_exception::MismatchNodeValueShapeError("Add ==> Add");
  }
};

void Add::do_forward() {
  if (parents_.empty()) {
    throw adg_exception::OpsParentsUnsetException("Add ==> do_forward");
  }

  value_ = parents_[0]->get_value().add(parents_[1]->get_value());
}

DTensor Add::do_backward(Node *parent_ptr) {
  if (parents_.empty()) {
    throw adg_exception::OpsParentsUnsetException("Add ==> do_backward");
  }

  return tensor::Eye(get_value_size());
}

VecDot::VecDot(const std::vector<Node *> &parents)
    : Node(NodeType::ADG_VECDOT_TYPE, parents) {
  // accept two [n × 1] matrix parents
  if (parents_.size() != 2) {
    throw adg_exception::OpsParentsNumException("VecDot ==> VecDot");
  }

  if (parents_[0]->get_value().get_dim() != 2 ||
      parents_[0]->get_value_shape()[1] != 1 ||
      parents_[1]->get_value().get_dim() != 2 ||
      parents_[1]->get_value_shape()[1] != 2) {
    throw adg_exception::MismatchNodeValueShapeError("VecDot ==> VecDot");
  }
};

void VecDot::do_forward() {
  if (parents_.empty()) {
    throw adg_exception::OpsParentsUnsetException("VecDot ==> do_forward");
  }

  DTensor l_mat = parents_[0]->get_value().t();
  value_ = DTensor::dot(l_mat, parents_[1]->get_value());
}

DTensor VecDot::do_backward(Node *parent_ptr) {
  if (parents_.empty()) {
    throw adg_exception::OpsParentsUnsetException("VecDot ==> do_backward");
  }

  if (parent_ptr == parents_[0]) {
    return parents_[1]->get_value().t();
  } else {
    return parents_[0]->get_value().t();
  }
}

MatMul::MatMul(const std::vector<Node *> &parents)
    : Node(NodeType::ADG_MATMUL_TYPE, parents) {
  if (parents_.size() != 2) {
    throw adg_exception::OpsParentsNumException("Matmul ==> MatMul");
  }

  tensor::TensorShape pa_shape_a = parents_[0]->get_value_shape();
  tensor::TensorShape pa_shape_b = parents_[1]->get_value_shape();
  size_t pa_dim_a = pa_shape_a.size();
  size_t pa_dim_b = pa_shape_b.size();
  if (pa_dim_a < 2 || pa_dim_b < 2) {
    throw adg_exception::IncompatibleNodeValueShapeError("Matmul ==> MatMul");
  } else if (pa_shape_a[pa_dim_a - 1] != pa_shape_b[pa_dim_b - 2] ||
             pa_shape_a[pa_dim_a - 2] != pa_shape_b[pa_dim_b - 1]) {
    throw adg_exception::IncompatibleNodeValueShapeError("Matmul ==> MatMul");
  }
};

void MatMul::do_forward() {
  if (parents_.empty()) {
    throw adg_exception::OpsParentsUnsetException("MatMul ==> do_forward");
  }

  value_ = DTensor::dot(parents_[0]->get_value(), parents_[1]->get_value());
}

DTensor MatMul::do_backward(Node *parent_ptr) {
  if (parents_.empty()) {
    throw adg_exception::OpsParentsUnsetException("MatMul ==> do_backward");
  }

  DTensor zeros = tensor::Zeros({get_value_size(), get_value_size()});
  if (parent_ptr == parents_[0]) {
    zeros.fill_diag(parents_[1]->get_value().t().to_vector());
  } else {
    zeros.fill_diag(parents_[0]->get_value().to_vector());
    tensor::TensorShape row_shape(get_value_shape());
    tensor::TensorShape col_shape(parent_ptr->get_value_shape());

    std::reverse(row_shape.begin(), row_shape.end());
    std::reverse(col_shape.begin(), col_shape.end());

    std::vector<int32_t> rows_inds_int32 =
        tensor::Ranges<int32_t>(row_shape, 0).t().to_vector();
    tensor::TensorSlice rows_inds(rows_inds_int32.begin(),
                                  rows_inds_int32.end());

    std::vector<int32_t> cols_inds_int32 =
        tensor::Ranges<int32_t>(row_shape, 0).t().to_vector();
    tensor::TensorSlice cols_inds(cols_inds_int32.begin(),
                                  cols_inds_int32.end());

    return zeros.take(0, rows_inds).take(1, cols_inds);
  }
}

} // namespace ops
} // namespace graph_component
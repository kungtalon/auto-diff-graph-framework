#include <iostream>

#include "autodiff/functional.h"
#include "autodiff/ops.h"
#include "autodiff/variable.h"
#include "gtest/gtest.h"

typedef graph_component::Graph g;
typedef graph_component::Variable v;

TEST(GraphTest, ConstructorDestructorTest) {
  g *graph_ptr = new g();

  // use nested code block to make sure graph_ptr outlives node_ptr
  {
    v *node_ptr0 = new v({2});
    v *node_ptr1 = new v({2}, {}, "node1", true, true, graph_ptr);
    v *node_ptr2 = new v({2}, {node_ptr1}, "node2", true, true, graph_ptr);
    v *node_ptr3 =
        new v({2}, {node_ptr1, node_ptr2}, "node3", true, true, graph_ptr);

    EXPECT_THROW(new v({2}, {node_ptr1, node_ptr2}),
                 adg_exception::MismatchRegisterdGraphError);
    EXPECT_THROW(new v({2}, {}, "node3", true, true, graph_ptr),
                 adg_exception::DuplicateNodeNameError);
  }
  graph_ptr->remove_all();
  delete graph_ptr;
  SUCCEED();
}

#ifdef ADGC_ENABLE_GRAPHVIZ_
TEST(GraphTest, GraphVizTest) {
  g *graph_ptr = new g();

  // use nested code block to make sure graph_ptr outlives node_ptr
  {
    v *pv1 = new v({2, 2}, graph_ptr);
    v *pv2 = new v({2, 1}, graph_ptr);
    v *pv3 = new v({1}, graph_ptr);

    v *pv4 = new v({2, 1}, graph_ptr);

    auto matmul = new graph_component::ops::MatMul(pv1, pv2, graph_ptr);
    auto add = new graph_component::ops::Add(matmul, pv3, graph_ptr);
    auto relu = new graph_component::functional::ReLU(add, graph_ptr);

    auto matsum = new graph_component::ops::MatSum({relu, pv4}, graph_ptr);
    auto sigmoid = new graph_component::functional::Sigmoid(matsum, graph_ptr);
    auto target =
        new graph_component::functional::ReduceSum(sigmoid, graph_ptr);

    graph_ptr->visualize("../graphviz/test.svg");
  }
  graph_ptr->remove_all();
  delete graph_ptr;
  SUCCEED();
}
#endif

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
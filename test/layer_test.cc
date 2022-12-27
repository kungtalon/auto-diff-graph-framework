#include "autodiff/layer/layer.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;
using namespace auto_diff;

MATCHER_P(FloatNearPointwise, tol, "Out of range") {
  return (std::get<0>(arg) > std::get<1>(arg) - tol && std::get<0>(arg) < std::get<1>(arg) + tol);
}

TEST(LayerTest, DenseLayerTest) {
  Graph *graph = Graph::get_instanceof_global_graph();

  try {
    Variable v1 = Variable({2, 2});

    DTensor value_v1 = tensor::Tensor<double>({2, 2}, {1, 2, 3, 4});
    DTensor weight = tensor::Tensor<double>({2, 1}, {-1, 2});
    DTensor bias = tensor::Tensor<double>({1}, 2);

    layer::Dense dense_layer(2, 1);
    dense_layer.assign_weight(weight);
    dense_layer.assign_bias(bias);

    // build graph
    auto target = functional::reduce_sum(dense_layer(v1));

    // forward
    v1.assign_value(value_v1);
    graph->zero_grad();
    target.forward();

    // test forward
    ASSERT_EQ(v1.get_value_shape(), tensor::TensorShape({2, 2}));
    ASSERT_EQ(dense_layer.get_weight().get_value_shape(), tensor::TensorShape({2, 1}));
    ASSERT_THAT(dense_layer.get_weight().get_value().to_vector(), ElementsAre(-1., 2.));
    ASSERT_FLOAT_EQ(target.get_value().get_value(), 12.);

    graph->backward(target);

    // test backward
    ASSERT_THAT(target.get_value().to_vector(), ElementsAre(12.));
    ASSERT_THAT(target.get_grad().to_vector(), ElementsAre(1.));
    ASSERT_EQ(v1.get_grad().get_shape(), tensor::TensorShape({2, 2}));
    ASSERT_THAT(v1.get_grad().to_vector(), ElementsAre(-1, 2, -1., 2.));
    ASSERT_THAT(dense_layer.get_weight().get_grad().to_vector(), ElementsAre(4., 6.));
    ASSERT_FLOAT_EQ(dense_layer.get_bias().get_grad().get_value(), 2.);

    // test again
    v1.assign_value(value_v1);
    graph->zero_grad();
    target.forward();
    graph->backward(target);

    ASSERT_THAT(target.get_value().to_vector(), ElementsAre(12.));
    ASSERT_THAT(target.get_grad().to_vector(), ElementsAre(1.));
    ASSERT_EQ(v1.get_grad().get_shape(), tensor::TensorShape({2, 2}));
    ASSERT_THAT(v1.get_grad().to_vector(), ElementsAre(-1, 2, -1., 2.));
    ASSERT_THAT(dense_layer.get_weight().get_grad().to_vector(), ElementsAre(4., 6.));
    ASSERT_FLOAT_EQ(dense_layer.get_bias().get_grad().get_value(), 2.);
  } catch (const std::exception &ex) {
    FAIL() << "Failed and got this: " << std::endl << ex.what();
  }
  graph->remove_all();
  Graph::delete_global_graph();
}

TEST(LayerTest, ConvLayerTest1) {
  Graph *graph = Graph::get_instanceof_global_graph();

  try {
    Variable v1 = Variable({2, 1, 7, 7});

    DTensor value_v1 = tensor::Tensor<double>({2, 1, 7, 7},
                                              {1., 1., 8., 9., 5., -3., 1., 7., 0., -4., 0., -6., 6., -3., 7., 9., 6.,
                                               8., 9., -9., 1., 5., -3., 0., -9., 8., 9., -8., -1., -8., -3., -4., -2.,
                                               -9., -9., -9., -8., -7., 1., -9., 6., 1., -8., 9., 1., 7., 1., -10., 3.,
                                               0., -1., -1., 7., 5., 9., -2., -8., 1., -7., 5., 5., 9., -1., 8., 4.,
                                               -3., -7., -9., -1., -4., 6., -8., 6., -10., 8., -4., -4., 2., -7., -1.,
                                               -5., 4., -5., -2., 2., 2., -7., 8., 8., 6., -3., -2., 8., 8., -8., -10.,
                                               9., -9.});
    DTensor weight = tensor::Tensor<double>({4, 1, 3, 3},
                                            {1., -3., 7., 5., -2., -9., -4., 8., -8., 8., -10., -8., -10., 8., 7., 5.,
                                             0., -3., -4., 1., -2., 6., -9., 1., -7., -1., -4., 8., 4., -10., -7., 6.,
                                             -8., 3., 5., -4.});
    DTensor bias = tensor::Tensor<double>({4}, {-5., 7., -7., -3.});

    layer::Conv2D conv_layer(1, 4, {3, 3}, {1, 1}, "VALID", "none");
    conv_layer.assign_weight(weight);
    conv_layer.assign_bias(bias);

    // build graph
    auto target = functional::reduce_sum(conv_layer(v1));

    // forward
    v1.assign_value(value_v1);
    graph->zero_grad();
    target.forward();

    // test forward
    ASSERT_FLOAT_EQ(target.get_value().get_value(), 1224.);

    graph->backward(target);

    // test backward
    std::vector<double> v1_grad_exp
      ({13., 5., -8., -8., -8., -21., -13., 7., 2., -20., -20., -20., -27., -22., 4., 11., -30., -30., -30., -34., -41.,
        4., 11., -30., -30., -30., -34., -41., 4., 11., -30., -30., -30., -34., -41., -9., 6., -22., -22., -22., -13.,
        -28., -3., 9., -10., -10., -10., -7., -19., 13., 5., -8., -8., -8., -21., -13., 7., 2., -20., -20., -20., -27.,
        -22., 4., 11., -30., -30., -30., -34., -41., 4., 11., -30., -30., -30., -34., -41., 4., 11., -30., -30., -30.,
        -34., -41., -9., 6., -22., -22., -22., -13., -28., -3., 9., -10., -10., -10., -7., -19.});
    std::vector<double> weight_grad_exp
      ({37., 12., -7., -16., -27., -41., -3., -21., -53., 37., 12., -7., -16., -27., -41., -3., -21., -53., 37., 12.,
        -7., -16., -27., -41., -3., -21., -53., 37., 12., -7., -16., -27., -41., -3., -21., -53.});
    std::vector<double> bias_grad_exp({50., 50., 50., 50.});
    ASSERT_THAT(v1.get_grad().to_vector(), ElementsAreArray(v1_grad_exp));
    ASSERT_THAT(conv_layer.get_weight().get_grad().to_vector(), ElementsAreArray(weight_grad_exp));
    ASSERT_THAT(conv_layer.get_bias().get_grad().to_vector(), ElementsAreArray(bias_grad_exp));

  } catch (const std::exception &ex) {
    FAIL() << "Failed and got this: " << std::endl << ex.what();
  }
  graph->remove_all();
  Graph::delete_global_graph();
}

TEST(LayerTest, ConvLayerTest2) {
  Graph *graph = Graph::get_instanceof_global_graph();

  try {
    Variable v1 = Variable({2, 3, 9, 9});

    DTensor value_v1 = tensor::Tensor<double>({2, 3, 9, 9},
                                              {-6., 3., -8., 9., -1., 8., -1., 3., 8., 4., -8., -7., 2., 0., -6., -10.,
                                               8., -9., 6., 0., -9., -8., 7., -9., 5., -5., 5., -6., 6., 7., -3., 1.,
                                               -9., -5., 4., -6., 0., -2., 7., 1., -9., 7., -9., 7., -4., 4., 0., -5.,
                                               8., -10., -7., 4., -10., 0., -6., 3., 9., 9., -1., -3., 6., -5., 0., 8.,
                                               -10., -8., 7., -9., -6., -3., -6., -9., 2., -5., 1., -7., 4., -1., 9.,
                                               0., -7., 5., -5., 6., 3., -5., 3., 9., 8., -5., 8., -2., -2., 8., 9.,
                                               -5., -8., -8., 8., -6., 9., 3., -2., -4., -9., 4., -2., 5., -6., 7., 1.,
                                               6., 6., 0., 3., -7., 2., 7., -8., 4., -8., -4., 8., -1., -6., 2., -3.,
                                               -10., -1., -8., 1., 6., 9., -9., 0., 3., 8., -2., -4., 4., -3., 6., 6.,
                                               -10., 3., 5., -4., 1., -6., -2., 8., -1., -9., -7., -7., -7., 8., -8.,
                                               4., 9., -3., -1., -7., 8., 4., 0., 1., -1., 4., 6., 7., 0., 8., -8., 0.,
                                               -9., 2., -5., -1., -7., 0., 4., 7., -2., -4., -3., 1., -2., 0., 4., -5.,
                                               -6., 5., 9., -1., 8., -4., 1., -2., 0., -2., 4., -5., -3., 9., -5., 3.,
                                               -4., -7., -4., -10., -5., -1., -9., -4., 4., -5., 3., 6., 7., 4., 7.,
                                               -9., 4., -9., -2., -5., -6., 8., -7., 5., 5., -5., 5., -5., -7., -3.,
                                               -4., 7., 7., 7., -7., -2., 1., 2., -2., 8., -8., 6., 2., -6., -5., 9.,
                                               -8., 3., -1., 8., 2., 5., -3., 7., 9., -2., 7., -2., -1., -1., -3., -10.,
                                               -5., 7., 6., -4., -1., -5., 2., 9., 4., -10., -8., 2., 3., -9., -2., 6.,
                                               -4., -8., -10., 0., -6., 9., 4., -2., -10., 3., -1., -6., -2., 8., -5.,
                                               1., 4., 5., -4., 0., -2., 3., 8., 5., -8., 0., -5., 0., -6., -7., -9.,
                                               -5., 6., -10., -8., -2., -10., -3., -5., -10., 0., -4., 7., 0., 1., 3.,
                                               2., -7., 6., -5., 0., 4., 2., 0., -1., 1., -8., -9., 2., 3., -10., -7.,
                                               -1., -2., -1., -5., 8., 8., 2., -3., -9., 3., 9., -1., 0., 8., 2., 9.,
                                               7., -1., 5., -3., -2., -7., -2., 7., -6., 9., -8., 3., -6., 6., 4., 5.,
                                               -8., -2., -4., -10., -9., 5., 4., 9., 1., -5., -3., -5., 4., -5., -6.,
                                               -8., 3., 9., 9., -6., 5., 7., 6., -6., 9., 4., 3., 3., 1., 4., 9., -5.,
                                               -9., -4., 1., 0., 8., -4., -4., 7., 7., 0., 4., -2., -3., 8., -9., -2.,
                                               -5., -2., -10., -8., 0., 4., 9., 9., -1., -1., -10., -2., -7., 0., 8.,
                                               -10., -2., 5., 2., 7., 5., 0., -8., -10., 7., -6., -8., -8., -6., 2.,
                                               -1., -2., -3., 0., -7., -2., 3., -3., 8., 8., 7., 8., -6., 5., 6., 9.,
                                               9., 3., -5., -6., -3., 5., -6., -3., 3., 2., 7., -6., 4.});
    DTensor weight = tensor::Tensor<double>({5, 3, 2, 2},
                                            {-10., -6., 8., 6., -6., -8., -6., -3., 2., 6., -2., 0., 5., 3., 8., -9.,
                                             -8., 8., -3., 7., 2., 1., 8., 5., 7., -4., 7., 9., -2., 7., 7., 0., -8.,
                                             6., -2., 7., -2., 6., -1., -5., 4., 9., 7., 1., 4., -8., -9., -4., -6., 0.,
                                             1., -1., 1., -9., 8., 2., 8., -1., 8., 8.});
    DTensor bias = tensor::Tensor<double>({5}, {0., 0., -2., -6., 1.});

    layer::Conv2D conv_layer(3, 5, {2, 2}, {2, 2}, {1, 1});
    conv_layer.assign_weight(weight);
    conv_layer.assign_bias(bias);

    // build graph
    auto target = functional::reduce_sum(conv_layer(v1));

    // forward
    v1.assign_value(value_v1);
    graph->zero_grad();
    target.forward();

    // test forward
    ASSERT_FLOAT_EQ(target.get_value().get_value(), 10783.);

    graph->backward(target);

    // test backward
    std::vector<double> v1_grad_exp
      ({-14., 9., -10., 23., 0., 23., 0., 16., 14., 2., -3., 9., -2., 6., -16., -6., 10., 5., 4., 8., -15., -1., -5.,
        9., 5., 14., -5., -6., -2., 6., -5., -3., -6., -1., 2., -7., 5., -1., -5., 16., -3., 23., 0., 23., 6., 9., -9.,
        -10., 2., -7., -2., 6., -10., -6., -15., 16., 14., 23., 6., -1., -5., 8., 6., 6., -10., -6., -1., 2., -4., -7.,
        -10., -6., -5., 8., 6., 7., 3., 24., 5., 8., 6., 8., 5., 9., 13., 7., 13., 7., 9., -1., 16., -3., 8., 4., 9.,
        -5., -17., -6., 24., 1., 12., 10., 7., 1., 2., -1., 11., 8., -17., 4., 9., -14., 0., -11., 7., -16., 7., -1.,
        7., 1., -9., 4., 13., 7., -2., 4., 8., -7., -10., -16., 7., 4., 9., -6., -8., 10., 9., -1., -2., 4., 7., 1.,
        -6., -3., 9., -6., -8., 3., 7., -15., -2., -6., -8., 1., -6., -3., 22., 3., 6., 6., -6., -3., 1., 16., 13., 3.,
        16., 3., 16., 4., 15., -2., 14., -8., 4., -8., 10., 5., -2., -1., 3., 7., 9., -9., -4., 6., 8., -3., 8., 5., 4.,
        -8., 4., 7., 8., 4., -4., 13., 8., -9., -4., 6., 5., 3., 16., 4., 12., -8., 2., 11., -4., 13., 4., -8., 2., 6.,
        9., 4., 15., 4., 12., -9., -4., -2., 0., -8., 2., 6., 4., -3., 4., 12., 2., 6., -4., -2., 0., -3., 11., 12.,
        20., -2., 0., 14., 17., -4., 15., -6., 14., 10., -1., -5., 2., -18., 0., -10., -6., -2., 6., -8., 6., 4., 8.,
        0., 8., 6., -1., -5., 0., -6., 5., 5., 2., -6., 0., 10., 5., 0., 0., -6., 6., 4., 1., -1., 14., -5., 0., 0., 9.,
        -18., 0., -3., -10., -11., -3., 6., -1., -15., 8., 0., 15., 15., 17., -4., 16., -1., 9., 7., -4., -3., 9., -11.,
        -3., 4., 5., -14., 7., 9., 8., -15., 17., -4., 15., -6., -1., -1., 6., 19., 10., 8., -2., 7., 1., 16., -1., -8.,
        -6., -8., 4., 9., 5., 0., 1., 9., 0., -6., -3., 7., 1., 15., 3., 15., 2., 16., 1., -9., -6., 24., 0., 0., 10.,
        14., 1., 8., 2., 11., 8., 0., 0., 8., -1., -8., -8., -1., -13., -9., -9., 6., 10., 9., 0., 1., -3., -1., 6.,
        12., 9., 17., -2., 7., -3., 8., -13., -9., -5., 15., 8., 7., 0., 12., 10., -1., 6., 19., 10., 15., 14., 13., 5.,
        16., -13., 3., -9., -4., -2., 14., -3., 2., 6., 4., -8., 12., -9., 3., -3., 4., -2., 0., -9., -4., -1., 4., -2.,
        -4., -2., 8., -1., -2., -1., 0., 0., 16., -11., 3., 8., 8., -3., 8., 0., 0., -8., 14., -3., -6., 12., 12., 6.,
        2., 6., 9., -3., 4., -4., 7., 14., 13., 14., 20., -7., -8., 6., 14., -8., 12., 6., 6., -2., 1., -2., 7., 7., 9.,
        14., 13., 5., 16.});
    std::vector<double> weight_grad_exp
      ({-64., -40., 43., -8., -18., -32., -19., -28., -18., 10., 6., 13., -34., -26., 44., -102., -27., 38., 29., 70.,
        -5., 5., 31., 68., 25., -32., 48., 23., -21., 44., 63., -3., -31., -9., 9., 67., -22., 38., -3., -36., 13., 46.,
        39., 24., 34., -44., -20., -20., -70., -30., 3., -52., -27., -33., 59., 33., 36., -24., 27., 79.});
    std::vector<double> bias_grad_exp({23., 23., 22., 28., 26.});
    ASSERT_THAT(v1.get_grad().to_vector(), ElementsAreArray(v1_grad_exp));
    ASSERT_THAT(conv_layer.get_weight().get_grad().to_vector(), ElementsAreArray(weight_grad_exp));
    ASSERT_THAT(conv_layer.get_bias().get_grad().to_vector(), ElementsAreArray(bias_grad_exp));

  } catch (const std::exception &ex) {
    FAIL() << "Failed and got this: " << std::endl << ex.what();
  }
  graph->remove_all();
  Graph::delete_global_graph();
}

TEST(LayerTest, BatchNormTest) {
  Graph *graph = Graph::get_instanceof_global_graph();

  try {
    Variable v1 = Variable({3, 5, 4, 4});

    DTensor value_v1 = tensor::Tensor<double>({3, 5, 4, 4},
                                              {3.6105729162621527, 8.444973792121944, 2.4677771009841116,
                                               9.107443133998114, 13.149648728093439, 11.478563719349161,
                                               1.5473878568119244, 5.9396393530626606, 4.454656201006301,
                                               6.304716163015122, 12.74514476645828, 4.586035165019076,
                                               3.4532171689782842, 3.6998627391560137, 7.1686325770342325,
                                               9.18819570551631, 11.187670211976876, 8.384475060886494,
                                               11.655756364647319, 3.401050417280669, 10.038869662012127,
                                               9.51939102043347, 6.657868980706289, 5.951276007814673,
                                               6.335983597982082, 3.2281070051118737, 11.883432279319168,
                                               0.024599413180531604, 13.015989378456103, 4.824473233772272,
                                               7.265047651899321, 5.6814972382967355, 9.199503212038243,
                                               10.343850967720883, 1.8225022553061374, 8.717625079912281,
                                               2.7668825552763883, 11.234052072526652, 8.905332712736458,
                                               9.432856949074997, 12.325389481886951, 2.2904306029954777,
                                               8.931297974708496, 6.636468254939838, 6.386608058508372,
                                               6.441385918753172, 5.419533748965481, 9.189541977121438,
                                               6.122624769150228, 4.577008406963588, 1.9739458122130964,
                                               7.2066662371313575, 7.654631780929316, 2.962783602992654,
                                               4.992075160926302, 4.72955444960199, 8.693546987766563,
                                               9.679941440771504, 4.193083330250911, 11.9413628713266,
                                               12.140756061233283, 8.731702191620478, 4.191289309116704,
                                               4.035257426355108, 7.615991370398321, 6.058615385633569,
                                               11.155197054285248, 9.019680719446693, 9.790846038430653,
                                               12.447797414586377, 11.568022892066463, 9.597398700178568,
                                               11.555792644232522, 9.546598092806677, 10.494120206480368,
                                               6.560268477385266, 6.512491167249461, 10.361465123696634,
                                               2.511112380829963, 1.575233237230028, 1.9886513837742092,
                                               7.416325998239647, 7.593076992481906, 10.153178999276959,
                                               6.448707726387436, 7.052193385018354, 11.298702808171102,
                                               9.511388520356219, 9.535293310572753, 2.173418173035017,
                                               3.4507444685756177, 11.327551339374399, 0.07429986502320074,
                                               3.1996440161220856, 10.47112285505781, 6.2191486062726025,
                                               7.074256502516001, 10.063131133885914, 4.223081606225247,
                                               2.6816661249156333, 8.298020007943347, 10.04746615688511,
                                               10.99244978088473, 5.137903559939297, 3.834041799173237,
                                               10.679954062285956, 11.099692100638332, 6.131684826069784,
                                               3.0041353267766446, 5.32555055947598, 5.970838662313607,
                                               4.059179716752028, 4.795427613165244, 11.326669994407908,
                                               3.665291200600693, 2.633711298148423, 8.238809877426167,
                                               7.660351319732988, 10.473065687214762, 3.105445898006999,
                                               8.268974672382294, 12.003901583377111, 9.445687234640994,
                                               9.240510099520062, 11.941608268638236, 12.267042131430525,
                                               2.146587839488887, 8.464221063598616, 6.390467767756228,
                                               11.771757449861763, 1.790779390773937, 10.019115158038208,
                                               7.728050226912638, 10.563571834106302, 2.5537382919451965,
                                               11.547360017094643, 0.8502735625671745, 7.520782893429767,
                                               7.582175161347459, 2.8962020254599, 3.5157741356376278,
                                               9.871508966506301, 6.860723726198087, 2.3668784896703734,
                                               12.316012047530785, 7.153853879454655, 7.174945056704397,
                                               5.598981125592817, 3.9324087181000964, 8.13323117129342,
                                               4.389174453837155, 10.751264060837128, 8.249383322314658,
                                               8.652525070428887, 9.830927043445836, 3.77396879495355,
                                               7.792741001585991, 10.8960281872787, 10.549679289787662,
                                               3.8744782751911124, 8.465475677919535, 5.682055049500562,
                                               8.643807096185602, 9.75061039497433, 3.934065893809549,
                                               5.597439442286552, 5.368368888422713, 11.269322703013177,
                                               6.333969753550122, 2.200417885402611, 8.510659094077798,
                                               6.413108324448137, 5.007368700185899, 3.4568395737245146,
                                               12.79541923196718, 6.273713276786199, 12.949739746419594,
                                               8.73951152310328, 10.89803694969815, 9.594799518587994,
                                               7.905062319779724, 7.706630052544344, 7.832099158169064,
                                               4.351743383425674, 9.59121044258203, 5.267318077058668,
                                               3.2500357518128062, 9.978664547788352, 11.289579459174814,
                                               5.733163267423306, 11.144368600779623, 4.318378285198859,
                                               11.525526613348203, 6.736080231527287, 12.410522897347732,
                                               7.27806791814602, 9.594152609471239, 4.229672592021111,
                                               9.184079351739161, 9.050830574227614, 10.799008024436649,
                                               9.01791123116468, 9.597106350864323, 4.955931186806082,
                                               11.018968895322573, 4.520896287515886, 7.527898120650219,
                                               9.185394729396707, 5.179570420040871, 3.858339185815203,
                                               7.9332606539137345, 9.316010858396861, 10.581789394780863,
                                               3.0944797140538514, 11.05748222645709, 10.147932595379606,
                                               3.1172217006719674, 6.013738496080077, 9.318877809869827,
                                               2.0719081974484, 12.8731975898821, 4.284595702056031, 8.68804354984819,
                                               3.722618158408295, 7.171532762628845, 3.66088817885543,
                                               5.827142850276347, 10.108193922806752, 5.507869684665984,
                                               10.390352951813655, 2.9377556274559637, 2.3955816012859734,
                                               4.088599121780496, 1.245889427893377, 11.899644433092439,
                                               6.178589986917198, 10.103946671466286, 7.366022567777222,
                                               4.316598372288227, 5.655805791655958});
    DTensor weight = tensor::Tensor<double>({5},
                                            {206.8220385558213, 211.93932071562458, 200.79338483652177,
                                             207.41006211877448, 194.20772368104411});
    DTensor bias = tensor::Tensor<double>({5},
                                          {3.5521382021746613, -0.9649412508315304, 5.2740295942467785,
                                           6.669372573108524, -2.3164180572411626});

    layer::BatchNorm2D bn_layer(5);
    bn_layer.assign_weight(weight);
    bn_layer.assign_bias(bias);

    // build graph
    functional::BatchNorm2D &bn = dynamic_cast<functional::BatchNorm2D &>(bn_layer(v1));
    auto target = functional::reduce_sum(bn);

    // forward
    v1.assign_value(value_v1);
    graph->zero_grad();
    target.forward();

    // test forward
    std::vector<double> exp_bn_out
      ({-195.21392380143593, 108.92306058813527, -267.1083491825067, 150.59966810631693, 404.898856617895,
        299.76923078236365, -325.01095518791817, -48.69002958559056, -142.111801865784,
        -25.722683082750294, 379.4511086498549, -133.84662010839304, -205.1133309222349,
        -189.59661192164936, 28.62716081133822, 155.67989298089967, 250.59974375595112,
        61.26642938028426, 282.2152032304641, -275.3238734205234, 173.00749492632195,
        137.92088566217626, -55.351941500722035, -103.07662075471092, -77.0927133968736,
        -287.0048116572417, 297.5928823448009, -503.37601569990176, 374.08801655473195,
        -179.1830965290281, -14.341899730896133, -121.29800950146014, 87.35037653664739,
        163.47460827408977, -403.3820320069561, 55.294908767341326, -340.5600231847114,
        222.69251197394635, 67.78158449072794, 102.87351962560061, 295.2903926805199,
        -372.2545309487022, 69.50884406690786, -83.14767884594556, -99.76886362186666,
        -96.12493413745145, -164.10052196732718, 86.6877358739359, -26.01679985232949,
        -122.90542075096745, -286.08119391721334, 41.93750329766761, 70.01870430623427,
        -224.09483547471768, -96.88651570041037, -113.34290830844273, 135.14421933292437,
        196.9774145240701, -146.97217633219705, 338.7370445574446, 351.23622071737316,
        137.5360192465085, -147.08463647289483, -156.865662545627, 7.194024749037982,
        -90.55793628345744, 229.33967196701565, 95.29952264563744, 143.70332683189423,
        310.4724371353651, 255.25154912626974, 131.56120003936238, 254.4838920406484,
        128.37259366503574, 187.84580093119837, -59.07063074101961, -62.06947354090916,
        179.519427203921, -313.2243822031333, -371.96679410062063, -297.25062627486597,
        44.20980116595151, 55.3293826315946, 216.3879551559221, -16.664030110822964,
        21.301855021886226, 288.45400133823387, 176.012275546155, 177.51614975841278,
        -285.62676314391877, -205.2688909726066, 290.26889123039217, -417.6843910629338,
        -221.06587106364884, 236.39012046770614, -31.10582348370538, -27.228309245494433,
        174.64616245887294, -219.80226998791636, -323.9125035522093, 55.42708504784177,
        173.58811909930617, 237.41417184236346, -158.01339578380308, -246.0787837955889,
        216.30759633066663, 244.65752906363525, -90.89145430650593, -302.13229936318857,
        -145.33934833437385, -101.75532059020811, -230.872525997145, -205.61727233843789,
        228.85363573489894, -280.7961379741428, -349.41883344412275, 23.443193017070907,
        -15.036991942154309, 172.07022197787418, -318.03813311078557, 25.44981367075745,
        273.9043942675222, 103.72701510093438, 90.07823420927862, 269.760522176781,
        291.4090137855518, -381.8232305057681, 38.43798222095872, -9.22677395251476,
        328.10514663976716, -297.5631776761533, 218.2388830973869, 74.62101840815124,
        252.3687343165241, -249.73627991205137, 314.03855267382175, -356.5197951599173,
        61.62824310517574, 65.47668332482361, -228.26857313836263, -189.43003029487647,
        208.98603052445347, 20.251725626992844, -261.449787213557, 302.20065293482907,
        -21.81300145934697, -20.489169605757883, -119.40784196855783, -224.013744864684,
        39.659662691433084, -195.3438881072019, 203.98597278226535, 46.95019538972921,
        72.254230406159, 146.21909494358712, -233.9585580279307, 18.288085033378973,
        213.0723959615449, 191.33308300025902, -227.64987027899082, 110.21285468690657,
        -64.89491522150335, 121.4318622053327, 191.06196244938124, -174.86265770065555,
        -70.21816752746454, -84.62922470985143, 286.60566959801787, -23.882310503738697,
        -283.92818429004876, 113.05538849567955, -18.903624019130184, -107.34010687162201,
        -204.88544182407213, 382.6139254547305, -27.67310559986534, 369.61338609428697,
        85.2462903933743, 231.03734351444908, 143.01412395567633, 28.885949843438137,
        15.483444199070604, 23.957874390075524, -211.112200478719, 142.77171070253874,
        -149.27248473984451, -285.5237009713242, 168.94112308848574, 257.48289476668594,
        -117.80838426742737, 247.67506776163324, -213.36574484194156, 242.08196363396942,
        -76.52129700361455, 300.95363256101456, -40.46722510971484, 113.60321972705515,
        -243.25239341271865, 86.32435144081204, 77.46038437516337, 193.75253829169554,
        75.27052584000832, 113.79970833218677, -194.94026338390532, 208.38476195389964,
        -223.87962847025236, -23.848035587997366, 86.41185291326921, -85.13317410473583,
        -167.95597237741114, 87.48485432020662, 174.16403528471432, 253.5107216641466,
        -215.83932337572475, 283.33003747888955, 226.31394244396856, -214.41371753081296,
        -32.84245277025941, 174.34375321549888, -279.9403166351161, 397.1501042241943,
        -141.2356250661263, 134.7992304190143, -176.46379067832382, -20.70334938043516,
        -241.05629941247102, -105.08679410435799, 163.62233054759824, -125.12664212357606,
        181.33263215637507, -286.4452264777662, -320.47591322102284, -214.2101274957399,
        -392.63874713669685, 276.0664687558439, -83.02747949954986, 163.35574293382356,
        -8.495789972773231, -199.89927766300895, -115.84112295078145});
    EXPECT_THAT(bn.get_value().to_vector(), Pointwise(FloatNearPointwise(1e-3), exp_bn_out));

    graph->backward(target);

    // test backward
    std::vector<double> v1_grad_exp
      ({3.1647811530116244e-14, -1.6777306240362158e-14, 4.309494290450953e-14, -2.3413114218114817e-14,
        -6.390298813899439e-14,
        -4.7164101474257337e-14, 5.2314277148443756e-14, 8.318071321661896e-15, 2.3192817101347833e-14,
        4.661178157196227e-15, -5.985116196987804e-14, 2.1876823254695524e-14, 3.32240090341612e-14,
        3.075341524392258e-14, -3.992480314037699e-15, -2.4221994757079672e-14, 2.4583081992431487e-14,
        6.081294306809658e-15, 2.7672567436182837e-14, -2.6810552223185206e-14, 1.7000711617573462e-14,
        1.3572022919262552e-14, -5.3147367899806846e-15, -9.978426779422844e-15, -7.439260659805396e-15,
        -2.795202191084837e-14, 2.917528530181191e-14, -4.909597162556459e-14, 3.665044484677779e-14,
        -1.7415606342462845e-14, -1.3072060059459994e-15, -1.1759033996072622e-14, 1.5002046482300075e-15,
        2.8916155790258643e-15, -7.46948111094333e-15, 9.142896612673387e-16, -6.321210346499857e-15,
        3.974009931339169e-15, 1.1425231250971982e-15, 1.783939150126517e-15, 5.3009656517744486e-15,
        -6.9005276485268265e-15, 1.17409425297285e-15, -1.616186184547252e-15, -1.9199908687082767e-15,
        -1.853386540494468e-15, -3.09585524953816e-15, 1.4880927957982268e-15, -9.391921636722053e-16,
        -3.72315329294133e-15, -8.411784481205573e-15, 1.0133811806638571e-15, 1.8202558306067954e-15,
        -6.630691811619676e-15, -2.975535878067864e-15, -3.448387663177729e-15, 3.6915478427783336e-15,
        5.468239577205532e-15, -4.414678380452468e-15, 9.54150737740842e-15, 9.900653996014253e-15,
        3.760272920363796e-15, -4.41790976776494e-15, -4.698954085852009e-15, 4.779297665467473e-16,
        -4.434414786422115e-15, 1.1641449644313823e-14, 4.905509102117252e-15, 7.337953027802659e-15,
        1.571862715392464e-14, 1.2943603251013806e-14, 6.7277728351932926e-15, 1.2905026059689263e-14,
        6.567535301449308e-15, 9.556251667603872e-15, -2.8520783070788314e-15, -3.0027796223020637e-15,
        9.137825109270406e-15, -1.5624106442621646e-14, -1.8576098035066007e-14, 4.789423859358043e-14,
        -6.4735701948415304e-15, -8.244045571629211e-15, -3.388801767219277e-14, 3.218846709451008e-15,
        -2.8261348389191402e-15, -4.5362474751576e-14, -2.7459345262981098e-14, -2.7698794227028448e-14,
        4.6043470781853087e-14, 3.324877752384276e-14, -4.5651444068953194e-14, 6.706987175476811e-14,
        3.576399487848965e-14, -3.707279052325459e-14, 5.518289331670701e-15, -2.566475214089734e-15,
        1.7160843387698313e-14, -2.1384941193965255e-14, -3.1558668113915087e-14, 5.510669377867014e-15,
        1.705745062945374e-14, 2.329457841944354e-14, -1.5346887956330573e-14, -2.3952701026359277e-14,
        2.123202867654342e-14, 2.4002404497917926e-14, -8.78768354819557e-15, -2.943029089206394e-14,
        -1.4108370921966693e-14, -9.849308376815143e-15, -2.2466734575038348e-14, -3.8547050790737566e-15,
        4.086623940719825e-15, -5.22883645646672e-15, -6.483133102860874e-15, 3.320988864246128e-16,
        -3.712481129379738e-16, 3.0487275864822446e-15, -5.909551623438753e-15, 3.687762209313188e-16,
        4.9100689388793915e-15, 1.799539477468541e-15, 1.5500648687764116e-15, 4.834326579432482e-15,
        5.230021185107829e-15, -7.075425875455722e-15, 6.061760511732864e-16, -4.567538852555167e-16,
        9.236014428302078e-15, -8.741703476597504e-15, 6.079158604747631e-15, 1.952496990049329e-15,
        7.05983289095942e-15, -7.367463415444358e-15, 8.831830216996473e-15, -1.0435740710956515e-14,
        1.5791674796349625e-15, 1.689747110972931e-15, -6.75061841715481e-15, -5.634646359849584e-15,
        5.8132906250098104e-15, 3.9027021538042407e-16, -7.704034921079856e-15, 1.5302943891597765e-14,
        -9.797648483491777e-16, -9.13238119613135e-16, -5.8842135991411415e-15, -1.1140990362211441e-14,
        2.109430536663958e-15, -9.70023957635051e-15, 1.0367346242476014e-14, 2.47580281411434e-15,
        3.74741043475093e-15, 7.464378315331152e-15, -1.1640748610094858e-14, 1.0354413093583956e-15,
        1.0823967684534512e-14, 9.731498597646752e-15, -1.1323717140111516e-14, -1.6982669068095943e-14,
        1.0898235970006844e-14, -1.876897520063711e-14, -2.985557754679981e-14, 2.8407454361169357e-14,
        1.1745811677892312e-14, 1.4040360440696816e-14, -4.5068180776186855e-14, 4.368151450631527e-15,
        4.577302067133402e-14, -1.743526129305232e-14, 3.5754379967142026e-15, 1.7656418992610663e-14,
        3.3187724211914773e-14, -6.035475010326456e-14, 4.971727169454821e-15, 3.621318073917069e-14,
        8.42462357592392e-15, 2.2671430167801807e-14, 1.4069737829337185e-14, 2.9170505522179663e-15,
        1.6073480589978646e-15, 2.4354754736983577e-15, -2.0535741349961843e-14, 1.4046049031850473e-14,
        -1.4492719840975315e-14, -2.7807286709121655e-14, 1.6603342836265596e-14, 2.5255708460882805e-14,
        -1.1418025307795199e-14, 2.429728055027885e-14, -2.075595932631726e-14, 4.328413442102276e-15,
        -1.495068113249705e-15, 5.4044792596439854e-15, -8.360659980507266e-16, 1.980058331425027e-15,
        -4.542605865057296e-15, 1.4814507967817572e-15, 1.3194337834883182e-15, 3.4450404455476643e-15,
        1.2794071975620308e-15, 1.983649781699362e-15, -3.659548995019186e-15, 3.7124905791090995e-15,
        -4.1885073567970405e-15, -5.322977834664098e-16, 1.4830501627452465e-15, -2.6378197888316323e-15,
        -5.017618870187609e-15, 2.322121604584036e-15, 4.812728533281336e-15, 7.092646325152319e-15,
        -6.393481037303345e-15, 7.949463311086025e-15, 6.311184317663322e-15, -6.352518216065078e-15,
        -1.1353179029917791e-15, 4.8178924805942026e-15, -8.235334827010828e-15, 1.121992622624349e-14,
        -4.249846477668876e-15, 3.6816350613036215e-15, -5.262079315851341e-15, -9.240013292549575e-16,
        -1.1997432515567991e-14, -5.164535745874113e-15, 8.338945830094139e-15, -6.1716013558090866e-15,
        9.228944380715437e-15, -1.427836935079101e-14, -1.598851876253994e-14, -1.0648327648614172e-14,
        -1.9614928917059667e-14, 1.3989618655541934e-14, -4.055985566454869e-15, 8.325548961135367e-15,
        -3.105329412260263e-16, -9.929162278614059e-15, -5.7049747117174946e-15});
    std::vector<double> weight_grad_exp
      ({2.5125396639773734e-14, -1.471857369336139e-14, -2.6482453956961875e-15, -4.56341939903464e-15,
        -7.463440452015782e-15,
       });
    std::vector<double> bias_grad_exp({48.0, 48.0, 48.0, 48.0, 48.0,});
    EXPECT_THAT(v1.get_grad().to_vector(), Pointwise(FloatNearPointwise(1e-6), v1_grad_exp));
    EXPECT_THAT(bn_layer.get_weight().get_grad().to_vector(), Pointwise(FloatNearPointwise(1e-6), weight_grad_exp));
    EXPECT_THAT(bn_layer.get_bias().get_grad().to_vector(), Pointwise(FloatNearPointwise(1e-6), bias_grad_exp));
  } catch (const std::exception &ex) {
    FAIL() << "Failed and got this: " << std::endl << ex.what();
  }
  graph->remove_all();
  Graph::delete_global_graph();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
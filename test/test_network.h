/*
    Copyright (c) 2013, Taiga Nomi
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY 
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once
#include "picotest/picotest.h"
#include "testhelper.h"
#include "tiny_cnn/tiny_cnn.h"

namespace tiny_cnn {

TEST(network, in_dim) {
    network<sequential> net;
    convolutional_layer<identity> c1(32, 32, 5, 3, 6, padding::same);
    max_pooling_layer<identity> p1(32, 32, 6, 2);
    net << c1 << p1;

    EXPECT_EQ(c1.in_data_size(), net.in_data_size());
}

TEST(network, out_dim) {
    network<sequential> net;
    convolutional_layer<identity> c1(32, 32, 5, 3, 6, padding::same);
    max_pooling_layer<identity> p1(32, 32, 6, 2);
    net << c1 << p1;

    EXPECT_EQ(p1.out_data_size(), net.out_data_size());
}

TEST(network, name) {

    network<sequential> net1;
    network<sequential> net2("foo");

    EXPECT_EQ(net1.name(), "");
    EXPECT_EQ(net2.name(), "foo");
}

TEST(network, add) {
    network<sequential> net;
    net << convolutional_layer<identity>(32, 32, 5, 3, 6, padding::same);

    EXPECT_EQ(net.out_data_size(), 32*32*6);
    //EXPECT_EQ(net.depth(), 1);
}

TEST(network, train_predict) {
    // train xor function
    network<sequential> net;
    adagrad optimizer;

    std::vector<vec_t> data;
    std::vector<label_t> label;
    size_t tnum = 300;

    optimizer.alpha *= 10;

    for (size_t i = 0; i < tnum; i++) {
        bool in[2] = { bernoulli(0.5), bernoulli(0.5) };
        data.push_back({in[0]*1.0, in[1]*1.0});
        label.push_back((in[0] ^ in[1]) ? 1 : 0);
    }

    net << fully_connected_layer<tan_h>(2, 10)
        << fully_connected_layer<tan_h>(10, 2);

    net.train<mse>(optimizer, data, label, 10, 10);


    for (size_t i = 0; i < tnum; i++) {
        bool in[2] = { bernoulli(0.5), bernoulli(0.5) };
        label_t expected = (in[0] ^ in[1]) ? 1 : 0;
        label_t actual = net.predict_label({ in[0] * 1.0, in[1] * 1.0 });
        EXPECT_EQ(expected, actual);
    }
}

TEST(network, set_netphase) {
    // TODO: add unit-test for public api
}

TEST(network, test) {
    // TODO: add unit-test for public api
}

TEST(network, get_loss) {
    // TODO: add unit-test for public api
}

TEST(network, at) {
    network<sequential> net;
    convolutional_layer<identity> c1(32, 32, 5, 3, 6, padding::same);
    average_pooling_layer<identity> p1(32, 32, 6, 2);

    net << c1 << p1;
    net.init_weight();

    auto& c = net.at<convolutional_layer<identity>>(0);
    auto& p = net.at<average_pooling_layer<identity>>(1);
}

TEST(network, bracket_operator) {
    network<sequential> net;

    net << convolutional_layer<identity>(32, 32, 5, 3, 6, padding::same)
        << average_pooling_layer<identity>(32, 32, 6, 2);

    EXPECT_EQ(net[0]->layer_type(), "conv");
    EXPECT_EQ(net[1]->layer_type(), "ave-pool");
}


TEST(network, weight_init) {
    network<sequential> net;

    net << convolutional_layer<identity>(32, 32, 5, 3, 6, padding::same)
        << average_pooling_layer<identity>(32, 32, 6, 2);

    net.weight_init(weight_init::constant(2.0));
    net.init_weight();

    vec_t& w1 = *net[0]->get_weights()[0];
    vec_t& w2 = *net[1]->get_weights()[0];

    for (size_t i = 0; i < w1.size(); i++)
        EXPECT_NEAR(w1[i], 2.0, 1e-10);

    for (size_t i = 0; i < w2.size(); i++)
        EXPECT_NEAR(w2[i], 2.0, 1e-10);
}

TEST(network, bias_init) {
    network<sequential> net;

    net << convolutional_layer<identity>(32, 32, 5, 3, 6, padding::same)
        << average_pooling_layer<identity>(32, 32, 6, 2);

    net.bias_init(weight_init::constant(2.0));
    net.init_weight();

    vec_t& w1 = *net[0]->get_weights()[1];
    vec_t& w2 = *net[1]->get_weights()[1];

    for (size_t i = 0; i < w1.size(); i++)
        EXPECT_NEAR(w1[i], 2.0, 1e-10);

    for (size_t i = 0; i < w2.size(); i++)
        EXPECT_NEAR(w2[i], 2.0, 1e-10);
}

TEST(network, gradient_check) { // sigmoid - cross-entropy
    typedef cross_entropy loss_func;
    typedef sigmoid activation;
    typedef network<sequential> network;

    network nn;
    nn << fully_connected_layer<activation>(10, 14*14*3)
       << convolutional_layer<activation>(14, 14, 5, 3, 6)
       << average_pooling_layer<activation>(10, 10, 6, 2)
       << fully_connected_layer<activation>(5*5*6, 3);

    vec_t a(10, 0.0);
    label_t t = 2;

    uniform_rand(a.begin(), a.end(), -1, 1);
    nn.init_weight();
    EXPECT_TRUE(nn.gradient_check<loss_func>(&a, &t, 1, 1e-4, GRAD_CHECK_RANDOM));
}

TEST(network, gradient_check2) { // tan_h - mse
    typedef mse loss_func;
    typedef tan_h activation;
    typedef network<sequential> network;

    network nn;
    nn << fully_connected_layer<activation>(10, 14 * 14 * 3)
        << convolutional_layer<activation>(14, 14, 5, 3, 6)
        << average_pooling_layer<activation>(10, 10, 6, 2)
        << fully_connected_layer<activation>(5 * 5 * 6, 3);

    vec_t a(10, 0.0);
    label_t t = 2;

    uniform_rand(a.begin(), a.end(), -1, 1);
    nn.init_weight();
    EXPECT_TRUE(nn.gradient_check<loss_func>(&a, &t, 1, 1e-4, GRAD_CHECK_RANDOM));
}

TEST(network, gradient_check3) { // mixture - mse
    typedef mse loss_func;
    typedef network<sequential> network;

    network nn;
    nn << fully_connected_layer<tan_h>(10, 14 * 14 * 3)
        << convolutional_layer<sigmoid>(14, 14, 5, 3, 6)
        << average_pooling_layer<rectified_linear>(10, 10, 6, 2)
        << fully_connected_layer<identity>(5 * 5 * 6, 3);

    vec_t a(10, 0.0);
    label_t t = 2;

    uniform_rand(a.begin(), a.end(), -1, 1);
    nn.init_weight();
    EXPECT_TRUE(nn.gradient_check<loss_func>(&a, &t, 1, 1e-4, GRAD_CHECK_RANDOM));
}

TEST(network, gradient_check4) { // sigmoid - cross-entropy
    typedef cross_entropy loss_func;
    typedef sigmoid activation;
    typedef network<sequential> network;

    network nn;
    nn << fully_connected_layer<activation>(10, 14 * 14 * 3)
        << convolutional_layer<activation>(14, 14, 5, 3, 6)
        << average_pooling_layer<activation>(10, 10, 6, 2)
        << fully_connected_layer<activation>(5 * 5 * 6, 3);

    vec_t a(10, 0.0);
    label_t t = 2;

    uniform_rand(a.begin(), a.end(), -1, 1);
    nn.init_weight();
    EXPECT_TRUE(nn.gradient_check<loss_func>(&a, &t, 1, 1e-4, GRAD_CHECK_RANDOM));
}

TEST(network, gradient_check5) { // softmax - cross-entropy
    typedef cross_entropy loss_func;
    typedef softmax activation;
    typedef network<sequential> network;

    network nn;
    nn << fully_connected_layer<activation>(10, 14 * 14 * 3)
        << convolutional_layer<activation>(14, 14, 5, 3, 6)
        << average_pooling_layer<activation>(10, 10, 6, 2)
        << fully_connected_layer<activation>(5 * 5 * 6, 3);

    vec_t a(10, 0.0);
    label_t t = 2;

    uniform_rand(a.begin(), a.end(), -1, 1);
    nn.init_weight();
    EXPECT_TRUE(nn.gradient_check<loss_func>(&a, &t, 1, 5e-3, GRAD_CHECK_RANDOM));
}

TEST(network, gradient_check6) { // sigmoid - cross-entropy
    typedef cross_entropy loss_func;
    typedef sigmoid activation;
    typedef network<sequential> network;

    network nn;
    nn << fully_connected_layer<activation>(3, 201)
        << fully_connected_layer<activation>(201, 2);

    vec_t a(3, 0.0);
    label_t t = 1;

    uniform_rand(a.begin(), a.end(), 0, 3);
    nn.init_weight();
    EXPECT_TRUE(nn.gradient_check<loss_func>(&a, &t, 1, 1e-4, GRAD_CHECK_ALL));
}

TEST(network, read_write)
{
    typedef mse loss_func;
    typedef network<sequential> network;

    network n1, n2;

    n1 << convolutional_layer<tan_h>(32, 32, 5, 1, 6) // C1, 1@32x32-in, 6@28x28-out
        << average_pooling_layer<tan_h>(28, 28, 6, 2) // S2, 6@28x28-in, 6@14x14-out
        << convolutional_layer<tan_h>(14, 14, 5, 6, 16) // C3, 6@14x14-in, 16@10x10-in
        << average_pooling_layer<tan_h>(10, 10, 16, 2) // S4, 16@10x10-in, 16@5x5-out
        << convolutional_layer<tan_h>(5, 5, 5, 16, 120) // C5, 16@5x5-in, 120@1x1-out
        << fully_connected_layer<tan_h>(120, 10); // F6, 120-in, 10-out

    n2 << convolutional_layer<tan_h>(32, 32, 5, 1, 6) // C1, 1@32x32-in, 6@28x28-out
        << average_pooling_layer<tan_h>(28, 28, 6, 2) // S2, 6@28x28-in, 6@14x14-out
        << convolutional_layer<tan_h>(14, 14, 5, 6, 16) // C3, 6@14x14-in, 16@10x10-in
        << average_pooling_layer<tan_h>(10, 10, 16, 2) // S4, 16@10x10-in, 16@5x5-out
        << convolutional_layer<tan_h>(5, 5, 5, 16, 120) // C5, 16@5x5-in, 120@1x1-out
        << fully_connected_layer<tan_h>(120, 10); // F6, 120-in, 10-out

    n1.init_weight();
    n2.init_weight();

    std::vector<vec_t> t;
    std::vector<label_t> l;
    t.push_back(vec_t(32*32, 0.0));
    l.push_back(3);
    adagrad optimizer;
    n1.train<loss_func>(optimizer, t, l, 1, 1);

    serialization_test(n1, n2);

    vec_t in(32*32, 0.0);

    auto res1 = n1.predict(in);
    auto res2 = n2.predict(in);

    ASSERT_TRUE(n1.has_same_weights(n2, 1e-6));

    for (int i = 0; i < 10; i++) {
        tiny_cnn::float_t eps = std::abs(res1[i]) * 1e-5;
        ASSERT_TRUE(std::abs(res1[i] - res2[i]) < eps);
    }
}

} // namespace tiny-cnn
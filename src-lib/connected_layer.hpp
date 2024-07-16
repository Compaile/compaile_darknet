#pragma once

#include "activations.hpp"
#include "layer.hpp"
#include "network.hpp"

#ifdef __cplusplus
extern "C" {
#endif
layer make_connected_layer(int batch, int steps, int inputs, int outputs, ACTIVATION activation, int batch_normalize);
size_t get_connected_workspace_size(layer l);

void forward_connected_layer(layer l, network_state state);
void backward_connected_layer(layer l, network_state state);
void update_connected_layer(layer l, int batch, float learning_rate, float momentum, float decay);
void denormalize_connected_layer(layer l);
void statistics_connected_layer(layer l);

#ifdef GPU
void forward_connected_layer_gpu(layer l, network_state state);
void backward_connected_layer_gpu(layer l, network_state state);
void update_connected_layer_gpu(layer l, int batch, float learning_rate, float momentum, float decay, float loss_scale);
void push_connected_layer(layer l);
void pull_connected_layer(layer l);
#endif

#ifdef __cplusplus
}
#endif

#include "softmax_layer.h"
#include "blas.h"
#include "cuda.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

softmax_layer make_softmax_layer(int batch, int inputs, int groups)
{
    assert(inputs%groups == 0);
    fprintf(stderr, "softmax                                        %4d\n",  inputs);
    softmax_layer l = {0};
    l.type = SOFTMAX;
    l.batch = batch;
    l.groups = groups;
    l.inputs = inputs;
    l.outputs = inputs;
    l.output = calloc(inputs*batch, sizeof(float));
    l.delta = calloc(inputs*batch, sizeof(float));

    l.forward = forward_softmax_layer;
    l.backward = backward_softmax_layer;
    #ifdef GPU
    l.forward_gpu = forward_softmax_layer_gpu;
    l.backward_gpu = backward_softmax_layer_gpu;

    l.output_gpu = cuda_make_array(l.output, inputs*batch); 
    l.delta_gpu = cuda_make_array(l.delta, inputs*batch); 
    #endif
    return l;
}

void forward_softmax_layer(const softmax_layer l, network net)
{
    if(l.softmax_tree){
        int i;
        int count = 0;
        for (i = 0; i < l.softmax_tree->groups; ++i) {
            int group_size = l.softmax_tree->group_size[i];
            softmax_cpu(net.input + count, group_size, l.batch, l.inputs, 1, 0, 1, l.temperature, l.output + count);
            count += group_size;
        }
    } else {
        if(l.spatial){
            softmax_cpu(net.input, l.c, l.batch*l.c, l.inputs/l.c, l.w*l.h, 1, l.w*l.h, l.temperature, l.output);
        }else{
            softmax_cpu(net.input, l.inputs/l.groups, l.batch, l.inputs, l.groups, l.inputs/l.groups, 1, l.temperature, l.output);
        }
    }
}

void backward_softmax_layer(const softmax_layer l, network net)
{
    if(l.softmax_tree){
        int i;
        int count = 0;
        for(i = 0; i < l.softmax_tree->groups; ++i){
            int group_size = l.softmax_tree->group_size[i];
            backward_softmax_cpu(l.output + count, l.delta + count, group_size, l.batch, l.inputs, 1, 0, 1, l.temperature, net.delta + count);
            count += group_size;
        }
    } else {
        if(l.spatial){
            backward_softmax_cpu(l.output, l.delta, l.c, l.batch*l.c, l.inputs/l.c, l.w*l.h, 1, l.w*l.h, l.temperature, net.delta);
        } else {
            backward_softmax_cpu(l.output, l.delta, l.inputs/l.groups, l.batch, l.inputs, l.groups, l.inputs/l.groups, 1, l.temperature, net.delta);
        }
    }
}

#ifdef GPU

void pull_softmax_layer_output(const softmax_layer l)
{
    cuda_pull_array(l.output_gpu, l.output, l.inputs*l.batch);
}

void forward_softmax_layer_gpu(const softmax_layer l, network net)
{
    if(l.softmax_tree){
        int i;
        int count = 0;
        for (i = 0; i < l.softmax_tree->groups; ++i) {
            int group_size = l.softmax_tree->group_size[i];
            softmax_gpu(net.input_gpu + count, group_size, l.batch, l.inputs, 1, 0, 1, l.temperature, l.output_gpu + count);
            count += group_size;
        }
    } else {
        if(l.spatial){
            softmax_gpu(net.input_gpu, l.c, l.batch*l.c, l.inputs/l.c, l.w*l.h, 1, l.w*l.h, l.temperature, l.output_gpu);
        }else{
            softmax_gpu(net.input_gpu, l.inputs/l.groups, l.batch, l.inputs, l.groups, l.inputs/l.groups, 1, l.temperature, l.output_gpu);
        }
    }
}

void backward_softmax_layer_gpu(const softmax_layer l, network net)
{
    if(l.softmax_tree){
        int i;
        int count = 0;
        for(i = 0; i < l.softmax_tree->groups; ++i){
            int group_size = l.softmax_tree->group_size[i];
            backward_softmax_gpu(l.output_gpu + count, l.delta_gpu + count, group_size, l.batch, l.inputs, 1, 0, 1, l.temperature, net.delta_gpu + count);
            count += group_size;
        }
    } else {
        if(l.spatial){
            backward_softmax_gpu(l.output_gpu, l.delta_gpu, l.c, l.batch*l.c, l.inputs/l.c, l.w*l.h, 1, l.w*l.h, l.temperature, net.delta_gpu);
        } else {
            backward_softmax_gpu(l.output_gpu, l.delta_gpu, l.inputs/l.groups, l.batch, l.inputs, l.groups, l.inputs/l.groups, 1, l.temperature, net.delta_gpu);
        }
    }
}

#endif

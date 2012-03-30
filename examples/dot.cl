// Copyright (c) 2011-2012, Motorola Mobility, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of the Motorola Mobility, Inc. nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
      CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void texture_filter(read_only image2d_t src_image,
                             __global uchar* dst_buffer) {

   int k[9] = {
       -1, -1, -1,
       -1, 9, -1,
       -1, -1, -1};

   int x = get_global_id(0);
   int y = get_global_id(1);

   /* Compute two-dimensional dot product */
   int pixel =
      k[0] * read_imageui(src_image, sampler, (int2)(x-1, y-1)).s0 +
      k[1] * read_imageui(src_image, sampler, (int2)(x,   y-1)).s0 +
      k[2] * read_imageui(src_image, sampler, (int2)(x+1, y-1)).s0 +
      k[3] * read_imageui(src_image, sampler, (int2)(x-1, y)).s0 +
      k[4] * read_imageui(src_image, sampler, (int2)(x,   y)).s0 +
      k[5] * read_imageui(src_image, sampler, (int2)(x+1, y)).s0 +
      k[6] * read_imageui(src_image, sampler, (int2)(x-1, y+1)).s0 +
      k[7] * read_imageui(src_image, sampler, (int2)(x,   y+1)).s0 +
      k[8] * read_imageui(src_image, sampler, (int2)(x+1, y+1)).s0;

   /* Set output pixel */
   dst_buffer[y*get_global_size(0) + x] = (uchar)clamp(pixel, 0, 255);
}

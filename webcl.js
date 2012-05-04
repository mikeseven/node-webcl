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

"use strict";

var cl = require('./build/Release/webcl.node');

module.exports = function WebCL() {
  return cl;
};

cl.size = {};
cl.size.CHAR = cl.size_CHAR;
cl.size.SHORT = cl.size_SHORT;
cl.size.INT = cl.size_INT;
cl.size.LONG = cl.size_LONG;
cl.size.FLOAT = cl.size_FLOAT;
cl.size.DOUBLE = cl.size_DOUBLE;
cl.size.HALF = cl.size_HALF;

cl.type = {};
cl.type.CHAR      = 0;
cl.type.UCHAR     = 1;
cl.type.SHORT     = 2;
cl.type.USHORT    = 3;
cl.type.INT       = 4;
cl.type.UINT      = 5;
cl.type.LONG      = 6;
cl.type.ULONG     = 7;
cl.type.FLOAT     = 8;
cl.type.HALF      = 9;
cl.type.DOUBLE    = 10;
cl.type.QUAD      = 11;
cl.type.LONG_LONG = 12;

cl.type.VEC2      = 1 << 16;
cl.type.VEC3      = 1 << 17;
cl.type.VEC4      = 1 << 18;
cl.type.VEC8      = 1 << 19;
cl.type.VEC16     = 1 << 20;

cl.type.LOCAL_MEMORY_SIZE = 0xFF;

//////////////////////////////
// WebCL object
//////////////////////////////
function checkObjectType(obj, type) {
  //console.log('checking object: '+obj+' type: '+Object.prototype.toString.call(obj)+' for type '+type);
  return Object.prototype.toString.call(obj) === '[object '+type+']';
}

function isArray(obj) {
  return Object.prototype.toString.call(obj) === '[object Array]';
}

var _getPlatforms = cl.getPlatforms;
cl.getPlatforms = function () {
  if (!(arguments.length === 0)) {
    throw new TypeError('Expected getPlatforms()');
  }
  return _getPlatforms();
}

var _createContext = cl.createContext;
cl.createContext = function (properties, data, callback) {
if (!((properties===null || typeof properties === 'undefined' || typeof properties === 'object') &&
    (callback===null || typeof callback === 'undefined' || typeof callback === 'function')
  )) {
  throw new TypeError('Expected createContext(optional WebCLContextProperties properties, optional any data, optional function callback)');
}
return _createContext(properties, data, callback);
}

var _waitForEvents = cl.waitForEvents;
cl.waitForEvents = function (events) {
  if (!(arguments.length === 1 && typeof events === 'object' )) {
    throw new TypeError('Expected waitForEvents(WebCLEvent[] events)');
  }
  return _waitForEvents(events);
}

var _unloadCompiler = cl.unloadCompiler;
cl.unloadCompiler = function () {
  if (!(arguments.length === 0 )) {
    throw new TypeError('Expected unloadCompiler()');
  }
  return _unloadCompiler();
}

//////////////////////////////
//WebCLCommandQueue object
//////////////////////////////
cl.WebCLCommandQueue.prototype.getInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLCommandQueue.getInfo(CLenum param_name)');
  }
  return this._getInfo(param_name);
}

cl.WebCLCommandQueue.prototype.enqueueNDRangeKernel=function (kernel, offsets, globals, locals, event_list, event) {
  if (!(arguments.length>= 4 && checkObjectType(kernel, 'WebCLKernel') &&
      typeof offsets === 'object' && typeof globals === 'object' && typeof locals === 'object' &&
      (typeof event_list === 'undefined' || typeof event_list === 'object') &&
      (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
      )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueNDRangeKernel(WebCLKernel kernel, int[3] offsets, int[3] globals, int[3] locals, WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueNDRangeKernel(kernel, offsets, globals, locals, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueTask=function (kernel, event_list, event) {
  if (!(arguments.length >= 1 && checkObjectType(kernel, 'WebCLKernel') &&
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
    )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueTask(WebCLKernel kernel, WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueTask(kernel, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueWriteBuffer=function (buffer, blocking_write, offset, cb, ptr, event_list, event) {
    if (!(arguments.length >= 5 &&
      checkObjectType(buffer, 'WebCLBuffer') &&
      (typeof blocking_write === 'boolean' || typeof blocking_write === 'number') &&
      typeof offset === 'number' && typeof cb === 'number' &&
      typeof ptr === 'object' &&
      (typeof event_list === 'undefined' || typeof event_list === 'object') &&
      (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
      )) {
        throw new TypeError('Expected WebCLCommandQueue.enqueueWriteBuffer(WebCLBuffer buffer, boolean blocking_write, ' +
            'uint offset, uint cb, ArrayBuffer ptr, WebCLEvent[] event_list, WebCLEvent event)');
    }
    return this._enqueueWriteBuffer(buffer, blocking_write, offset, cb, ptr, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueReadBuffer=function (buffer, blocking_read, offset, cb, ptr, event_list, event) {
  //console.log('checking object: type: '+Object.prototype.toString.call(ptr)+' for typeof: '+typeof(ptr));
  if (!(arguments.length >= 5 &&
    checkObjectType(buffer, 'WebCLBuffer') &&
    (typeof blocking_read === 'boolean' || typeof blocking_read === 'number') &&
    typeof offset === 'number' && typeof cb === 'number' &&
    typeof ptr === 'object' &&
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
    )) {
      throw new TypeError('Expected WebCLCommandQueue.enqueueReadBuffer(WebCLBuffer buffer, boolean blocking_read, ' +
          'uint offset, uint cb, ArrayBuffer ptr, WebCLEvent[] event_list, WebCLEvent event)');
    }
    return this._enqueueReadBuffer(buffer, blocking_read, offset, cb, ptr, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueCopyBuffer=function (src_buffer, dst_buffer,
                                                           src_offset, dst_offset, size,
                                                           event_list, event) {
  if (!(arguments.length >= 5 && 
      checkObjectType(src_buffer, 'WebCLBuffer') &&
      checkObjectType(dst_buffer, 'WebCLBuffer') &&
      typeof src_offset === 'number' && typeof dst_offset === 'number' && typeof size === 'number' &&
      (typeof event_list === 'undefined' || typeof event_list === 'object') &&
      (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueCopyBuffer(WebCLBuffer src_buffer, WebCLBuffer dst_buffer, ' +
        'int src_offset, int dst_offset, int size, ' +
        'WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueCopyBuffer(src_buffer, dst_buffer,
                                 src_offset, dst_offset, size,
                                 event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueWriteBufferRect=function (buffer, blocking_write,
                                                                buffer_origin, host_origin, region,
                                                                buffer_row_pitch, buffer_slice_pitch,
                                                                host_row_pitch, host_slice_pitch,
                                                                ptr,
                                                                event_list, event) {
    if (!(arguments.length >= 9 &&
      checkObjectType(buffer, 'WebCLBuffer') &&
      (typeof blocking_write === 'boolean' || typeof blocking_write === 'number') &&
      typeof buffer_origin === 'object' && typeof host_origin === 'object' && typeof region === 'object' &&
      typeof host_row_pitch === 'number' && typeof host_slice_pitch === 'number' &&
      typeof ptr === 'object' &&
      (typeof event_list === 'undefined' || typeof event_list === 'object') &&
      (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
      )) {
        throw new TypeError('Expected WebCLCommandQueue.enqueueWriteBufferRect(WebCLBuffer memory_object, ' +
            'boolean blocking_write, uint[3] buffer_origin, uint[3] host_origin, uint[3] region, ' +
            'uint buffer_row_pitch, uint buffer_slice_pitch, uint host_row_pitch, uint host_slice_pitch, ' +
            'ArrayBuffer ptr,' +
            'WebCLEvent[] event_list, WebCLEvent event)');
    }
    return this._enqueueWriteBufferRect(buffer, blocking_write,
        buffer_origin, host_origin, region,
        buffer_row_pitch, buffer_slice_pitch,
        host_row_pitch, host_slice_pitch,
        ptr,
        event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueReadBufferRect=function (buffer, blocking_read,
                                                               buffer_origin, host_origin, region,
                                                               buffer_row_pitch, buffer_slice_pitch,
                                                               host_row_pitch, host_slice_pitch,
                                                               ptr,
                                                               event_list, event) {
    if (!(arguments.length >= 9 &&
      checkObjectType(buffer, 'WebCLBuffer') &&
      (typeof blocking_read === 'boolean' || typeof blocking_read === 'number') &&
      typeof buffer_origin === 'object' && typeof host_origin === 'object' && typeof region === 'object' &&
      typeof host_row_pitch === 'number' && typeof host_slice_pitch === 'number' &&
      typeof ptr === 'object' &&
      (typeof event_list === 'undefined' || typeof event_list === 'object') &&
      (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
      )) {
        throw new TypeError('Expected WebCLCommandQueue.enqueueReadBufferRect(WebCLBuffer buffer, ' +
            'boolean blocking_write, uint[3] buffer_origin, uint[3] host_origin, uint[3] region, ' +
            'uint buffer_row_pitch, uint buffer_slice_pitch, uint host_row_pitch, uint host_slice_pitch, ' +
            'ArrayBuffer ptr,' +
            'WebCLEvent[] event_list, WebCLEvent event)');
    }
    return this._enqueueReadBufferRect(buffer, blocking_read,
        buffer_origin, host_origin, region,
        buffer_row_pitch, buffer_slice_pitch,
        host_row_pitch, host_slice_pitch,
        ptr,
        event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueCopyBufferRect=function (src_buffer, dst_buffer,
                                                               src_origin, dst_origin, region,
                                                               src_row_pitch, src_slice_pitch,
                                                               dst_row_pitch, dst_slice_pitch,
                                                               event_list, event) {
  if (!(arguments.length >= 9 &&
    checkObjectType(src_buffer, 'WebCLBuffer') && checkObjectType(dst_buffer, 'WebCLBuffer') &&
    typeof src_origin === 'object' && typeof dst_origin === 'object' && typeof region === 'object' &&
    typeof src_row_pitch === 'number' && typeof src_slice_pitch === 'number' &&
    typeof dst_row_pitch === 'number' && typeof dst_slice_pitch === 'number' &&
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueCopyBufferRect(WebCLBuffer src_buffer, WebCLBuffer dst_buffer, ' +
        'uint[3] src_origin, uint[3] dst_origin, uint[3] region, ' +
        'uint src_row_pitch, uint src_slice_pitch, ' +
        'uint dst_row_pitch, uint dst_slice_pitch, ' +
        'WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueCopyBufferRect(src_buffer, dst_buffer,
      src_origin, dst_origin, region,
      src_row_pitch, src_slice_pitch,
      dst_row_pitch, dst_slice_pitch,
      event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueWriteImage=function (image, blocking_write, origin, region, row_pitch, slice_pitch, ptr, event_list, event) {
  //console.log('checking object: type: '+Object.prototype.toString.call(ptr)+' for typeof: '+typeof(ptr));
  if (!(arguments.length >= 7 &&
    checkObjectType(image, 'WebCLImage') &&
    typeof origin === 'object' &&
    typeof region === 'object' &&
    typeof row_pitch === 'number' &&
    typeof slice_pitch === 'number' &&
    typeof ptr === 'object' &&
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueWriteImage(WebCLImage image, boolean blocking_write, ' +
      'int[3] origin, int[3] region, int row_pitch, int slice_pitch, ArrayBuffer ptr, WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueWriteImage(image, blocking_write, origin, region, row_pitch, slice_pitch, ptr, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueReadImage=function (image, blocking_read, origin, region, row_pitch, slice_pitch,
                                                          ptr, event_list, event) {
  if (!(arguments.length >= 7 && 
    checkObjectType(image, 'WebCLImage') &&
    typeof origin === 'object' &&
    typeof region === 'object' &&
    typeof row_pitch === 'number' &&
    typeof slice_pitch === 'number' &&
    typeof ptr === 'object' &&
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueReadImage(WebCLImage image, boolean blocking_write, ' +
        'uint[3] region, uint row_pitch, uint slice_pitch, ' +
        'ArrayBuffer ptr, WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueReadImage(image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueCopyImage=function (src_image, dst_image, src_origin, dst_origin, region,
                                                          event_list, event) {
  if (!(arguments.length >= 5 && 
    checkObjectType(src_image, 'WebCLImage') &&
    checkObjectType(dst_image, 'WebCLImage') &&
    typeof src_origin === 'object' &&
    typeof dst_origin === 'object' &&
    typeof region === 'object' &&
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueCopyImage(WebCLImage src_image, WebCLImage dst_image, ' +
        'uint[3] src_origin, uint[3] dst_origin, uint[3] region, ' +
        'WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueCopyImage(src_image, dst_image, src_origin, dst_origin, region, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueCopyImageToBuffer=function (src_image, dst_buffer, src_origin, region, dst_offset,
                                                                  event_list, event) {
  if (!(arguments.length >= 5 && 
    checkObjectType(src_image, 'WebCLImage') &&
    checkObjectType(dst_buffer, 'WebCLBuffer') &&
    typeof src_origin === 'object' &&
    typeof region === 'object' &&
    typeof dst_offset === 'number' &&
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueCopyImageToBuffer(WebCLImage src_image, WebCLBuffer dst_buffer, ' +
        'uint[3] src_origin, uint[3] region, uint dst_offset, ' +
        'WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueCopyImageToBuffer(src_image, dst_buffer, src_origin, region, dst_offset, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueCopyBufferToImage=function (src_buffer, dst_image, src_offset, dst_origin,
                                                                  region, event_list, event) {
  if (!(arguments.length >= 5 && 
    checkObjectType(src_buffer, 'WebCLBuffer') &&
    checkObjectType(dst_image, 'WebCLImage') &&
    typeof src_offset === 'number' &&
    typeof dst_origin === 'object' &&
    typeof region === 'object' &&
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueCopyBufferToImage(WebCLBuffer src_buffer, WebCLImage dst_image, ' +
        'uint src_offset, uint[3] dst_origin, uint[4] region, WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueCopyBufferToImage(src_buffer, dst_image, src_offset, dst_origin, region, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueMapBuffer=function (memory_object, blocking, flags, offset, size, event_list, event) {
  if (!(arguments.length >= 5 && 
    checkObjectType(memory_object, 'WebCLBuffer') &&
    (typeof blocking === 'boolean' || typeof blocking === 'number') &&
    typeof flags === 'number' && 
    typeof offset === 'number' && 
    typeof size === 'number' && 
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueMapBuffer(WebCLBuffer memory_object, boolean blocking, CLenum flags, uint offset, uint size, WebCLEvent[] event_list, WebCLEvent event)');
  }

  return this._enqueueMapBuffer(memory_object, blocking, flags, offset, size, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueMapImage=function (memory_object, blocking, flags, origin, region, event_list, event) {
  if (!(arguments.length >= 5 && 
    checkObjectType(memory_object, 'WebCLImage') &&
    (typeof blocking === 'boolean' || typeof blocking === 'number') &&
    typeof flags === 'number' && 
    typeof origin === 'number' && 
    typeof region === 'object' && 
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueMapImage(WebCLImage memory_object, boolean blocking, CLenum flags, uint origin, WebCLRegion region, WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueMapImage(memory_object, blocking, flags, origin, region, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueUnmapMemObject=function (memory_object, region, event_list, event) {
  if (!(arguments.length >= 2 && 
    (checkObjectType(memory_object, 'WebCLBuffer') || checkObjectType(memory_object, 'WebCLImage')) &&
    typeof region === 'object' && 
    (typeof event_list === 'undefined' || typeof event_list === 'object') &&
    (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueUnmapMemObject(WebCLMemoryObject memory_object, ArrayBuffer region, WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueUnmapMemObject(memory_object, region, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueMarker=function (event_list, event) {
  if (!(arguments.length >= 0 &&
      (typeof event_list === 'undefined' || typeof event_list === 'object') &&
      (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
      )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueMarker(WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueMarker(event_list, event);
}

/*cl.WebCLCommandQueue.prototype.enqueueWaitForEvents=function (event_wait_list) {
  if (!(arguments.length >=0 &&       
      (typeof event_list === 'undefined' || typeof event_list === 'object') )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueWaitForEvents(WebCLEvent[] event_wait_list)');
  }
  return this._enqueueWaitForEvents(event_wait_list);
}*/

cl.WebCLCommandQueue.prototype.enqueueBarrier=function (event_list, event) {
  if (!(arguments.length >= 0 &&
      (typeof event_list === 'undefined' || typeof event_list === 'object') &&
      (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
      )) {
    throw new TypeError('Expected WebCLCommandQueue.enqueueBarrier(WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueBarrier(event_list, event);
}

cl.WebCLCommandQueue.prototype.flush=function () {
  if (!(arguments.length === 0)) {
    throw new TypeError('Expected WebCLCommandQueue.flush()');
  }
  return this._flush();
}

cl.WebCLCommandQueue.prototype.finish=function () {
  if (!(arguments.length === 0)) {
    throw new TypeError('Expected WebCLCommandQueue.finish()');
  }
  return this._finish();
}

cl.WebCLCommandQueue.prototype.enqueueAcquireGLObjects=function (mem_objects, event_list, event) {
  if (!(arguments.length >= 1 && 
      typeof mem_objects === 'object' && 
      (typeof event_list==='undefined' || typeof event_list === 'object') &&
      (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLEvent WebCLGL.enqueueAcquireGLObjects(WebCLMemoryObject[] mem_objects, WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueAcquireGLObjects(mem_objects, event_list, event);
}

cl.WebCLCommandQueue.prototype.enqueueReleaseGLObjects=function (mem_objects, event_list, event) {
  if (!(arguments.length >= 1 && 
      typeof mem_objects === 'object' && 
      (typeof event_list==='undefined' || typeof event_list === 'object') &&
      (typeof event === 'undefined' || checkObjectType(event, 'WebCLEvent'))
  )) {
    throw new TypeError('Expected WebCLEvent WebCLGL.enqueueReleaseGLObjects(WebCLMemoryObject[] mem_objects, WebCLEvent[] event_list, WebCLEvent event)');
  }
  return this._enqueueReleaseGLObjects(mem_objects, event_list, event);
}

//////////////////////////////
//WebCLContext object
//////////////////////////////
cl.WebCLContext.prototype.getInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLContext.getInfo(CLenum param_name)');
  }
  return this._getInfo(param_name);
}

cl.WebCLContext.prototype.createProgram=function (sources) {
  if (!(arguments.length === 1 && typeof sources === 'string')) {
    throw new TypeError('Expected WebCLContext.createProgram(string sources)');
  }
  return this._createProgram(sources);
}

// TODO
cl.WebCLContext.prototype.createProgramWithBinaries=function (devices, binaries) {
  if (!(arguments.length === 2 && typeof devices === 'object' && typeof binaries === 'object')) {
    throw new TypeError('Expected WebCLContext.createProgramWithBinaries(WebCLDevice[] devices, Object[] binaries)');
  }
  return this._createProgramWithBinaries(devices, binaries);
}

cl.WebCLContext.prototype.createCommandQueue=function (device, properties) {
  if (!(arguments.length >=1 && checkObjectType(device, 'WebCLDevice') && 
      (properties==null || typeof properties === 'number' || typeof properties === 'object'))) {
    throw new TypeError('Expected WebCLContext.createCommandQueue(WebCLDevice device, CLenum[] properties)');
  }
  return this._createCommandQueue(device, properties);
}

cl.WebCLContext.prototype.createBuffer=function (flags, size, host_ptr) {
  if (!(arguments.length >= 2 && typeof flags === 'number' && typeof size === 'number' && 
      (host_ptr === null || typeof host_ptr === 'undefined' || typeof host_ptr === 'object') )) {
    throw new TypeError('Expected WebCLContext.createBuffer(CLenum flags, int size, optional ArrayBuffer host_ptr)');
  }
  return this._createBuffer(flags, size, host_ptr);
}

cl.WebCLContext.prototype.createImage=function (flags, descriptor, host_ptr) {
  if (!(arguments.length === 3 && typeof flags === 'number' &&
    typeof descriptor === 'object' &&
    typeof host_ptr === 'object')) {
    throw new TypeError('Expected WebCLContext.createImage3D(CLenum flags, cl.WebCLImageDescriptor descriptor, ' +
      'ArrayBuffer host_ptr)');
  }
  return this._createImage(flags, descriptor, host_ptr);
}

cl.WebCLContext.prototype.createSampler=function (normalized_coords, addressing_mode, filter_mode) {
  if (!(arguments.length === 3 && 
      (typeof normalized_coords === 'number' || typeof normalized_coords === 'boolean') && 
      typeof addressing_mode === 'number' && 
      typeof filter_mode === 'number')) {
    throw new TypeError('Expected WebCLContext.createSampler(bool normalized_coords, CLenum addressing_mode, CLenum filter_mode)');
  }
  return this._createSampler(normalized_coords, addressing_mode, filter_mode);
}

cl.WebCLContext.prototype.createUserEvent=function () {
  if (!(arguments.length === 0)) {
    throw new TypeError('Expected WebCLContext.createUserEvent()');
  }
  return this._createUserEvent();
}

cl.WebCLContext.prototype.getSupportedImageFormats=function (flags, image_type) {
  if (!(arguments.length === 2 && typeof flags === 'number' && typeof image_type === 'number')) {
    throw new TypeError('Expected WebCLContext.getSupportedImageFormats(CLenum flags, CLenum image_type)');
  }
  return this._getSupportedImageFormats(flags, image_type);
}

cl.WebCLContext.prototype.createFromGLBuffer=function (flags, buffer) {
  if (!(arguments.length === 2 && typeof flags === 'number' && typeof buffer ==='object')) {
    throw new TypeError('Expected WebCLContext.createFromGLBuffer(CLenum flags, WebGLBuffer buffer)');
  }
  return this._createFromGLBuffer(flags, buffer ? buffer._ : 0);
}

cl.WebCLContext.prototype.createFromGLTexture2D=function (flags, texture_target, miplevel, texture) {
  if (!(arguments.length === 4 && typeof flags === 'number' && 
      typeof texture_target ==='number' &&
      typeof miplevel ==='number' &&
      typeof texture ==='object' 
    )) {
    throw new TypeError('Expected WebCLContext.createFromGLTexture2D(CLenum flags, GLenum texture_target, GLint miplevel, WebGLTexture2D texture)');
  }
  return this._createFromGLTexture2D(flags, texture_target, miplevel, texture ? texture._ : 0);
}

cl.WebCLContext.prototype.createFromGLTexture3D=function (flags, texture_target, miplevel, texture) {
  if (!(arguments.length === 4 && typeof flags === 'number' && 
      typeof texture_target ==='number' &&
      typeof miplevel ==='number' &&
      typeof texture ==='object' 
    )) {
    throw new TypeError('Expected WebCLContext.createFromGLTexture3D(CLenum flags, GLenum texture_target, GLint miplevel, WebGLTexture3D texture)');
  }
  return this._createFromGLTexture3D(flags, texture_target, miplevel, texture ? texture._ : 0);
}

//////////////////////////////
//WebCLDevice object
//////////////////////////////
cl.WebCLDevice.prototype.getInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLDevice.getInfo(CLenum param_name)');
  }
  return this._getInfo(param_name);
}

cl.WebCLDevice.prototype.getExtension=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLDevice.getExtension(CLenum param_name)');
  }
  return this._getExtension(param_name);
}

//////////////////////////////
//WebCLEvent object
//////////////////////////////
cl.WebCLEvent.prototype.getInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLEvent.getInfo(CLenum param_name)');
  }
  return this._getInfo(param_name);
}

cl.WebCLEvent.prototype.getProfilingInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLEvent.getProfilingInfo(CLenum param_name)');
  }
  return this._getProfilingInfo(param_name);
}

cl.WebCLEvent.prototype.setUserEventStatus=function (execution_status) {
  if (!(arguments.length === 1 && typeof execution_status === 'number')) {
    throw new TypeError('Expected WebCLEvent.setUserEventStatus(CLenum execution_status)');
  }
  return this._setUserEventStatus(execution_status);
}

cl.WebCLEvent.prototype.setCallback=function (execution_status, fct, args) {
  if (!(arguments.length >= 2 && typeof execution_status === 'number' &&
      typeof fct === 'function')) {
    throw new TypeError('Expected WebCLEvent.setCallback(CLenum execution_status, function callback, any args)');
  }
  return this._setCallback(execution_status, fct, args);
}

//////////////////////////////
//WebCLKernel object
//////////////////////////////
cl.WebCLKernel.prototype.getInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLKernel.getInfo(CLenum param_name)');
  }
  return this._getInfo(param_name);
}

cl.WebCLKernel.prototype.getWorkGroupInfo=function (device, param_name) {
  if (!(arguments.length === 2 && checkObjectType(device, 'WebCLDevice') && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLKernel.getWorkGroupInfo(WebCLDevice device, CLenum param_name)');
  }
  return this._getWorkGroupInfo(device, param_name);
}

cl.WebCLKernel.prototype.setArg=function (index, value, type) {
  if (!(arguments.length >= 2 && typeof index === 'number' && 
      (typeof value === 'number' || typeof value === 'object') &&
      (typeof type == 'undefined' || typeof type === 'number') )) {
    throw new TypeError('Expected WebCLKernel.setArg(int index, any value, optional WebCL.types type)');
  }
  return this._setArg(index, value, type);
}

//////////////////////////////
//WebCLMappedRegion object
//////////////////////////////

//static JS_METHOD(getBuffer);

//////////////////////////////
//WebCLMemoryObject object
//////////////////////////////

cl.WebCLMemoryObject.prototype.getInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLMemoryObject.getInfo(CLenum param_name)');
  }
  return this._getInfo(param_name);
}

cl.WebCLMemoryObject.prototype.getGLObjectInfo=function (object_type, param_name) {
  if (!(arguments.length === 1 && typeof object_type === 'number' && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLMemoryObject.getGLObjectInfo(CLenum object_type, GLenum param_name)');
  }
  return this._getGLObjectInfo(object_type, param_name);
}

//////////////////////////////
//WebCLBuffer object
//////////////////////////////

cl.WebCLBuffer.prototype.createSubBuffer=function (flags, type, region) {
  if (!(arguments.length === 3 && typeof flags === 'number' && typeof type === 'number' && typeof region === 'object')) {
    throw new TypeError('Expected WebCLMemoryObject.createSubBuffer(CLenum flags, CLenum type, WebCLRegion region)');
  }
  return this._createSubBuffer(flags, type, region);
}

//////////////////////////////
//WebCLImage object
//////////////////////////////

cl.WebCLImage.prototype.getInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLImage.getInfo(CLenum param_name)');
  }
  return this._getImageInfo(param_name);
}

cl.WebCLImage.prototype.getGLTextureInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLImage.getGLTextureInfo(CLenum param_name)');
  }
  return this._getGLTextureInfo(param_name);
}

//////////////////////////////
//WebCLPlatform object
//////////////////////////////
cl.WebCLPlatform.prototype.getInfo=function (param_name) {
if (!(arguments.length === 1 && typeof param_name === 'number')) {
  throw new TypeError('Expected WebCLPlatform.getInfo(CLenum param_name)');
}
return this._getInfo(param_name);
}

cl.WebCLPlatform.prototype.getDevices=function (device_type) {
  if (!(arguments.length === 1 && typeof device_type === 'number')) {
    throw new TypeError('Expected WebCLPlatform.getDevices(CLenum device_type)');
  }
  return this._getDevices(device_type);
}

//////////////////////////////
//WebCLProgram object
//////////////////////////////
cl.WebCLProgram.prototype.getInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLProgram.getInfo(CLenum param_name)');
  }
  return this._getInfo(param_name);
}

cl.WebCLProgram.prototype.getBuildInfo=function (device, param_name) {
  if (!(arguments.length === 2 && checkObjectType(device, 'WebCLDevice') && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLProgram.getBuildInfo(WebCLDevice device, CLenum param_name)');
  }
  return this._getBuildInfo(device, param_name);
}

cl.WebCLProgram.prototype.build=function (devices, options, data, callback) {
  if (  !(arguments.length === 1 && typeof devices === 'object') &&
        !(arguments.length >= 2 && typeof devices === 'object' && 
            (options==null || typeof options==='undefined' || typeof options === 'string')) &&
        !(typeof data === 'undefined' || typeof data === 'object') &&
        !(typeof callback === 'undefined' || callback==null || typeof callback === 'function')
        ) {
    throw new TypeError('Expected WebCLProgram.build(WebCLDevice[] devices, String options, any data, function callback)');
  }
  return this._build(devices, options, data, callback);
}

cl.WebCLProgram.prototype.createKernel=function (name) {
  if (!(arguments.length === 1 && typeof name === 'string')) {
    throw new TypeError('Expected WebCLProgram.createKernel(String name)');
  }
  return this._createKernel(name);
}

//////////////////////////////
//WebCLSampler object
//////////////////////////////
cl.WebCLSampler.prototype.getInfo=function (param_name) {
  if (!(arguments.length === 1 && typeof param_name === 'number')) {
    throw new TypeError('Expected WebCLSampler.getInfo(CLenum param_name)');
  }
  return this._getInfo(param_name);
}

//////////////////////////////
// extensions
//////////////////////////////

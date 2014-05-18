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

var util=require('util');
var log=console.log;
try {
  ArrayBuffer;
  Int8Array;
} catch (e) {
  var typed_arrays = process.binding('typed_array');
  ArrayBuffer=typed_arrays.ArrayBuffer;
  Int8Array=typed_arrays.Int8Array;
  Int16Array=typed_arrays.Int16Array;
  Int32Array=typed_arrays.Int32Array;
  Int64Array=typed_arrays.Int64Array;
  Float32Array=typed_arrays.Float32Array;
  Float64Array=typed_arrays.Float64Array;
}

// note: ctype field is useless
var ctypes= {
  char : {
    ctype : 'char',
    num : 1,
    fill : function(buffer, offset, values) {
      var fbuf=new Int8Array(buffer, offset);
      if(typeof values == 'string') {
        values=values.charCodeAt(0) & 0xFF;
      }
      fbuf[0]=values;
      return offset + 1;
    }
  },
  short : {
    ctype : 'short',
    num : 2,
    fill : function(buffer, offset, values) {
      var fbuf=new Int16Array(buffer, offset);
      if(typeof values == 'string') {
        values=values.charCodeAt(0);
      }
      fbuf[0]=values;
      return offset + 2;
    }
  },
  int : {
    ctype : 'int',
    num : 4,
    fill : function(buffer, offset, values) {
      var fbuf=new Int32Array(buffer, offset);
      if(typeof values == 'string') {
        values=values.charCodeAt(0);
      }
      fbuf[0]=values;
      return offset + 4;
    }
  },
  long : {
    ctype : 'int',
    num : 8,
    fill : function(buffer, offset, values) {
      var fbuf=new Int64Array(buffer, offset);
      if(typeof values == 'string') {
        values=values.charCodeAt(0);
      }
      fbuf[0]=values;
      return offset + 8;
    }
  },
  float : {
    ctype : 'float',
    num : 4,
    fill : function(buffer, offset, values) {
      var fbuf=new Float32Array(buffer, offset);
      fbuf[0]=values;
      return offset + 4;
    }
  },
  double : {
    ctype : 'float',
    num : 8,
    fill : function(buffer, offset, values) {
      var fbuf=new Float64Array(buffer, offset);
      fbuf[0]=values;
      return offset + 8;
    }
  },
  Array : function(type, num) {
    return {
      ctype : 'Array',
      type : type,
      num : num * type.num,
      fill : function(buffer, offset, values) {
        for(var v=0,l=values.length;v<l;++v) {
          //log('offset= '+offset);
          offset = type.fill(buffer, offset, values[v]);
        }
        return offset;
      }
    };
  },

  size : function(cstruct) {
    var fields = cstruct.length;
    //log("#fields: "+fields);
    var sz=0;
    for(var i=0;i<fields;++i) {
      var field = cstruct[i];
      var arr=[],k=0;
      for(var j in field) {
        for (var i = 0; i < field[j].length; i++) {
          arr[k++]=field[j][i];
        }
      }
      //log(util.inspect(arr[0]))
      //log(arr[0][0].num)
      sz += arr[0].num;
    }

    return sz;
  },

  compute : function(cstruct) {
    var sz=this.size(cstruct);

    var buffer = new ArrayBuffer(sz);
    var offset = 0;

    for(var i=0,li=cstruct.length;i<li;++i) {
      var field = cstruct[i];
      //log('field: '+field);
      var arr=[],k=0;
      for(var j in field) {
        for (var i = 0; i < field[j].length; i++) {
          arr[k++]=field[j][i];
        }
      }
      offset = arr[0].fill(buffer, offset, arr[0][1]);
    }

    return buffer;
  },
};

//log(util.inspect(ctypes));

var cstruct = [
               { "vec" : [ ctypes.float, 1 ] },
               { "mat" : [ ctypes.Array( ctypes.char, 4), [1,2,3,4] ] },
               { "str" : [ ctypes.Array( ctypes.short, 4), ['a','b','c','d'] ] },
];

//log(util.inspect(cstruct));
//log(util.inspect(cstruct[0]));
log('size: '+ctypes.size(cstruct));

var buf=ctypes.compute(cstruct);
var sz=buf.byteLength;
log('buf size: '+sz+' bytes');
var str="";
for(var i=0;i<sz;i++)
  str+= buf[i]+" ";
log('buf vals: '+str);


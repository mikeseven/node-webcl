var util=require('util');
var log=console.log;
var typed_arrays = process.binding('typed_array');
var ArrayBuffer=typed_arrays.ArrayBuffer;
var Int8Array=typed_arrays.Int8Array;
var Int16Array=typed_arrays.Int16Array;
var Int32Array=typed_arrays.Int32Array;
var Int64Array=typed_arrays.Int64Array;
var Float32Array=typed_arrays.Float32Array;
var Float64Array=typed_arrays.Float64Array;

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
        for(var v in values) {
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
    for(var i in cstruct) {
      var field = cstruct[i];
      var arr=[],k=0;
      for(var j in field) {
        arr[k++]=field[j];
      }
      //log(util.inspect(arr[0]))
      //log(arr[0][0].num)
      sz += arr[0][0].num;
    }
  
    return sz;
  },

  compute : function(cstruct) {
    var sz=this.size(cstruct);
  
    var buffer = new ArrayBuffer(sz);
    var offset = 0;
    
    for(var i in cstruct) {
      var field = cstruct[i];
      //log('field: '+field);
      var arr=[],k=0;
      for(var j in field) {
        arr[k++]=field[j];
      }
      offset = arr[0][0].fill(buffer, offset, arr[0][1]);
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
log('buf size: '+buf.byteLength+' bytes');
var str="";
for(var i=0;i<buf.byteLength;i++)
  str+= buf[i]+" ";
log('buf vals: '+str);


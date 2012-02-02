var util=require('util');
var log=console.log;

var ctypes= new Object();
ctypes.bool = 1;
ctypes.short = 2;
ctypes.int = 4;
ctypes.long = 8;
ctypes.float = 4;
ctypes.double = 8;
ctypes.half = 2;
ctypes.Array = function(type, num) {
  this.type=type;
  this.num=num;
  return type * num;
}

log(util.inspect(ctypes));

var cstruct = [
               { "v" : [ ctypes.float, [1,2,3] ] },
               { "m" : [ ctypes.Array( ctypes.float, 4), [1,2,3,4] ] },
               ];

log(util.inspect(cstruct  ));
log(util.inspect(cstruct[0]));

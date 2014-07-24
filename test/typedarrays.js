var log=console.log;

function dumpArray(msg, array) {
  log(msg+':');
  for(var i=0;i<array.length;i++)
    log('  '+array[i]);
}

var buffer=ArrayBuffer(10*4);
var array=new Int32Array(buffer);
array.set([0,1,2,3,4,5,6,7,8,9]);
dumpArray('array',array);
var array3=new Int32Array(array);

var array2=new Int32Array(buffer,4*4,3);
dumpArray('array2',array2); // should be [4,5,6]

array2[0]=-5;
dumpArray('array with [4]=-5',array); // should be [0,1,2,3,-5,5,6,7,8,9]
dumpArray('array3',array3); // should be [0,1,2,3,4,5,6,7,8,9]

var arr4=new Int32Array([0,1,2,3,4]);
var arr4_2=arr4.subarray(2);
var arr4_2_3=arr4.subarray(2,3);
dumpArray('[1,2,3,4].subarray(2): ',arr4_2); // should be [2,3,4]
dumpArray('[1,2,3,4].subarray(2,1): ',arr4_2_3); // should be [2]
arr4[2]=-2;
dumpArray('arr4 with [2]=-2: ',arr4); // should be [0,1,-2,3,4]
dumpArray('arr4_2: ',arr4_2); // should be [-2,3,4]
dumpArray('arr4_2_3: ',arr4_2_3); // should be [-2]

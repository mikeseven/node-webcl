/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var log=console.log;

NUM_ELEMENTS=16;
var buf=new Buffer(NUM_ELEMENTS);
for(var i=0;i<NUM_ELEMENTS;i++)
  buf[i]=0;

var ibuf=new Int32Array(buf,0,NUM_ELEMENTS/4);
log("Setting int32 values in buffer");
for(var i=0;i<NUM_ELEMENTS/4;i++)
  ibuf.set(i,i*1000);

log("mapped buffer:")
for(var i=0;i<NUM_ELEMENTS/4;i++) {
  log(ibuf.get(i));
}

log("buffer:")
for(var i=0;i<NUM_ELEMENTS  ;i++) {
  log(buf[i]);
}

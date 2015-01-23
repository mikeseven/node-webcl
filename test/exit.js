// exit vs. sigkill

var log = console.log;

// make sure all OpenCL resources are released at node exit
process.on('exit',function() {
  log('exit callback called');
});

process.on('SIGINT', function () { 
  log('SIGINT called');
});

setTimeout(function() {
  log('** 10s timeout finished **');
},10000);
var cl=require('../webcl');
var log=console.log;

console.error(new Error('something broke'))

var err=new Error('something broke')
console.error(err.stack.trim())


function WebCLException(name, code, msg) {
  this.name = name;
  this.code=code;
  this.message = msg;
}
require('util').inherits(WebCLException, Error);

console.error(new WebCLException('CL_INVALID_ARG', 12345, "WebCL Exception"))

var platforms=cl.getPlatforms();
var platform = platforms[0];
try {
  var value=platform.getInfo(12345);
}
catch(err) {
  log("Error: ")
  log("  name: "+err.name)
  log("  message: "+err.message)
  log("  code: "+err.code)
  log("  stack: "+err.stack)
}
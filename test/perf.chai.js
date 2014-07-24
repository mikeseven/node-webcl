var chai=require('chai'),expect=chai.expect, assert=chai.assert,
  should=require('should'),
	cl=require("../webcl"),
	Benchmark=require('benchmark'),
	log=console.log;
var suite=new Benchmark.Suite("Performance of getting JS class name");

var platforms=cl.getPlatforms();
var platform=platforms[0];

log('typeof [] '+(typeof ([])))
log('typeof platform '+(typeof platform))
log('instanceof cl.WebCLPlatform '+(platform instanceof cl.WebCLPlatform))
log('proto '+(Object.prototype.toString.call(platform)))
log('expect '+(expect(platform).instanceof(cl.WebCLPlatform)))
log('assert '+(assert.instanceOf(platform, cl.WebCLPlatform)))
log('should '+(platform.should.be.instanceOf(cl.WebCLPlatform)))

suite.add('typeof',function() {
	typeof platform === 'object';
})
.add('instanceof', function() {
	platform instanceof cl.WebCLPlatform;
})
.add('proto', function() {
  Object.prototype.toString.call(platform) ==='[object WebCLPlatform]';
})
.add('expect', function() {
  expect(platform).to.be.an.instanceof(cl.WebCLPlatform);
})
.add('assert', function() {
  assert.instanceOf(platform, cl.WebCLPlatform);
})
.add('should', function() {
  platform.should.be.instanceOf(cl.WebCLPlatform);
})
// add listeners
.on('start', function(event) {
  log('Benchmark started...');
})
.on('cycle', function(event) {
  log('  '+String(event.target));
})
.on('complete', function() {
	log('  --> Fastest is ' + this.filter('fastest').pluck('name'));
  log('Benchmark completed.')
})
// run async
.run({ 'async': false });

// log('typeof '+(typeof platform))
// log('instanceof '+(platform instanceof cl.WebCLPlatform))
// expect(platform).to.be.an.instanceof(cl.WebCLPlatform)

// var devices=platform.getDevices(cl.DEVICE_TYPE_ALL);
// for(var i=0;i<devices.length;i++) {
// 	var device=devices[i];
// 	log('Device '+i+' is instanceof WebCLDevice? '+(device instanceof cl.WebCLDevice))
// 	expect(device).to.be.an.instanceof(cl.WebCLDevice);
// }

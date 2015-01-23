
var chai = require('chai')
  , expect = chai.expect
  , should = chai.should();
var log=console.log;

function dumpArray(msg, array) {
  log(msg+':');
  for(var i=0;i<array.length;i++)
    log('  '+array[i]);
}

function testTypedArray(typedArray, array) {
  var ok=true;
  for(var i=0;i<typedArray.length;i++)
    ok &= typedArray[i]==array[i];
  return ok;
}

describe("TypedArray tests", function() {
  var buffer=new ArrayBuffer(10*4);
  var array=new Int32Array(buffer);
  array.set([0,1,2,3,4,5,6,7,8,9]);
  var array2=new Int32Array(buffer,4*4,3);
  var array3=new Int32Array(array);

  before(function() {
  });

  describe("init", function() {
    it('TypedArray initialized correctly', function() {
      expect(array).to.satisfy(function(typedArray){
        return testTypedArray(typedArray, [0,1,2,3,4,5,6,7,8,9]);      
      });
    });
  });
  
  describe("subarrays", function() {
    it('subarray at 2', function() {
      expect(array2).to.satisfy(function(typedArray){
        return testTypedArray(array2, [4,5,6]);      
      });
    });

    it('change subarray[0], verify main array', function(){
      array2[0]=-5;
      expect(array).to.satisfy(function(typedArray){
        return testTypedArray(typedArray, [0,1,2,3,-5,5,6,7,8,9]);      
      });
    })
    it('verify no change in duplicated main array', function(){
      expect(array3).to.satisfy(function(typedArray){
        return testTypedArray(typedArray, [0,1,2,3,4,5,6,7,8,9]);      
      });
    })

    /*var arr4=new Int32Array([0,1,2,3,4]);
    var arr4_2=arr4.subarray(2);
    var arr4_2_3=arr4.subarray(2,3);
    it('subarray(begin)', function(done){
      expect(arr4_2).to.satisfy(function(typedArray){
        return testTypedArray(typedArray, [2,3,4]);      
      });
      done();      
    })
    it('subarray(begin,end)', function(done){
      expect(arr4_2_3).to.satisfy(function(typedArray){
        return testTypedArray(typedArray, [2]);      
      });
      done();      
    })

    arr4[2]=-2;
    it('verify modified original array', function(done){
      expect(arr4).to.satisfy(function(typedArray){
        return testTypedArray(typedArray, [0,1,-2,3,4]);      
      });
      done();      
    })
    it('verify modified subarray(begin)', function(done){
      expect(arr4_2).to.satisfy(function(typedArray){
        return testTypedArray(typedArray, [-2,3,4]);      
      });
      done();      
    })
    it('verify modified subarray(begin,end)', function(done){
      expect(arr4_2_3).to.satisfy(function(typedArray){
        return testTypedArray(typedArray, [-2]);      
      });
      done();      
    })*/
  })
});


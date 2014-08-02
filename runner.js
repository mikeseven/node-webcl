var fs = require('fs'),
Mocha = require("mocha"),
path = require('path');

// Our Mocha runner
var mocha = new Mocha({
  ui:"bdd",
  reporter:"spec",
  timeout:5000,
  slow:1000
});

// Files which need to be ignored
var avoided = [
"node_modules"
];

// Add the tests to the Mocha instance
(addFiles = function(dir){
  fs.readdirSync(dir).filter(function(file){
   if(!~avoided.indexOf(file)){
     if(fs.statSync(dir + '/' + file).isDirectory()){
      addFiles(dir + '/' + file);
    }
    return file.substr(-3) === '.js';
  }
}).forEach(function(file){
 mocha.addFile(dir + '/' + file);
});
})(path.join(process.cwd(), process.argv[2] || "."));

// Run the files in Mocha
mocha.run(function(failures){
  process.exit(failures);
});
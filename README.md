Important Notes
===============
node-webcl will be soon superseeded by [https://github.com/mikeseven/node-opencl](node-opencl). Node-opencl is a reimplementation of node.js bindings to OpenCL:

* true wrapper of OpenCL, not an object-oriented model as WebCL. This makes it easier to develop higher-level API
* supports OpenCL 1.1, 1.2, 2.0
* support for Nan2 and Node.JS 4
* no dependency on external packages such as node-webgl, which was a huge issue to install for many users
* Lots of unit tests

Introduction
============
This is an implementation of Khronos [WebCL][WEBCL] specification using NodeJS.
This implementation solely relies on [OpenCL][OPENCL] 1.1 C headers. 

Dislaimer
---------
While we are close to the WebCL specification, some features in this code may or may not be available in the final specification. As such, this implementation should be considered for 

* experimental development of WebCL/WebGL content,
* experimenting with new constructs that may not be available in browsers, 
* coupled with Node.JS awesome capabilities, this software opens the door to non-browser applications (e.g. fast server-side image processing).

This implementation is not secure nor robust, we will update as the standard progresses in these areas. So beware that hacking your GPU may crash your computer; don't blame us!

License
-------
node-webcl is distributed under BSD license

Copyright (c) 2011-2012, Motorola Mobility, Inc.
All rights reserved.

See LICENSES in this distribution for all licenses used in samples from other companies.

Dependencies
------------
- [NAN][NAN] must be installed first to support all versions of v8

- [node-webgl][NODE_WEBGL]. This module is used for samples using WebGL interoperability with WebCL.
In turns, [node-webgl][NODE_WEBGL] relies on [node-glfw][NODE_GLFW] that relies on [GLFW][GLFW], [GLEW][GLEW], [AntTweakBar][ANTTWEAKBAR], and FreeImage. See node-webgl and node-glfw for instructions on how to install these modules.

- OpenCL 1.1 must be installed on your machine. Typically, this means your machine has a not too old graphic card (maybe not more than 3 years old) and its latest graphic drivers installed.

On Mac, we recommend using OSX 10.7 "Lion" since OSX 10.6 "Snow Leopard" only supports OpenCL 1.0 and is buggy.

On Windows, use Windows 7. Note that if your machine is 64-bit, you should use node.js 64-bit distribution, not the 32-bit default to avoid mismatch between node libraries and these native dependencies when node-gyp build the modules.

On Linux, make sure you use the latest AMD or NVidia drivers. This module has been tested with Ubuntu 10.10, 11.04 and 11.10 64-bit.

Pre-built binaries are available in submodule deps. Don't forget to do:

	git submodule init
	git submodule udpate
	
if you need these binaries. 

Installation
------------
Make sure GLEW, GLFW, AntTweakBar, and FreeImage libraries are in your path.

- on Windows, put DLLs in Windows\System32. Put headers in <Visual Studio>\include and static librairies in <Visual Studio>\lib for 32-bit libraries (if you use node.js in 32-bit) or <Visual Studio>\lib\x64 (if you use 64-bit node.js).
- on Mac, use homebrew

	brew install freeimage anttweakbar glfw3 glew

- on Linux use you package manager to install these libraries


Now install the usual way: 

	npm install node-webcl

this will also install [https://github.com/mikeseven/node-webgl](node-webgl), [https://github.com/mikeseven/node-glfw](node-glfw), [https://github.com/mikeseven/node-image](node-image), and [https://github.com/rvagg/nan](nan).

If you want to use the latest code, retrieve each repo (node-webcl, node-webgl, node-glfw, and node-image) from github and simply do

	node-gyp rebuild
	npm link
	

A crash course on WebCL
=======================
WebCL is a JavaScript representation of OpenCL 1.1. It is not a straight mapping of OpenCL C methods but rather an object-oriented representation of OpenCL object model. Knowledge of OpenCL is therefore mandatory to be able to develop WebCL programs. See the Books section and/or jump directly into Specifications references at the end of this page.

There are few steps in creating a WebCL program:

1. Init WebCL
	 1.	find a suitable platform/device (`cl.getPlatform()`, `cl.getDevices()`)
	 2.	create a context (`context = cl.createContext()`)
	 3.	create a command queue (`queue = context.createCommandQueue()`)
	 4.	compile a program (`program = context.createProgram()`, `program.build()`)
	 5.	set up a kernel and its arguments (`kernel = program.createKernel()`, `kernel.setArg()`)
	 6. set up commands (`queue.enqueueXXX()`)
2. Run computation
	 1. set up kernel arguments (`kernel.setArg()`)
	 2. set up commands (`queue.enqueueXXX()`)
	 3. launch the computation (`queue.enqueueTask()` or `queue.enqueueNDRange()`)

When used with WebGL, WebGL context must be created first because WebCL context is created with sharing the WebGL context. Remember that WebCL allows computations on WebGL buffers. WebCL doesn't do any rendering. By using WebCL and WebGL together, data remains in the device and this avoids expensive data transfer to/from CPU memory.

So the sequence becomes:

1. init WebGL
	 1. init buffers
	 2. init shaders
	 3. init textures
2. init WebCL (`context = cl.createContext({ sharedObject: gl })`)
3. init shared objects (e.g. textures/array/pixel/render buffers)
4. launch WebGL rendering loop
	* ... execute GL commands
	* acquire GL objects (`queue.enqueueAcquireGLObjects()`)
	* launch CL computation (`queue.enqueueNDRange()`)
	* release GL objects (`queue.enqueueReleaseGLObjects()`)
	* ... execute GL commands
	
References
==========
Specifications

* *Khronos OpenCL specification*, [http://www.khronos.org/registry/cl/][OPENCL]
* *Khronos WebCL working draft*, [https://cvs.khronos.org/svn/repos/registry/trunk/public/webcl/spec/latest/index.html][WEBCL]
* *Khronos WebGL specification*, [http://www.khronos.org/webgl/][WEBGL]

Books

* *Heterogeneous Computing with OpenCL*, Benedict Gaster, Lee Howes, David R. Kaeli and Perhaad Mistry, Morgan Kaufmann, August 2011
* *OpenCL Programming Guide by Aaftab Munshi*, Benedict Gaster, Timothy G. Mattson and James Fung, Addison-Wesley Professional, July 2011
* *OpenCL in Action: How to Accelerate Graphics and Computations*, Matthew Scarpino, Manning Publications, November 2011
* *The OpenCL Programming Book*, Ryoji Tsuchiyama, Takashi Nakamura, Takuro Iizuka and Akihiro Asahara, Fixstars Corporation, April 2010, [http://www.fixstars.com/en/opencl/book/OpenCLProgrammingBook/contents.html](http://www.fixstars.com/en/opencl/book/OpenCLProgrammingBook/contents.html)
* *OpenCL Programming Guide for Mac OS X*, [http://developer.apple.com/library/mac/documentation/Performance/Conceptual/OpenCL_MacProgGuide/OpenCL_MacProgGuide.pdf](http://developer.apple.com/library/mac/documentation/Performance/Conceptual/OpenCL_MacProgGuide/OpenCL_MacProgGuide.pdf)

OpenCL SDKs (use latest!)

* **NVidia graphic cards**: *NVidia CUDA/OpenCL SDK*, [http://developer.nvidia.com/opencl](http://developer.nvidia.com/opencl)
* **AMD/ATI graphics cards**: *AMD Accelerated Parallel Processing SDK*, [http://developer.amd.com/zones/openclzone/Pages/default.aspx](http://developer.amd.com/zones/openclzone/Pages/default.aspx)
* **Intel CPUs**: *Intel OpenCL SDK*, [http://software.intel.com/en-us/articles/vcsource-tools-opencl-sdk/](http://software.intel.com/en-us/articles/vcsource-tools-opencl-sdk/)

[OPENCL]: http://www.khronos.org/registry/cl/ "Khronos OpenCL specification"
[WEBCL]: https://cvs.khronos.org/svn/repos/registry/trunk/public/webcl/spec/latest/index.html "Khronos WebCL working draft"
[NODE_WEBGL]: https://github.com/mikeseven/node-webgl "node-webgl"
[NODE_GLFW]: https://github.com/mikeseven/node-glfw "node-glfw"
[WEBGL]: http://www.khronos.org/webgl/ "Khronos WebGL specification"
[GLFW]: http://www.glfw.org/ "GLFW"
[ANTTWEAKBAR]: "http://www.antisphere.com/Wiki/tools:anttweakbar" "AntTweakBar"
[GLEW]: http://glew.sourceforge.net/ "GLEW: The OpenGL Extension Wrangler Library"
[NAN]: https://github.com/rvagg/nan "Native Abstractions for Node.js"

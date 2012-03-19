/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

Introduction
============
This is an implementation of Khronos WebCL specification using NodeJS.
This implementation solely relies on OpenCL 1.1 C headers.

Dependencies
------------
- node-webgl
This module is used for samples using WebGL interoperability with WebCL.
In turns, node-webgl relies on node-glfw that relies on GLFW, GLEW, AntTweakBar. See node-webgl and node-glfw for instructions on how to install these modules.

- OpenCL 1.1
OpenCL 1.1 must be installed on your machine. Typically, this means your machine has a not too old graphic card (maybe not more than 3 years old) and its latest graphic drivers installed.

On Mac, you must use OSX 10.7 "Lion" at minimum since OSX 10.6 "Snow Leopard" only supports OpenCL 1.0 and is buggy.

On Windows, use Windows 7.

On Linux, make sure you use the latest AMD or NVidia drivers. This module has been tested with Ubuntu 10.10, 11.04 and 11.10.

Installation
------------
Since OpenCL is a compute specification, rendering is not the main purpose but if you want to use graphic acceleration, you should first install node-glfw and then node-webgl and make sure some of node-webgl samples are working. See node-webgl for instructions.

Then you can proceed with installing node-webcl, the usual way: npm install node-webcl.

A crash course on WebCL
=======================
WebCL is a JavaScript representation of OpenCL 1.1. It is not a straight mapping of OpenCL C methods but rather an object-oriented representation of OpenCL object model.
There are few steps in creating a WebCL program:

1. Init WebCL
	 1.	find a suitable platform/device
	 2.	create a context
	 3.	create a command queue
	 4.	compile a program
	 5.	set up a kernel and its arguments
2. Run computation
	1. set up kernel arguments
	2. launch the computation

When used with WebGL, WebGL context must be create first because WebCL context is created with sharing the WebGL context. So the sequence becomes:

1. init WebGL
	 1. init buffers
	 2. init shaders
	 3. init textures
2. init WebCL
3. init shared objects
4. launch WebGL rendering loop
	* ... execute GL commands
	* acquire GL objects
	* launch CL computation
	* release GL objects
	* ... execute GL commands
	

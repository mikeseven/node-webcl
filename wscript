#
## This file contains proprietary software owned by Motorola Mobility, Inc. 
## No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. 
## 
## (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  
#

import Options
from os import unlink, symlink, popen, environ
from os.path import exists 

top='.'
srcdir = "."
blddir = "build"
VERSION = "0.1.0"
OPENCL_PATH="/usr/local/cuda"

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool('node_addon')

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.target = "_webcl"
  obj.source  = bld.path.ant_glob('src/*.cpp')
  obj.cxxflags = ["-g", "-I"+OPENCL_PATH+"/include", 
                  "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE","-fPIC"]
  obj.ldflags = [ "-lOpenCL"]

def shutdown():
  if Options.commands['clean']:
    if exists('_webcl.node'): unlink('_webcl.node')
  else:
    if exists('build/Release/_webcl.node') and not exists('_webcl.node'):
      symlink('build/Release/_webcl.node', '_webcl.node')

#
## This file contains proprietary software owned by Motorola Mobility, Inc. 
## No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. 
## 
## (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  
#

import Options
import sys
from os import unlink, symlink, popen, environ
from os.path import exists 

top='.'
srcdir = "."
blddir = "build"
VERSION = "0.1.0"

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool('node_addon')

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.target = "node_webcl"
  obj.source  = bld.path.ant_glob('src/*.cc')
  obj.cxxflags = ["-g", "-pthread"]
  if sys.platform.startswith('darwin'):
    obj.framework = ['OpenGL','OpenCL']
  elif sys.platform.startswith('linux'):
    obj.ldflags = [ "-lOpenCL","-lGL" ]

def shutdown():
  if Options.commands['clean']:
    if exists('node_webcl.node'): unlink('node_webcl.node')
  else:
    if exists('build/Release/node_webcl.node') and not exists('node_webcl.node'):
      symlink('build/Release/node_webcl.node', 'node_webcl.node')

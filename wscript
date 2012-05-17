# Copyright (c) 2011-2012, Motorola Mobility, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the Motorola Mobility, Inc. nor the names of its
#    contributors may be used to endorse or promote products derived from this
#    software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import Options
import sys
from os import unlink, symlink, popen, environ
from os.path import exists 

top='.'
srcdir = "."
blddir = "build"
VERSION = "0.6.0"

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool('node_addon')

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.target = "node-webcl"
  obj.source  = bld.path.ant_glob('src/*.cc')
  obj.cxxflags = ["-pthread"]
  #obj.cxxflags += ["-g", "-DLOGGING"]
  if sys.platform.startswith('darwin'):
    obj.framework = ['OpenGL','OpenCL']
  elif sys.platform.startswith('linux'):
    obj.ldflags = [ "-lOpenCL","-lGL" ]

def shutdown():
  if Options.commands['clean']:
    if exists('webcl.node'): unlink('webcl.node')
  else:
    if exists('build/Release/webcl.node') and not exists('webcl.node'):
      symlink('build/Release/webcl.node', 'webcl.node')

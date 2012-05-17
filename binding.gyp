{
  'targets': [
    {
      'target_name': 'node-webcl',
      'defines': [
        'VERSION=0.6.0'
      ],
      'sources': [ 
        'src/bindings.cc',
        'src/commandqueue.cc',
        'src/context.cc',
        'src/device.cc',
        'src/event.cc',
        'src/kernel.cc',
        'src/memoryobject.cc',
        'src/platform.cc',
        'src/program.cc',
        'src/sampler.cc',
        'src/webcl.cc',
      ],
      'conditions': [
        ['OS=="mac"', {'libraries': ['-framework OpenGL', '-framework OpenCL']}],
      ],
    }
  ]
}

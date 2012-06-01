{
  'targets': [
    {
      'target_name': 'webcl',
      'defines': [
        'VERSION=0.6.0',
#        'LOGGING'
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
        ['OS=="linux"', {'libraries': ['-lGL', '-lOpenCL']}],
        ['OS=="win"', {
          'variables' : {
            'OPENCL_SDK' : 'C:\\Program Files (x86)\\AMD APP'
          },
          'include_dirs' : ['<(OPENCL_SDK)\\include'],
          'defines' : [
            'WIN32_LEAN_AND_MEAN',
            'VC_EXTRALEAN',
           ],
          'cflags' : [
            '/Ox','/Ob2','/Oi','/Ot','/Oy','/GL','/GF','/Gm-','/EHsc','/MT','/GS','/Gy','/GR-','/Gd','/wd"4530"','/wd"4251"' 
          ],
          'ldflags' : [
            '/OPT:REF','/OPT:ICF','/LTCG'
          ],
          'libraries': ['opengl32.lib', '<(OPENCL_SDK)\\lib\\x86_64\\opencl.lib'],
        },
       ],
    ]
  }]
}

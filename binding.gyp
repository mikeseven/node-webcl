{
    'targets': [
    {
      'target_name': 'webcl',
      'defines': [
        'VERSION=0.9.0',
        # 'LOGGING'
      ],
      'sources': [
        'src/bindings.cc',
        'src/commandqueue.cc',
        'src/context.cc',
        'src/device.cc',
        'src/event.cc',
        'src/exceptions.cc',
        'src/kernel.cc',
        'src/memoryobject.cc',
        'src/platform.cc',
        'src/program.cc',
        'src/sampler.cc',
        'src/webcl.cc',
      ],
      'include_dirs' : [
        "<!(node -e \"require('nan')\")",
      ],
      'conditions': [
        ['OS=="mac"', {
          'make_global_settings': [
            ['CC', '/usr/bin/clang'],
            ['CXX', '/usr/bin/clang++'],
          ],
          "xcode_settings": {
#              'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11','-stdlib=libc++'],
#              'OTHER_LDFLAGS': ['-stdlib=libc++'],
              'MACOSX_DEPLOYMENT_TARGET': '10.7'
          },
          'libraries': ['-framework OpenGL', '-framework OpenCL']
        }],
        ['OS=="linux"', {'libraries': ['-lGL', '-lOpenCL']}],
        ['OS=="win"', {
          'variables' :
            {
            # AMD APP SDK
              'AMD_OPENCL_SDK' : '<!(echo %AMDAPPSDKROOT%)',
              'AMD_OPENCL_SDK_INCLUDE' : '<(AMD_OPENCL_SDK)\\include',
              'AMD_OPENCL_SDK_LIB' : '<(AMD_OPENCL_SDK)\\lib\\x86_64',

            # Intel OpenCL SDK
              'INTEL_OPENCL_SDK' : '<!(echo %INTELOCLSDKROOT%)',
              'INTEL_OPENCL_SDK_INCLUDE' : '<(INTEL_OPENCL_SDK)\\include',
              'INTEL_OPENCL_SDK_LIB' : '<(INTEL_OPENCL_SDK)\\lib\\x64',
            },
            'include_dirs' : [
              "<(AMD_OPENCL_SDK_INCLUDE)", "<(INTEL_OPENCL_SDK_INCLUDE)"
            ],
            'library_dirs' : [
              "<(AMD_OPENCL_SDK_LIB)", "<(INTEL_OPENCL_SDK_LIB)"
            ],
            'defines' : [
              'WIN32_LEAN_AND_MEAN',
              'VC_EXTRALEAN',
            ],
            'cflags' : [
              '/O2','/Oy','/GL','/GF','/Gm-','/EHsc','/MT','/GS','/Gy','/GR-','/Gd'
            ],
            'ldflags' : [
              '/OPT:REF','/OPT:ICF','/LTCG'
            ],
            'libraries': ['opengl32.lib', 'OpenCL.lib'],
          },
       ],
    ]
  }]
}

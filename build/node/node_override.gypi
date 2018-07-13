{
  'variables': {
    # Node disables the inspector unless icu is enabled. But node doesn't know
    # that we're building v8 with icu, so force it on.
    'v8_enable_inspector': 1,

    # By default, node will build a dylib called something like
    # libnode.$node_module_version.dylib, which is inconvenient for our
    # purposes (since it makes the library's name unpredictable). This forces
    # it to drop the module_version from the filename and just produce
    # `libnode.dylib`.
    'shlib_suffix': 'dylib',
  },
  'target_defaults': {
    'target_conditions': [
      ['_target_name=="node_lib"', {
        'include_dirs': [
          '../../../v8',
          '../../../v8/include',
          '../../../third_party/icu/source/common',
          '../../../third_party/icu/source/i18n',
        ],
        'defines': [
          'EVP_CTRL_AEAD_SET_IVLEN=EVP_CTRL_GCM_SET_IVLEN',
          'EVP_CTRL_CCM_SET_TAG=EVP_CTRL_GCM_SET_TAG',
          'EVP_CTRL_AEAD_GET_TAG=EVP_CTRL_GCM_GET_TAG',
        ],
        'conditions': [
          ['OS=="win"', {
            'libraries': [
              '-lv8.dll',
              '-lv8_libbase.dll',
              '-lv8_libplatform.dll',
              '-licuuc.dll',
              '-ldbghelp',
            ],
            'msvs_settings': {
              # Change location of some hard-coded paths.
              'VCLinkerTool': {
                'AdditionalOptions!': [
                  '/WHOLEARCHIVE:<(PRODUCT_DIR)\\lib\\zlib<(STATIC_LIB_SUFFIX)',
                  '/WHOLEARCHIVE:<(PRODUCT_DIR)\\lib\\libuv<(STATIC_LIB_SUFFIX)',
                ],
                'AdditionalOptions': [
                  '/WHOLEARCHIVE:<(PRODUCT_DIR)\\obj\\third_party\\electron_node\\deps\\zlib\\zlib<(STATIC_LIB_SUFFIX)',
                  '/WHOLEARCHIVE:<(PRODUCT_DIR)\\obj\\third_party\\electron_node\\deps\\uv\\libuv<(STATIC_LIB_SUFFIX)',
                ],
              },
            },
          }, {
            'libraries': [
              '-lv8',
              '-lv8_libbase',
              '-lv8_libplatform',
              '-licuuc',
            ]
          }]
        ]
      }],
    ],
  },
}

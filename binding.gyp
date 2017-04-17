{
  'targets': [
    {
      'target_name': 'rocksdb',
      'sources': [
        "rocksdb.cc",
        "RocksDBNode.cc"
      ],
      'include_dirs' : [
        "<!(node -e \"require('nan')\")"
      ],
      'xcode_settings': {
        'MACOSX_DEPLOYMENT_TARGET': '10.9',
      },
      'cflags_cc': [
        '-fexceptions'
      ],
      'libraries': [
        "librocksdb.a"
      ],
    }
  ]
}

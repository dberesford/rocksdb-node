{
  'targets': [
    {
      'target_name': 'rocksdb',
      'sources': [
        "rocksdb.cc",
        "PutWorker.cc",
        "GetWorker.cc",
        "DeleteWorker.cc",
        "OptionsHelper.cc",
        "Iterator.cc",
        "Batch.cc",
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

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
        "Snapshot.cc",
        "Batch.cc",
        "FileWriter.cc",
        "DBNode.cc",
        "DBWithTTL.cc",
        "CompactRangeWorker.cc",
        "MultiGetWorker.cc"
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
        "-lrocksdb"
      ],
    }
  ]
}

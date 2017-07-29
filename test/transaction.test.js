const os = require('os')
const fs = require('fs')
const test = require('tap').test
const rimraf = require('rimraf')
const rocksdb = require('../build/Release/rocksdb.node')
const path = os.tmpdir() + '/rocksdbTransactionTest'

test('setup', function (t) {
  rimraf.sync(path)
  t.end()
})

test('Basic transaction test', function (t) {
  const db = rocksdb.openTransactionDB({create_if_missing: true}, path)
  t.ok(db)

  const tnx = db.beginTransaction({}, {})
  tnx.put('k1', 'v1')
  tnx.del('k2')
  tnx.merge('k3', 'v1')
  tnx.commit()

  t.end()
})


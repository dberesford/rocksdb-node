const os = require('os')
const fs = require('fs')
const test = require('tap').test
const rimraf = require('rimraf')
const rocksdb = require('../build/Release/rocksdb.node')
const path = os.tmpdir() + '/rocksdbBuffersTest'
let db

test('setup', function (t) {
  rimraf.sync(path)
  db = rocksdb.open({create_if_missing: true}, path)
  t.ok(db)
  t.end()
})

test('Basic batch test', function (t) {
  const batch = db.batch()
  batch.put('k1', 'v1')
  batch.del('k1')
  db.write(batch)
  t.equal(db.get('k1'), null)
  t.end()
})

test('Column families batch test', function (t) {
  db.createColumnFamily('foo')
  const batch = db.batch()
  batch.put('foo', 'k1', 'v1')
  batch.del('foo', 'k1')
  db.write(batch)
  t.equal(db.get('foo', 'k1'), null)
  t.end()
})

test('Write opts batch test', function (t) {
  const opts = {
    sync: false
  }
  const batch = db.batch()
  batch.put('k2', 'v2')
  batch.put('k3', 'v3')
  db.write(opts, batch)
  t.equal(db.get('k3'), 'v3')
  t.end()
})

test('teardown', function (t) {
  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))
  t.end()
})

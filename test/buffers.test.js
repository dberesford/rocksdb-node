const tap = require('tap')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')
let db

test('setup', function (t) {
  db = rocksdb({create_if_missing: true}, '/tmp/rocksdbBuffersTest')
  tap.ok(db)
  t.end()
})

test('buffer put/get', function (t) {
  const key = Buffer.from('tést')
  const val = Buffer.from('tést')
  db.put(key, val)
  tap.equal(db.get(key), 'tést')
  t.end()
})

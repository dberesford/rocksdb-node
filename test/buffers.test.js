var fs = require('fs')
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

test('image buffers', function (t) {
  const key = fs.readFileSync('./test/fixtures/beach-thumb.jpg')
  const val = fs.readFileSync('./test/fixtures/beach.jpg')
  db.put(key, val)
  const beach = db.get({buffer: true}, key)
  // Note - this works :-) fs.writeFileSync('/tmp/test-beach.jpg', beach)
  tap.equal(val.length, beach.length)
  t.end()
})

const os = require('os')
const fs = require('fs')
const rimraf = require('rimraf')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')

test('iterator with strings', function (t) {
  rimraf.sync(os.tmpdir() + '/rocksdbIteratorStringTest')
  const db = rocksdb({create_if_missing: true}, os.tmpdir() + '/rocksdbIteratorStringTest')
  t.ok(db)

  db.put('foo', 'bar')
  const it = db.newIterator()
  t.ok(it)

  it.seekToFirst()
  t.ok(it.valid())
  let count = 0

  for (it.seekToFirst(); it.valid(); it.next()) {
    t.equal(it.key(), 'foo')
    t.equal(it.value(), 'bar')
    count++
  }
  t.equal(count, 1)

  t.ok(!it.status())

  for (it.seekToLast(); it.valid(); it.prev()) {
    t.equal(it.key(), 'foo')
    t.equal(it.value(), 'bar')
  }

  t.end()
})

test('iterator with buffers', function (t) {
  rimraf.sync(os.tmpdir() + '/rocksdbIteratorBufferTest')
  const db = rocksdb({create_if_missing: true}, os.tmpdir() + '/rocksdbIteratorBufferTest')
  t.ok(db)

  const key = fs.readFileSync('./test/fixtures/beach-thumb.jpg')
  const val = fs.readFileSync('./test/fixtures/beach.jpg')
  db.put(key, val)

  // also tests ReadOptions are passed ok
  const it = db.newIterator({verify_checksums: true})
  t.ok(it)

  for (it.seekToFirst(); it.valid(); it.next()) {
    const k = it.key({buffer: true})
    const v = it.value({buffer: true})

   // fs.writeFileSync('/tmp/test-beach.jpg', v)
    t.equal(key.length, k.length)
    t.equal(val.length, v.length)
  }

  t.end()
})

test('iterator seek', function (t) {
  rimraf.sync(os.tmpdir() + '/rocksdbIteratorSeekTest')
  const db = rocksdb({create_if_missing: true}, os.tmpdir() + '/rocksdbIteratorSeekTest')
  t.ok(db)

  db.put('1', 'one')
  db.put('2', 'two')
  db.put('3', 'three')
  const it = db.newIterator()
  t.ok(it)

  let count = 0
  for (it.seek('2'); it.valid(); it.next()) {
    // console.log(iter.key(), iter.value())
    count++
  }
  t.equal(count, 2)

  count = 0
  for (it.seek('2'); it.valid() && it.key() < '3'; it.next()) {
    count++
  }
  t.equal(count, 1)

  t.end()
})

test('iterator seek buffers', function (t) {
  rimraf.sync(os.tmpdir() + '/rocksdbIteratorSeekBufferTest')
  const db = rocksdb({create_if_missing: true}, os.tmpdir() + '/rocksdbIteratorSeekBufferTest')
  t.ok(db)

  const a = Buffer.from('A')
  const b = Buffer.from('B')
  const c = Buffer.from('C')
  db.put(a, a)
  db.put(b, b)
  db.put(c, c)

  const it = db.newIterator()
  t.ok(it)

  let count = 0
  for (it.seek({buffer: true}, b); it.valid(); it.next()) {
    count++
  }
  t.equal(count, 2)

  t.end()
})

const os = require('os')
const fs = require('fs')
const rimraf = require('rimraf')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')
const path = os.tmpdir() + '/rocksdbIteratorStringTest'

test('iterator with strings', function (t) {
  rimraf.sync(path)
  const db = rocksdb.open({create_if_missing: true}, path)
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
  db.releaseIterator(it)

  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))

  t.end()
})

test('iterator with buffers', function (t) {
  const path = os.tmpdir() + '/rocksdbIteratorBufferTest'
  rimraf.sync(path)
  const db = rocksdb.open({create_if_missing: true}, path)
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
  db.releaseIterator(it)

  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))

  t.end()
})

test('iterator seek', function (t) {
  const path = os.tmpdir() + '/rocksdbIteratorSeekTest'
  rimraf.sync(path)
  const db = rocksdb.open({create_if_missing: true}, path)
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
  db.releaseIterator(it)

  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))

  t.end()
})

test('iterator seek buffers', function (t) {
  const path = os.tmpdir() + '/rocksdbIteratorSeekBufferTest'
  rimraf.sync(path)
  const db = rocksdb.open({create_if_missing: true}, path)
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
  db.releaseIterator(it)

  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))

  t.end()
})

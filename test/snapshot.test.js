const os = require('os')
const rimraf = require('rimraf')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')

test('setup', function (t) {
  const path = os.tmpdir() + '/rocksdbSnapshotTest'
  rimraf.sync(path)
  const db = rocksdb.open({create_if_missing: true}, path)
  db.put('foo', 'bar')

  const readOptions = {}
  readOptions.snapshot = db.getSnapshot()

  // these should not be part of the iterator
  db.put('foo2', 'bar2')

  const it = db.newIterator(readOptions)

  let count = 0
  for (it.seekToFirst(); it.valid(); it.next()) {
    count++
  }
  t.equal(count, 1)

  db.releaseIterator(it)
  db.releaseSnapshot(readOptions.snapshot)
  db.close()
  t.end()
})

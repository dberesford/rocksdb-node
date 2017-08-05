const os = require('os')
const fs = require('fs')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')

test('setup', function (t) {
  const path = os.tmpdir() + '/rocksdbWithTTLTest'
  const db = rocksdb.openDBWithTTL({create_if_missing: true}, path, 1)
  db.put('foo', 'bar')
  t.equal(db.get('foo'), 'bar')
  // sleep, then foo should be removed (after compact)
  setTimeout(() => {
    db.compactRange()
    t.equal(db.get('foo'), null)

    db.close()
    rocksdb.destroyDB(path)
    t.ok(!fs.existsSync(path))
    t.end()
  }, 2000)
})

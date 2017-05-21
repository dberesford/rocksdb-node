const os = require('os')
const fs = require('fs')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')

test('setup', function (t) {
  const path = os.tmpdir() + '/rocksdbReadOnlyTest'
  const db = rocksdb.open({create_if_missing: true}, path)
  db.put('foo', 'bar')

  const dbRO = rocksdb.open({readOnly: true}, path)
  t.equal(dbRO.get('foo'), 'bar')
  let gotException = false
  try {
    dbRO.put('should not work, db is readOnly', '')
  } catch (e) {
    gotException = true
  }
  t.equal(gotException, true)

  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))
  t.end()
})

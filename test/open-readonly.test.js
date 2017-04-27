const os = require('os')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')

test('setup', function (t) {
  const db = rocksdb({create_if_missing: true}, os.tmpdir() + '/rocksdbReadOnlyTest')
  db.put('foo', 'bar')

  const dbRO = rocksdb({readOnly: true}, os.tmpdir() + '/rocksdbReadOnlyTest')
  t.equal(dbRO.get('foo'), 'bar')
  let gotException = false
  try {
    dbRO.put('should not work, db is readOnly', '')
  } catch (e) {
    gotException = true
  }
  t.equal(gotException, true)
  t.end()
})

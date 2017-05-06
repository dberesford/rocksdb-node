const os = require('os')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')
let db

test('setup', function (t) {
  db = rocksdb({create_if_missing: true}, os.tmpdir() + '/rocksdbColumnsTest')
  t.ok(db)
  t.end()
})

test('list column families', function (t) {
  const families = db.listColumnFamilies()
  t.equal(families.length, 1)
  t.end()
})

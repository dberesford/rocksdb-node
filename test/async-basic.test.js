const tap = require('tap')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')
let db

test('setup', function (t) {
  db = rocksdb({create_if_missing: true}, '/tmp/rocksdbSyncBasicTest')
  tap.ok(db)
  t.end()
})

test('simple async put/get', function (t) {
  db.put('rocks', 'db', function (err) {
    tap.ok(!err)
    db.get('rocks', function (err, val) {
      tap.ok(!err)
      tap.equal(val, 'db')
      t.end()
    })
  })
})

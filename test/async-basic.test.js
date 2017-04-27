const test = require('tap').test
const os = require('os')
const rocksdb = require('../build/Release/rocksdb.node')
let db

test('setup', function (t) {
  db = rocksdb({create_if_missing: true}, os.tmpdir() + '/rocksdbSyncBasicTest')
  t.ok(db)
  t.end()
})

test('simple async put/get/del', function (t) {
  db.put('rocks', 'db', function (err) {
    t.ok(!err)
    db.get('rocks', function (err, val) {
      t.ok(!err)
      t.equal(val, 'db')
      db.del('rocks', function (err) {
        t.ok(!err)
        db.get('rocks', function (err, val) {
          t.ok(!err)
          t.equal(val, null)
          t.end()
        })
      })
    })
  })
})

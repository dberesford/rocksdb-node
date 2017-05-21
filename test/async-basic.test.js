const test = require('tap').test
const os = require('os')
const fs = require('fs')
const rocksdb = require('../build/Release/rocksdb.node')
const path = os.tmpdir() + '/rocksdbSyncBasicTest'
let db

test('setup', function (t) {
  db = rocksdb.open({create_if_missing: true}, path)
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

test('teardown', function (t) {
  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))
  t.end()
})

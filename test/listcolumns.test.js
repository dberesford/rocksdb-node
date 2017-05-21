const os = require('os')
const fs = require('fs')
const rimraf = require('rimraf')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')
const path = os.tmpdir() + '/rocksdbListColumnsTest'
let db

test('list columns test', function (t) {
  rimraf.sync(path)
  db = rocksdb.open({create_if_missing: true}, path)
  t.ok(db)

  let families = rocksdb.listColumnFamilies(path)
  t.equal(families.length, 1)

  // create a column, check it gets picked up
  db.createColumnFamily('foo')

  // also sanity check passing open options to listColumnFamilies
  families = rocksdb.listColumnFamilies({paranoid_checks: true}, path)
  t.equal(families.length, 2)

  t.end()
})

test('list columns with bad path', function (t) {
  let gotException = false
  try {
    rocksdb.listColumnFamilies('idontexist')
  } catch (e) {
    gotException = true
  }
  t.ok(gotException)
  t.end()
})

test('teardown', function (t) {
  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))
  t.end()
})

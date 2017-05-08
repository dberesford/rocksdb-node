const os = require('os')
const rimraf = require('rimraf')
const test = require('tap').test
const rocks = require('../build/Release/rocksdb.node')
const path = os.tmpdir() + '/rocksdbListColumnsTest'

test('list columns test', function (t) {
  rimraf.sync(path)
  const db = rocks.open({create_if_missing: true}, path)
  t.ok(db)

  let families = rocks.listColumnFamilies(path)
  t.equal(families.length, 1)

  // create a column, check it gets picked up
  db.createColumnFamily('foo')

  // also sanity check passing open options to listColumnFamilies
  families = rocks.listColumnFamilies({paranoid_checks: true}, path)
  t.equal(families.length, 2)

  t.end()
})

test('list columns with bad path', function (t) {
  let gotException = false
  try {
    rocks.listColumnFamilies('idontexist')
  } catch (e) {
    gotException = true
  }
  t.ok(gotException)
  t.end()
})

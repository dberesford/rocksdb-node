const os = require('os')
const fs = require('fs')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')

test('multiget sync', function (t) {
  const path = os.tmpdir() + '/rocksdbMultiGetTestSync'
  const db = rocksdb.open({create_if_missing: true}, path)
  db.put('foo1', 'bar1')
  db.put('foo2', 'bar2')
  db.put('foo3', 'bar3')

  const vals = db.multiGet(['foo1', 'foo2', 'foo3', 'idontexist'])
  t.equal(vals[0], 'bar1')
  t.equal(vals[1], 'bar2')
  t.equal(vals[2], 'bar3')
  t.equal(vals[3], null)

  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))
  t.end()
})

test('multiget async', function (t) {
  const path = os.tmpdir() + '/rocksdbMultiGetTestASync'
  const db = rocksdb.open({create_if_missing: true}, path)
  db.put('foo1', 'bar1')
  db.put('foo2', 'bar2')
  db.put('foo3', 'bar3')

  db.multiGet(['foo1', 'foo2', 'foo3', 'idontexist'], function (err, vals) {
    t.equal(vals[0], 'bar1')
    t.equal(vals[1], 'bar2')
    t.equal(vals[2], 'bar3')
    t.equal(vals[3], null)

    db.close()
    rocksdb.destroyDB(path)
    t.ok(!fs.existsSync(path))
    t.end()
  })
})

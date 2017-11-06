const os = require('os')
const fs = require('fs')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')

test('multiget sync', function (t) {
  const path = os.tmpdir() + '/rocksdbMultiGetTestSync'
  rocksdb.destroyDB(path)
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
  rocksdb.destroyDB(path)
  const db = rocksdb.open({create_if_missing: true}, path)
  db.put('foo1', 'bar1')
  db.put('foo2', 'bar2')
  db.put('foo3', 'bar3')

  db.multiGet(['foo1', 'foo2', 'foo3', 'idontexist'], function (err, vals) {
    t.ok(!err)
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

test('multiget column families', function (t) {
  const path = os.tmpdir() + '/rocksdbMultiGetTestColumnFamilies'
  rocksdb.destroyDB(path)
  const db = rocksdb.open({create_if_missing: true}, path)
  db.createColumnFamily('col1')

  db.put('col1', 'foo1', 'bar1')
  db.put('col1', 'foo2', 'bar2')
  db.put('col1', 'foo3', 'bar3')

  const vals = db.multiGet('col1', ['foo1', 'foo2', 'foo3', 'idontexist'])
  t.equal(vals[0], 'bar1')
  t.equal(vals[1], 'bar2')
  t.equal(vals[2], 'bar3')
  t.equal(vals[3], null)

  db.multiGet('col1', ['foo1', 'foo2', 'foo3', 'idontexist'], function (err, vals) {
    t.ok(!err)
    t.equal(vals[0], 'bar1')
    t.equal(vals[3], null)

    db.close()
    rocksdb.destroyDB(path)
    t.ok(!fs.existsSync(path))
    t.end()
  })
})

test('multiget buffers', {timeout: 50000}, function (t) {
  const path = os.tmpdir() + '/rocksdbMultiGetBuffers'
  rocksdb.destroyDB(path)
  const db = rocksdb.open({create_if_missing: true}, path)
  const key = fs.readFileSync('./test/fixtures/beach-thumb.jpg')
  const val = fs.readFileSync('./test/fixtures/beach.jpg')

  db.put(key, val)

  const vals = db.multiGet({buffer: true}, [key])
  t.equal(vals[0].length, val.length)

  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))
  t.end()
})

test('multiget buffers async', {timeout: 50000}, function (t) {
  const path = os.tmpdir() + '/rocksdbMultiGetBuffersAsync'
  rocksdb.destroyDB(path)
  const db = rocksdb.open({create_if_missing: true}, path)
  const key = fs.readFileSync('./test/fixtures/beach-thumb.jpg')
  const val = fs.readFileSync('./test/fixtures/beach.jpg')

  db.put(key, val)

  db.multiGet({buffer: true}, [key], (err, vals) => {
    t.ok(!err)
    t.equal(vals[0].length, val.length)

    db.close()
    rocksdb.destroyDB(path)
    t.ok(!fs.existsSync(path))
    t.end()
  })
})

test('multiget test all args', function (t) {
  const path = os.tmpdir() + '/rocksdbMultiAllArgs'
  rocksdb.destroyDB(path)
  const db = rocksdb.open({create_if_missing: true}, path)
  db.createColumnFamily('col1')

  db.put('col1', 'foo1', 'bar1')
  db.put('col1', 'foo2', 'bar2')
  db.put('col1', 'foo3', 'bar3')

  db.multiGet({buffer: false}, 'col1', ['foo1', 'foo2', 'foo3', 'idontexist'], function (err, vals) {
    t.ok(!err)
    t.equal(vals[0], 'bar1')
    t.equal(vals[3], null)

    db.close()
    rocksdb.destroyDB(path)
    t.ok(!fs.existsSync(path))
    t.end()
  })
})

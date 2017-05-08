const os = require('os')
const rimraf = require('rimraf')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')
const path = os.tmpdir() + '/rocksdbColumnsTest'
let db

test('setup', function (t) {
  rimraf.sync(path)
  db = rocksdb.open({create_if_missing: true}, path)
  t.ok(db)
  t.end()
})

test('get column families', function (t) {
  const families = db.getColumnFamilies()
  t.equal(families.length, 1)
  t.end()
})

test('create column family', function (t) {
  db.createColumnFamily('foo')
  t.equal(db.getColumnFamilies().length, 2)
  t.end()
})

test('put, get, del, drop column family', function (t) {
  db.createColumnFamily('col1')
  db.put('col1', 'hello', 'world')

  t.equal(db.get('col1', 'hello'), 'world')
  t.equal(db.get('hello'), null)

  db.del('col1', 'hello')
  t.equal(db.get('col1', 'hello'), null)

  const numCols = db.getColumnFamilies().length
  db.dropColumnFamily('col1')
  t.equal(db.getColumnFamilies().length, numCols - 1)
  t.end()
})

test('put with invalid column sync', function (t) {
  let gotException = false
  try {
    db.put('non-existent-column', 'k', 'v')
  } catch (ex) {
    gotException = true
  }
  t.ok(gotException)
  t.end()
})

test('drop with invalid column', function (t) {
  let gotException = false
  try {
    db.dropColumnFamily('non-existent-column')
  } catch (ex) {
    gotException = true
  }
  t.ok(gotException)
  t.end()
})

test('column family async get/put/del', function (t) {
  db.createColumnFamily('col2')
  db.put('col2', 'h', 'w', function (err) {
    t.ok(!err)
    db.get('col2', 'h', function (err, val) {
      t.ok(!err)
      t.equal(val, 'w')
      db.del('col2', 'h', function (err) {
        t.ok(!err)
        t.equal(db.get('col2', 'h'), null)
        t.end()
      })
    })
  })
})

test('invalid columns async', function (t) {
  db.put('non-existent-column', 'k', 'v', function (err) {
    t.ok(err)
    db.get('non-existent-column', 'k', function (err) {
      t.ok(err)
      t.end()
    })
  })
})

test('open readonly', function (t) {
  const db2 = rocksdb.open({readOnly: true}, path)
  t.ok(db2)
  t.ok(db2.getColumnFamilies())
  t.end()
})

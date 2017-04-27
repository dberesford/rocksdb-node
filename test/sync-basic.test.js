const os = require('os')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')
let db

test('setup', function (t) {
  db = rocksdb({create_if_missing: true}, os.tmpdir() + '/rocksdbBasicTest')
  t.ok(db)
  t.end()
})

test('simple sync put/get/delete', function (t) {
  db.put('rocks', 'db')
  t.equal(db.get('rocks'), 'db')
  db.del('rocks')
  t.equal(db.get('rocks'), null)
  t.end()
})

test('get non-existent key', function (t) {
  t.notOk(db.get('nokey'))
  t.end()
})

test('delete non-existent key', function (t) {
  t.notOk(db.del('nokey'))
  t.end()
})

// TODO - this is failing with a weird crash..
test('wrong args constructor', function (t) {
  t.throws(function () {
    rocksdb()
  }, /Wrong number of arguments/)

  t.end()
})

test('wrong args put', function (t) {
  t.throws(function () {
    db.put()
  }, /Wrong number of arguments/)
  t.end()
})

test('wrong args get', function (t) {
  t.throws(function () {
    db.get()
  }, /Wrong number of arguments/)
  t.end()
})

// var assert = require('assert')
var tap = require('tap')
var test = require('tap').test

var rocksdb = require('../build/Release/rocksdb.node')
var db = rocksdb.open({create_if_missing: true}, '/tmp/rockme')
tap.ok(db)

test('exception thrown for wrong args', function (t) {
  t.throws(function () {
    rocksdb.open()
  }, /Wrong number of arguments/)

  t.end()
})

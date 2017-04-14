var tap = require('tap')

var rocksdb = require('../build/Release/rocksdb.node')
var db = rocksdb.open('/tmp/rockme')
tap.ok(db)

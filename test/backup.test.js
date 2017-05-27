const os = require('os')
const fs = require('fs')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')
const path = os.tmpdir() + '/rocksdbBackupTest'
const db

test('setup', function (t) {
  db = rocksdb.open({create_if_missing: true}, path)
  db.put('foo', 'bar')

  
  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))
  t.end()
})

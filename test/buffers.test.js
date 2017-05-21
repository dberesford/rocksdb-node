var fs = require('fs')
const os = require('os')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')
const path = os.tmpdir() + '/rocksdbBuffersTest'
let db

test('setup', function (t) {
  db = rocksdb.open({create_if_missing: true}, path)
  t.ok(db)
  t.end()
})

test('buffer put/get', function (t) {
  const key = Buffer.from('tést')
  const val = Buffer.from('tést')
  db.put(key, val)
  t.equal(db.get(key), 'tést')
  t.end()
})

test('image buffers sync', function (t) {
  const key = fs.readFileSync('./test/fixtures/beach-thumb.jpg')
  const val = fs.readFileSync('./test/fixtures/beach.jpg')
  db.put(key, val)
  const beach = db.get({buffer: true}, key)
  // Note - this works :-) fs.writeFileSync('/tmp/test-beach.jpg', beach)
  t.equal(val.length, beach.length)
  t.end()
})

test('image buffers async', function (t) {
  const key = fs.readFileSync('./test/fixtures/beach-thumb.jpg')
  const val = fs.readFileSync('./test/fixtures/beach.jpg')
  db.put(key, val, function (err) {
    t.ok(!err)
    db.get({buffer: true}, key, function (err, beach) {
      t.ok(!err)
      t.equal(val.length, beach.length)
      db.del(key, function (err) {
        t.ok(!err)
        t.end()
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

const os = require('os')
const fs = require('fs')
const test = require('tap').test
const rimraf = require('rimraf')
const rocksdb = require('../build/Release/rocksdb.node')

test('SstFileWriter test', function (t) {
  const path = os.tmpdir() + '/rocksdbSSTFileTest'
  const file = os.tmpdir() + '/import1'
  rimraf.sync(path)
  rimraf.sync(file)
  const db = rocksdb.open({create_if_missing: true}, path)

  const sstFileWriter = db.getSstFileWriter()
  sstFileWriter.open(file)
  sstFileWriter.add('a', 'b')
  t.equal(sstFileWriter.fileSize(), 0)
  sstFileWriter.finish()

  db.ingestExternalFile(file)
  t.equal(db.get('a'), 'b')

  db.close()
  rocksdb.destroyDB(path)
  t.ok(!fs.existsSync(path))

  t.end()
})

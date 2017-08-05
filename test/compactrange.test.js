const os = require('os')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')

test('compact range sync', function (t) {
  const path = os.tmpdir() + '/rocksdbCompactRangeSyncTest'
  const db = rocksdb.open({create_if_missing: true}, path)

  db.put('foo1', 'bar1')
  db.put('foo2', 'bar2')
  db.put('foo3', 'bar3')

  db.compactRange()
  db.compactRange('foo1', 'foo3')

  const opts = {
    exclusive_manual_compaction: true,
    change_level: false,
    target_level: -1,
    target_path_id: 0
  }

  db.compactRange(opts)
  db.compactRange(opts, 'foo1', 'foo2')

  db.close()
  rocksdb.destroyDB(path)
  t.end()
})

test('compact range col families', function (t) {
  const path = os.tmpdir() + '/rocksdbCompactRangeColFamiliesTest'
  const db = rocksdb.open({create_if_missing: true}, path)
  db.createColumnFamily('col1')
  db.put('col1', 'foo1', 'bar1')
  db.put('col1', 'foo2', 'bar2')
  db.put('col1', 'foo3', 'bar3')

  db.compactRange('col1')
  db.compactRange('col1', 'foo1', 'foo3')

  const opts = {
    exclusive_manual_compaction: true
  }

  db.compactRange(opts, 'col1')
  db.compactRange(opts, 'col1', 'foo1', 'foo2')

  db.close()
  rocksdb.destroyDB(path)
  t.end()
})

// tests async and a myriad of parameter combinations
test('compact range async', function (t) {
  const path = os.tmpdir() + '/rocksdbCompactRangeAsyncTest'
  const db = rocksdb.open({create_if_missing: true}, path)

  db.put('foo1', 'bar1')
  db.put('foo2', 'bar2')
  db.put('foo3', 'bar3')
  db.createColumnFamily('col1')
  db.put('col1', 'foo1', 'bar1')
  db.put('col1', 'foo2', 'bar2')
  db.put('col1', 'foo3', 'bar3')

  db.compactRange((err) => {
    t.ok(!err)

    db.compactRange('foo1', 'foo3', (err) => {
      t.ok(!err)

      const opts = {
        exclusive_manual_compaction: true
      }

      db.compactRange(opts, (err) => {
        t.ok(!err)

        db.compactRange(opts, 'foo1', 'foo2', (err) => {
          t.ok(!err)

          db.compactRange(opts, 'col1', 'foo1', 'foo2', (err) => {
            t.ok(!err)

            db.close()
            rocksdb.destroyDB(path)
            t.end()
          })
        })
      })
    })
  })
})

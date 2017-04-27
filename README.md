# RocksDB

Node native binding for RocksDB. Aims to be a full nodejs mapping for https://github.com/facebook/rocksdb/wiki.

## Installation

`npm i rocksdb-node --save`

## Usage

RockDB-node has both a synchronous and asynchronous api. 

Sync:

```
const rocksdb = require('rocksdb-node')
const db = rocksdb({create_if_missing: true}, '/tmp/my-rocks-database')
db.put('node', 'rocks')
const value = db.get('node')
db.del('node')

// both keys and vals as buffers
const key = fs.readFileSync('./test/fixtures/beach-thumb.jpg')
const val = fs.readFileSync('./test/fixtures/beach.jpg')
db.put(key, val)
const beach = db.get({buffer: true}, key)
fs.writeFileSync('/tmp/beach2.jpg', beach)
```

Async: 

```
const rocksdb = require('rocksdb-node')
const db = rocksdb({create_if_missing: true}, '/tmp/my-rocks-database')
db.put('node', 'rocks', function(err) {
  if (err) return console.error(err);
  db.get('node', function (err, val) {
    if (err) return console.error(err);
    console.log(val)
    db.del('node', function(err){
      if (err) return console.error(err);      
    })
  })
})

```

## Options

All RocksDB options are mostly supported for Open, Write and Read options, as defined in [options.h](https://github.com/facebook/rocksdb/blob/5.2.fb/include/rocksdb/options.h). 

See [options.test.js](./test/options.test.js) for the definitive list.

Examples:
```
const db = rocksdb({error_if_exists: true}}, '/tmp/my-rocks-database')

db.put({sync: false}, 'foo', 'bar')
const val = db.get({verify_checksums: false}, 'foo')
```

## RocksDB Version

Developed against RocksDB 5.2.

## Rough TODO List

* iterators
* support for atomic updates (batch)
* support for snapshots
* support for js comparators
* support for cache
* support for filters
* support js environment support (rocksdb::env)
* full support for rocks specific api https://github.com/facebook/rocksdb/wiki/Features-Not-in-LevelDB


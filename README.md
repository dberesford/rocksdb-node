# RocksDB

Node native binding for RocksDB. Aims to be a full nodejs mapping for https://github.com/facebook/rocksdb/wiki.

## Installation

`npm i rocksdb-node --save`

## Usage

Sync:

```
const rocksdb = require('rocksdb-node')
const db = rocksdb({create_if_missing: true}, '/tmp/my-rocks-database')
db.put('node', 'rocks')
const value = db.get('node')
```

## Rough TODO List

* Slice & buffer support in put/get
* async usage
* more nan, less direct v8
* iterators
* make option/config passing dynamic
* support for ReadOptions and WriteOptions
* support for atomic updates (batch)
* support for snapshots
* support for js comparators
* support for cache
* support for filters
* support js environment support (rocksdb::env)
* full support for rocks specific api https://github.com/facebook/rocksdb/wiki/Features-Not-in-LevelDB


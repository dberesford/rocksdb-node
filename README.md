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

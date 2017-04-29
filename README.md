# RocksDB

Node native binding for RocksDB. Aims to be a full nodejs mapping for https://github.com/facebook/rocksdb/wiki. Developed against RocksDB 5.2.

## Installation

`npm i rocksdb-node --save`

## Sample Usage

Sync examples:

```javascript
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

Async examples: 

```javascript
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

## API

### Open

```javascript
const rocksdb = require('rocksdb-node')
const db = rocksdb({create_if_missing: true}, '/tmp/my-rocks-database')
```

Note that passing an options object is required, even if it's empty.

All boolean and int options as defined in [DBOptions](https://github.com/facebook/rocksdb/blob/5.2.fb/include/rocksdb/options.h#L848) are supported, e.g.

```javascript
const opts = {
    create_if_missing: true,
    error_if_exists: false,
    create_missing_column_families: false,
    paranoid_checks: true,
    use_fsync: false,
    allow_mmap_reads: false,
    allow_mmap_writes: false,
    use_direct_reads: false,
    allow_fallocate: true,
    is_fd_close_on_exec: true,
    advise_random_on_open: true,
    new_table_reader_for_compaction_inputs: true,
    use_adaptive_mutex: false,
    enable_thread_tracking: false,
    allow_concurrent_memtable_write: true,
    enable_write_thread_adaptive_yield: true,
    skip_stats_update_on_db_open: false,
    allow_2pc: false,
    fail_if_options_file_error: false,
    dump_malloc_stats: false,
    avoid_flush_during_recovery: false,
    avoid_flush_during_shutdown: false,

    max_open_files: -1,
    max_file_opening_threads: 16,
    max_total_wal_size: 0,
    base_background_compactions: 1,
    max_background_compactions: 1,
    max_subcompactions: 1,
    max_background_flushes: 1,
    max_log_file_size: 0,
    log_file_time_to_roll: 0,
    keep_log_file_num: 1000,
    recycle_log_file_num: 0,
    table_cache_numshardbits: 0,
    WAL_ttl_seconds: 0,
    WAL_size_limit_MB: 0,
    manifest_preallocation_size: 4 * 1024 * 1024,
    db_write_buffer_size: 0,
    compaction_readahead_size: 0,
    random_access_max_buffer_size: 1024 * 1024,
    writable_file_max_buffer_size: 1024 * 1024,
    bytes_per_sync: 0,
    wal_bytes_per_sync: 0,
    write_thread_max_yield_usec: 100,
    write_thread_slow_yield_usec: 3
  }
  ```

### Open ReadOnly

Open database for read only. All DB interfaces that modify data, like put/delete, will return error.

```javascript
const dbRO = rocksdb({readOnly: true}, './myrocks') // myrocks must already exist
```

### Put

`db.put` has both a synchronous and asynchronous interface, e.g. 

```javascript
try {
  db.put('foo', 'bar')
} catch(e)...
```
 or 

 ```javascript
db.put('foo', 'bar', function(err){...})
```

The key and value params for `put` can both be either a string or a buffer.

```javascript
const key = fs.readFileSync('./test/fixtures/beach-thumb.jpg')
const val = fs.readFileSync('./test/fixtures/beach.jpg')
db.put(key, val)
// or
db.put('foo', val)
// or
db.put(key, 'bar')
```

`put` optionally takes an options object as it's first parameter. All boolean and int options as defined in [WriteOptions](https://github.com/facebook/rocksdb/blob/5.2.fb/include/rocksdb/options.h#L1565) are supported, e.g. 

```javascript
const writeOpts = {
  sync: false,
  disableWAL: false,
  ignore_missing_column_families: false,
  no_slowdown: false
}
db.put(writeOpts, 'foo', 'bar')
```

### Get

`db.get` has both a synchronous and asynchronous interface, e.g. 

```javascript
try {
  const value = db.get('foo')
} catch(e)...
```
 or 

 ```javascript
db.get('foo', function(err, value){...})
```

The key param for `get` can both be either a string or a buffer, and if the value you are expecting to get is a buffer, you must pass the option `buffer:true`, e.g. 

```javascript
const key = fs.readFileSync('./test/fixtures/beach-thumb.jpg')
const valueBuffer = db.get({buffer:true}, key)
```

`get` optionally takes an options object as it's first parameter. All boolean and int options as defined in [ReadOptions](https://github.com/facebook/rocksdb/blob/5.2.fb/include/rocksdb/options.h#L1444) are supported, e.g. 

```javascript
const readOpts = {
  verify_checksums: true,
  fill_cache: true,
  tailing: false,
  managed: false,
  total_order_seek: false,
  prefix_same_as_start: false,
  pin_data: false,
  background_purge_on_iterator_cleanup: false,
  readahead_size: 0,
  ignore_range_deletions: false
}
const bar = db.get(readOpts, 'foo')
```

### Delete

`db.del` has both a synchronous and asynchronous interface, e.g. 

```javascript
try {
  db.del('foo')
} catch(e)...
```
 or 

 ```javascript
db.del('foo', function(err){...})
```

The key param for `del` can both be either a string or a buffer, e.g. 

```javascript
const key = fs.readFileSync('./test/fixtures/beach-thumb.jpg')
db.del(key)
```

`del` optionally takes an options object as it's first parameter, it also takes a [WriteOptions](https://github.com/facebook/rocksdb/blob/5.2.fb/include/rocksdb/options.h#L1565) (like `put`), e.g.

```javascript
const writeOpts = {
  sync: false,
  disableWAL: false,
  ignore_missing_column_families: false,
  no_slowdown: false
}
db.del(writeOpts, 'foo')
```

### Iteratation

The Iterator API matches the Rocks [Iterator API](https://github.com/facebook/rocksdb/wiki/Basic-Operations#iteration). Note the Iterator API is synchronous.

```javascript
const it = db.newIterator()
for (it.seekToFirst(); it.valid(); it.next()) {
  console.log(it.key(), it.value())
}
const err = it.status()
if (err) throw err;
```

The following Rocks [Iterator API](https://github.com/facebook/rocksdb/blob/master/include/rocksdb/iterator.h#L29) is supported (documentation has been copied from there):

#### iterator.newIterator()

Creates a new Iterator for the current database. Optionally takes [ReadOptions](https://github.com/facebook/rocksdb/blob/5.2.fb/include/rocksdb/options.h#L1444), e.g.

```javascript
const readOpts = {
  verify_checksums: true
}
const iter = db.newIterator(readOpts)
```

#### iterator.valid()

An iterator is either positioned at a key/value pair, or not valid. This method returns true iff the iterator is valid.

#### iterator.seekToFirst()

Position at the first key in the source. The iterator is Valid() after this call iff the source is not empty.

#### iterator.seekToLast()

Position at the last key in the source. The iterator is Valid() after this call iff the source is not empty.

#### iterator.seek()

Position at the first key in the source that at or past target. The iterator is Valid() after this call iff the source contains an entry that comes at or past target.

```javascript
db.put('1', 'one')
db.put('2', 'two')
db.put('3', 'three')

const it = db.newIterator()
for (it.seek('2'); it.valid(); it.next()) {
  console.log(iter.key(), iter.value())    
}
```

#### iterator.seekForPrev()

Position at the last key in the source that at or before target. The iterator is Valid() after this call iff the source contains an entry that comes at or before target.

#### iterator.next()

Moves to the next entry in the source.  After this call, Valid() is true iff the iterator was not positioned at the last entry in the source.

#### iterator.prev()

Moves to the previous entry in the source.  After this call, Valid() is true iff the iterator was not positioned at the first entry in source.

#### iterator.key() 

Return the key for the current entry.  The underlying storage for the returned slice is valid only until the next modification of the iterator.

Note if the key is a buffer, you need to pass the `buffer:true` flag

```javascript
for (it.seekToFirst(); it.valid(); it.next()) {
  const k = it.key({buffer: true})
  const v = it.value({buffer: true})
...
```

#### iterator.value() 

Return the value for the current entry.  The underlying storage for the returned slice is valid only until the next modification of the iterator.

As with `iterator.key` if the value is a buffer, you need to pass the `buffer:true` flag

#### iterator.status()

If an error has occurred, return it. A javascript Error object is returned if an error occurred, otherwise null.

```javascript
const err = it.status()
if (err) throw err;
```

## Rough TODO List

* support for atomic updates (batch)
* support for snapshots
* support for js comparators
* support for cache
* support for filters
* support js environment support (rocksdb::env)
* full support for rocks specific api https://github.com/facebook/rocksdb/wiki/Features-Not-in-LevelDB


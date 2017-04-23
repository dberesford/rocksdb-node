const tap = require('tap')
const test = require('tap').test
const rocksdb = require('../build/Release/rocksdb.node')

test('options test', function (t) {
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
    avoid_flush_during_shutdown: false

  }
  const db = rocksdb(opts, '/tmp/rocksdbOptsTest')
  tap.ok(db)
  t.end()
})

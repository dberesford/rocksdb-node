const os = require('os')
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

  const db = rocksdb.open(opts, os.tmpdir() + '/rocksdbOptsTest')
  t.ok(db)

  const writeOpts = {
    sync: false,
    disableWAL: false,
    ignore_missing_column_families: false,
    no_slowdown: false
  }
  db.put(writeOpts, 'foo', 'bar')

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
  t.equals(db.get(readOpts, 'foo'), 'bar')
  t.end()
})

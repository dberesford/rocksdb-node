#include <nan.h>
#include "OptionsHelper.h"
#include "Snapshot.h"
#include "rocksdb/db.h"

// Mirror the rocksdb options here - https://github.com/facebook/rocksdb/blob/5.2.fb/include/rocksdb/options.h
void OptionsHelper::ProcessOpenOptions (v8::Local<v8::Object> opts, rocksdb::Options *options) {
  BOOLEAN_OPTION(create_if_missing, opts, options)
  BOOLEAN_OPTION(error_if_exists, opts, options)
  BOOLEAN_OPTION(create_missing_column_families, opts, options)
  BOOLEAN_OPTION(paranoid_checks, opts, options)
  BOOLEAN_OPTION(use_fsync, opts, options)
  BOOLEAN_OPTION(allow_mmap_reads, opts, options)
  BOOLEAN_OPTION(allow_mmap_writes, opts, options)
  BOOLEAN_OPTION(use_direct_reads, opts, options)

  BOOLEAN_OPTION(allow_fallocate, opts, options)
  BOOLEAN_OPTION(is_fd_close_on_exec, opts, options)
  BOOLEAN_OPTION(advise_random_on_open, opts, options)
  BOOLEAN_OPTION(new_table_reader_for_compaction_inputs, opts, options)

  BOOLEAN_OPTION(use_adaptive_mutex, opts, options)
  BOOLEAN_OPTION(enable_thread_tracking, opts, options)
  BOOLEAN_OPTION(allow_concurrent_memtable_write, opts, options)
  BOOLEAN_OPTION(enable_write_thread_adaptive_yield, opts, options)
  BOOLEAN_OPTION(skip_stats_update_on_db_open, opts, options)
  BOOLEAN_OPTION(allow_2pc, opts, options)
  BOOLEAN_OPTION(fail_if_options_file_error, opts, options)

  BOOLEAN_OPTION(dump_malloc_stats, opts, options)
  BOOLEAN_OPTION(avoid_flush_during_recovery, opts, options)
  BOOLEAN_OPTION(avoid_flush_during_shutdown, opts, options)

  INT_OPTION(max_open_files, opts, options)
  INT_OPTION(max_file_opening_threads, opts, options)
  INT_OPTION(max_total_wal_size, opts, options)
  INT_OPTION(base_background_compactions, opts, options)
  INT_OPTION(max_background_compactions, opts, options)
  INT_OPTION(max_subcompactions, opts, options)
  INT_OPTION(max_background_flushes, opts, options)
  INT_OPTION(max_log_file_size, opts, options)
  INT_OPTION(log_file_time_to_roll, opts, options)
  INT_OPTION(keep_log_file_num, opts, options)
  INT_OPTION(recycle_log_file_num, opts, options)
  INT_OPTION(table_cache_numshardbits, opts, options)
  INT_OPTION(WAL_ttl_seconds, opts, options)
  INT_OPTION(WAL_size_limit_MB, opts, options)
  INT_OPTION(manifest_preallocation_size, opts, options)
  INT_OPTION(db_write_buffer_size, opts, options)
  INT_OPTION(compaction_readahead_size, opts, options)
  INT_OPTION(random_access_max_buffer_size, opts, options)
  INT_OPTION(writable_file_max_buffer_size, opts, options)
  INT_OPTION(bytes_per_sync, opts, options)
  INT_OPTION(wal_bytes_per_sync, opts, options)
  INT_OPTION(write_thread_max_yield_usec, opts, options)
  INT_OPTION(write_thread_slow_yield_usec, opts, options)
}

void OptionsHelper::ProcessWriteOptions (v8::Local<v8::Object> opts, rocksdb::WriteOptions *options) {
  BOOLEAN_OPTION(sync, opts, options)
  BOOLEAN_OPTION(disableWAL, opts, options)
  BOOLEAN_OPTION(ignore_missing_column_families, opts, options)
  BOOLEAN_OPTION(no_slowdown, opts, options)
}

void OptionsHelper::ProcessReadOptions (v8::Local<v8::Object> opts, rocksdb::ReadOptions *options) {
  BOOLEAN_OPTION(verify_checksums, opts, options)
  BOOLEAN_OPTION(fill_cache, opts, options)
  BOOLEAN_OPTION(tailing, opts, options)
  BOOLEAN_OPTION(managed, opts, options)
  BOOLEAN_OPTION(total_order_seek, opts, options)
  BOOLEAN_OPTION(prefix_same_as_start, opts, options)
  BOOLEAN_OPTION(pin_data, opts, options)
  BOOLEAN_OPTION(background_purge_on_iterator_cleanup, opts, options)
  INT_OPTION(readahead_size, opts, options)
  BOOLEAN_OPTION(ignore_range_deletions, opts, options)

  v8::Local<v8::String> snapshot = Nan::New("snapshot").ToLocalChecked();
  if (opts->Has(snapshot)) {
    Snapshot* ss = Nan::ObjectWrap::Unwrap<Snapshot>(opts->Get(snapshot).As<v8::Object>());
    options->snapshot = ss->_snapshot;
  }
}

// See https://github.com/facebook/rocksdb/blob/3c327ac2d0fd50bbd82fe1f1af5de909dad769e6/include/rocksdb/options.h#L1152
void OptionsHelper::ProcessCompactRangeOptions (v8::Local<v8::Object> opts, rocksdb::CompactRangeOptions *options) {
  BOOLEAN_OPTION(exclusive_manual_compaction, opts, options);
  BOOLEAN_OPTION(change_level, opts, options);
  INT_OPTION(target_level, opts, options);
  INT_OPTION(target_path_id, opts, options);
}

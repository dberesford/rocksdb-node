#ifndef CompactRangeWorker_H
#define CompactRangeWorker_H

#include <nan.h>
#include "rocksdb/db.h"

class CompactRangeWorker : public Nan::AsyncWorker {
 public:
  CompactRangeWorker(Nan::Callback *callback, rocksdb::DB *db, rocksdb::CompactRangeOptions options, rocksdb::ColumnFamilyHandle *family, v8::Local<v8::Object> &fromObj, v8::Local<v8::Object> &toObj);
  ~CompactRangeWorker();
  virtual void Execute();
  virtual void HandleOKCallback ();

 private:
  rocksdb::DB *_db;
  rocksdb::Slice* _from;
  rocksdb::Slice* _to;
  rocksdb::Status _status;
  rocksdb::CompactRangeOptions _options;
  rocksdb::ColumnFamilyHandle *_family;
};

#endif  // CompactRangeWorker_H

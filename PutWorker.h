#ifndef PutWorker_H
#define PutWorker_H

#include <nan.h>
#include "rocksdb/db.h"

class PutWorker : public Nan::AsyncWorker {
 public:
  PutWorker(Nan::Callback *callback, rocksdb::DB *db, rocksdb::Slice key, rocksdb::Slice value, rocksdb::WriteOptions options);
  ~PutWorker();
  virtual void Execute();
  virtual void HandleOKCallback ();

 private:
  rocksdb::DB *_db;
  rocksdb::Slice _key;
  rocksdb::Slice _value;
  rocksdb::Status _status;
  rocksdb::WriteOptions _options;
};

#endif  // PutWorker_H
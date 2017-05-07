#ifndef DeleteWorker_H
#define DeleteWorker_H

#include <nan.h>
#include "rocksdb/db.h"

class DeleteWorker : public Nan::AsyncWorker {
 public:
  DeleteWorker(Nan::Callback *callback, rocksdb::DB *db, rocksdb::ColumnFamilyHandle *family, rocksdb::Slice key, rocksdb::WriteOptions options);
  ~DeleteWorker();
  virtual void Execute();
  virtual void HandleOKCallback ();

 private:
  rocksdb::DB *_db;
  rocksdb::ColumnFamilyHandle *_family;
  rocksdb::Slice _key;
  rocksdb::Status _status;
  rocksdb::WriteOptions _options;
};

#endif  // DeleteWorker_H
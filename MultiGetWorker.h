#ifndef MultiGetWorker_H
#define MultiGetWorker_H

#include <nan.h>
#include "rocksdb/db.h"

class MultiGetWorker : public Nan::AsyncWorker {
 public:
  MultiGetWorker(Nan::Callback *callback, rocksdb::DB *db, rocksdb::ReadOptions options, v8::Local<v8::Array> &keysArray);
  ~MultiGetWorker();
  virtual void Execute();
  virtual void HandleOKCallback ();

 private:
  rocksdb::DB *_db;
  std::vector<rocksdb::Slice> _keys;
  std::vector<rocksdb::Status> _statuss;
  std::vector<std::string> _values;
  rocksdb::ReadOptions _options;
  v8::Local<v8::Array> _keysArray;
};

#endif  // MultiGetWorker_H
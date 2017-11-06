#ifndef MultiGetWorker_H
#define MultiGetWorker_H

#include <nan.h>
#include "rocksdb/db.h"

class MultiGetWorker : public Nan::AsyncWorker {
 public:
  MultiGetWorker(Nan::Callback *callback, rocksdb::DB *db, bool buffer, rocksdb::ReadOptions options, rocksdb::ColumnFamilyHandle *family, v8::Local<v8::Array> &keysArray);
  ~MultiGetWorker();
  virtual void Execute();
  virtual void HandleOKCallback ();

 private:
  rocksdb::DB *_db;
  bool _buffer;
  std::vector<rocksdb::Slice> _keys;
  std::vector<rocksdb::Status> _statuss;
  std::vector<std::string> _values;
  rocksdb::ReadOptions _options;
  std::vector<rocksdb::ColumnFamilyHandle*> _families;
  v8::Local<v8::Array> _keysArray;
};

#endif  // MultiGetWorker_H

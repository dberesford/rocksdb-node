#ifndef GetWorker_H
#define GetWorker_H

#include <nan.h>
#include "rocksdb/db.h"

class GetWorker : public Nan::AsyncWorker {
 public:
  GetWorker(Nan::Callback *callback, rocksdb::DB *db, bool isBuffer, rocksdb::ReadOptions options, v8::Local<v8::Object> &keyObj);
  ~GetWorker();
  virtual void Execute();
  virtual void HandleOKCallback ();

 private:
  rocksdb::DB *_db;
  rocksdb::Slice _key;
  std::string _value;
  rocksdb::Status _status;
  bool _buffer;
  rocksdb::ReadOptions _options;
  v8::Local<v8::Object> _keyObj;
};

#endif  // GetWorker_H
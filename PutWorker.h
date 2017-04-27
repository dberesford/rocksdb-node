#ifndef PutWorker_H
#define PutWorker_H

#include <nan.h>
#include "rocksdb/db.h"

class PutWorker : public Nan::AsyncWorker {
 public:
  PutWorker(Nan::Callback *callback, rocksdb::DB *db, rocksdb::WriteOptions options, v8::Local<v8::Object> &keyObj, v8::Local<v8::Object> &valueObj);
  ~PutWorker();
  virtual void Execute();
  virtual void HandleOKCallback ();

 private:
  rocksdb::DB *_db;
  rocksdb::Slice _key;
  rocksdb::Slice _value;
  rocksdb::Status _status;
  rocksdb::WriteOptions _options;
  v8::Local<v8::Object> _keyObj;
};

#endif  // PutWorker_H
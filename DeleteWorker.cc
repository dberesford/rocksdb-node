#include <nan.h>
#include "DeleteWorker.h"  
#include "rocksdb/db.h"

DeleteWorker::DeleteWorker(Nan::Callback *callback, rocksdb::DB *db, rocksdb::ColumnFamilyHandle *family, rocksdb::Slice key, rocksdb::WriteOptions options)
    : AsyncWorker(callback), _db(db), _family(family), _key(key), _options(options) {}
DeleteWorker::~DeleteWorker() {}

void DeleteWorker::Execute () {
  _status = _db->Delete(_options, _family, _key);
}

void DeleteWorker::HandleOKCallback () {
  Nan::HandleScope scope;
  
  v8::Local<v8::Value> argv[1] = { Nan::Null() };
  if (!_status.ok()) {
    v8::Local<v8::Value> errv[1] = {};
    errv[0] = Nan::Error(_status.getState());
    callback->Call(1, errv);
    return;
  } 

  callback->Call(1, argv);
}

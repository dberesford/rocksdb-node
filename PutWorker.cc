#include <nan.h>
#include "PutWorker.h"  
#include "rocksdb/db.h"

PutWorker::PutWorker(Nan::Callback *callback, rocksdb::DB *db, rocksdb::WriteOptions options, v8::Local<v8::Object> &keyObj, v8::Local<v8::Object> &valueObj)
    : AsyncWorker(callback), _db(db), _options(options), _keyObj(keyObj) {
      SaveToPersistent("key", keyObj);
      SaveToPersistent("value", valueObj);

  _key = node::Buffer::HasInstance(keyObj) ? rocksdb::Slice(node::Buffer::Data(keyObj), node::Buffer::Length(keyObj))
                                           : rocksdb::Slice(std::string(*Nan::Utf8String(keyObj)));
  _value = node::Buffer::HasInstance(valueObj) ? rocksdb::Slice(node::Buffer::Data(valueObj), node::Buffer::Length(valueObj))
                                               : rocksdb::Slice(std::string(*Nan::Utf8String(valueObj)));
}

PutWorker::~PutWorker() {}

void PutWorker::Execute () {
  _status = _db->Put(_options, _key, _value);
}

void PutWorker::HandleOKCallback () {
  Nan::HandleScope scope;
  
  v8::Local<v8::Value> argv[1] = { Nan::Null() };
  if (!_status.ok()) {
    argv[0] = Nan::New<v8::String>(_status.getState()).ToLocalChecked();
  } 
  callback->Call(1, argv);
}

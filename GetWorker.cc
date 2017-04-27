#include <nan.h>
#include "GetWorker.h"  
#include "rocksdb/db.h"

GetWorker::GetWorker(Nan::Callback *callback, rocksdb::DB *db, bool buffer, rocksdb::ReadOptions options, v8::Local<v8::Object> &keyObj)
    : AsyncWorker(callback), _db(db), _buffer(buffer), _options(options), _keyObj(keyObj) {
  SaveToPersistent("key", keyObj);
  _key = node::Buffer::HasInstance(keyObj) ? rocksdb::Slice(node::Buffer::Data(keyObj), node::Buffer::Length(keyObj))
                                           : rocksdb::Slice(std::string(*Nan::Utf8String(keyObj)));
}

GetWorker::~GetWorker() {}

void GetWorker::Execute () {
  _status = _db->Get(_options, _key, &_value);
}

void GetWorker::HandleOKCallback () {
  Nan::HandleScope scope;
  v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::Null() };
 
  if (_status.IsNotFound()) {
    callback->Call(2, argv);
    return;
  }
 
  if (!_status.ok()) {
    v8::Local<v8::Value> errv[1] = {};
    errv[0] = Nan::Error(_status.getState());
    callback->Call(1, errv);
    return;
  } 

  if (_buffer) {
    argv[1] = Nan::CopyBuffer((char*)_value.data(), _value.size()).ToLocalChecked();
  } else {
    argv[1] = Nan::New<v8::String>((char*)_value.data()).ToLocalChecked();
  }
  callback->Call(2, argv);
}

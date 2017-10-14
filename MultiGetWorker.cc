#include <nan.h>
#include "MultiGetWorker.h"
#include "rocksdb/db.h"

MultiGetWorker::MultiGetWorker(Nan::Callback *callback, rocksdb::DB *db, rocksdb::ReadOptions options, rocksdb::ColumnFamilyHandle *columnFamily, v8::Local<v8::Array> &keysArray)
  : AsyncWorker(callback), _db(db), _options(options), _keysArray(keysArray) {
  SaveToPersistent("key", keysArray);

  for (unsigned int i = 0; i < keysArray->Length(); i++) {
    std::string *str = new std::string(*Nan::Utf8String(keysArray->Get(i)));
    rocksdb::Slice s = rocksdb::Slice(*str);
    _keys.push_back(s);
  }

  // Currently just one column family passed, i.e. multigets only supported in one column
  // Change here in future to support passing array of cf handles
  for (unsigned int i = 0; i < keysArray->Length(); i++) {
    _families.push_back(columnFamily);
  }

}

MultiGetWorker::~MultiGetWorker() {}

void MultiGetWorker::Execute () {
  _statuss = _db->MultiGet(_options, _families, _keys, &_values);
}

void MultiGetWorker::HandleOKCallback () {
  Nan::HandleScope scope;
  v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::Null() };

  v8::Local<v8::Array> arr = Nan::New<v8::Array>();
  for(unsigned i = 0; i != _values.size(); i++) {
    rocksdb::Status s = _statuss[i];
    if (s.ok()) {
      std::string val = _values[i];
      Nan::Set(arr, i, Nan::New(val).ToLocalChecked());
    } else if (s.IsNotFound()) {
      Nan::Set(arr, i, Nan::Null());
    } else {
      v8::Local<v8::Value> errv[1] = {};
      errv[0] = Nan::Error(s.getState());
      callback->Call(1, errv);
      return;
    }
  }

  argv[1] = arr;
  callback->Call(2, argv);
}

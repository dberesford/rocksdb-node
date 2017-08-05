#include <nan.h>
#include "CompactRangeWorker.h"
#include "rocksdb/db.h"
using namespace std;

CompactRangeWorker::CompactRangeWorker(Nan::Callback *callback, rocksdb::DB *db, rocksdb::CompactRangeOptions options, rocksdb::ColumnFamilyHandle *family, v8::Local<v8::Object> &fromObj, v8::Local<v8::Object> &toObj)
  : AsyncWorker(callback), _db(db), _options(options), _family(family) {
    SaveToPersistent("from", fromObj);
    SaveToPersistent("to", toObj);

    _from = NULL;
    if (!fromObj->IsNull()) {
      _from = node::Buffer::HasInstance(fromObj) ? new rocksdb::Slice(node::Buffer::Data(fromObj), node::Buffer::Length(fromObj))
        : new rocksdb::Slice(string(*Nan::Utf8String(fromObj)));
    }

    _to = NULL;
    if (!toObj->IsNull()) {
      _to = node::Buffer::HasInstance(toObj) ? new rocksdb::Slice(node::Buffer::Data(toObj), node::Buffer::Length(toObj))
        : new rocksdb::Slice(string(*Nan::Utf8String(toObj)));
    }
}

CompactRangeWorker::~CompactRangeWorker() {
  if (_from) delete _from;
  if (_to) delete _to;
}

void CompactRangeWorker::Execute () {
  _status = _db->CompactRange(_options, _from, _to);
}

void CompactRangeWorker::HandleOKCallback () {
  Nan::HandleScope scope;

  v8::Local<v8::Value> argv[1] = { Nan::Null() };
  if (!_status.ok()) {
    argv[0] = Nan::New<v8::String>(_status.getState()).ToLocalChecked();
  } 
  callback->Call(1, argv);
}

#include <node.h>
#include <nan.h>
#include "rocksdb/db.h"
#include "Batch.h"
#include "RocksDBNode.h"
#include <iostream>
//using namespace std;

Nan::Persistent<v8::FunctionTemplate> batch_constructor;

Batch::Batch(RocksDBNode *rocksDBNode) {
  _rocksDBNode = rocksDBNode;
}

Batch::~Batch() {
}

void Batch::Init() {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(Batch::New);
  tpl->SetClassName(Nan::New("Batch").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "put", Batch::Put);
  Nan::SetPrototypeMethod(tpl, "del", Batch::Delete);
  batch_constructor.Reset(tpl);
} 

NAN_METHOD(Batch::New) {
  // We expect Batch(rocksDBNode)
  int rocksIndex = -1;
  if (info.Length() == 1) {
    rocksIndex = 0;
  } else {
    Nan::ThrowTypeError("Wrong number of arguments");
    return;
  }

  RocksDBNode* rocks = Nan::ObjectWrap::Unwrap<RocksDBNode>(info[rocksIndex].As<v8::Object>());
  Batch* obj = new Batch(rocks);
  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void Batch::NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  // Note: we pass an additional argument here which is the RocksDBNode object
  // that is creating the new Batch. Batches can't be created anywhere else.
  const unsigned argc = args.Length() + 1;
  v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
  for (unsigned int i=0; i<argc-1; i++) {
    argv[i] = args[i];
  }
  argv[argc-1] = args.Holder();
  v8::Local<v8::FunctionTemplate> cons = Nan::New<v8::FunctionTemplate>(batch_constructor);
  v8::MaybeLocal<v8::Object> instance;
  v8::Local<v8::Value> err;
  bool hasException = false;
  {
    Nan::TryCatch tc;
    instance = Nan::NewInstance(cons->GetFunction(), argc, argv);
    if (tc.HasCaught()) {
      err = tc.Exception();
      hasException = true;
    }
  }

  if (hasException) {
    isolate->ThrowException(err);
  } else {
    args.GetReturnValue().Set(instance.ToLocalChecked());
  }

  delete [] argv;
  argv = NULL;
}

NAN_METHOD(Batch::Put) {
  int familyIndex = -1;
  int keyIndex = -1;
  int valueIndex = -1;

  // 2 args, assume key value
  if (info.Length() == 2) {
    keyIndex = 0;
    valueIndex = 1;
  } else if (info.Length() == 3) {
    // 3 args is (family, key, value)
    familyIndex = 0;
    keyIndex = 1;
    valueIndex = 2;
  } else {
    Nan::ThrowTypeError("Wrong number of arguments");
    return;
  }

  v8::Local<v8::Object> keyObj = info[keyIndex].As<v8::Object>();
  v8::Local<v8::Object> valueObj = info[valueIndex].As<v8::Object>();

  Batch* rocksDBBatch = ObjectWrap::Unwrap<Batch>(info.Holder());

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(info[familyIndex]));
    columnFamily = rocksDBBatch->_rocksDBNode->GetColumnFamily(family);
  } else {
    columnFamily = rocksDBBatch->_rocksDBNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  if (columnFamily == NULL) {
    Nan::ThrowError("Column Family does not exist");
    return;
  }

  rocksdb::Slice key = node::Buffer::HasInstance(info[keyIndex]) ? rocksdb::Slice(node::Buffer::Data(keyObj), node::Buffer::Length(keyObj))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(info[keyIndex])));

  rocksdb::Slice value = node::Buffer::HasInstance(info[valueIndex]) ? rocksdb::Slice(node::Buffer::Data(valueObj), node::Buffer::Length(valueObj))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(info[valueIndex])));
  rocksDBBatch->_batch.Put(columnFamily, key, value);
}

NAN_METHOD(Batch::Delete) {
  int familyIndex = -1;
  int keyIndex = -1;

  // if only one arg, assume it's the key
  if (info.Length() == 1) {
    keyIndex = 0;
  } else if (info.Length() == 2) {
    // the two args are (family, key)
    familyIndex = 0;
    keyIndex = 1;
  } else {
    Nan::ThrowTypeError("Wrong number of arguments");
    return;
  }

  rocksdb::Slice key = node::Buffer::HasInstance(info[keyIndex]) ? rocksdb::Slice(node::Buffer::Data(info[keyIndex]->ToObject()), node::Buffer::Length(info[keyIndex]->ToObject()))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(info[keyIndex])));

  Batch* rocksDBBatch = ObjectWrap::Unwrap<Batch>(info.Holder());

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(info[familyIndex]));
    columnFamily = rocksDBBatch->_rocksDBNode->GetColumnFamily(family);
  } else {
    columnFamily = rocksDBBatch->_rocksDBNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  if (columnFamily == NULL) {
    Nan::ThrowError("Column Family does not exist");
    return;
  }

  rocksDBBatch->_batch.Delete(columnFamily, key);
}

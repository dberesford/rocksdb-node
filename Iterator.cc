#include <node.h>
#include <nan.h>
#include "rocksdb/db.h"
#include "Iterator.h"
#include "OptionsHelper.h"
#include "DBNode.h"
#include "Errors.h"
#include <iostream>

Nan::Persistent<v8::FunctionTemplate> iterator_constructor;

Iterator::Iterator (rocksdb::ReadOptions options, rocksdb::ColumnFamilyHandle *handle, rocksdb::DB* db) {
  if (handle != NULL) {
    _it = db->NewIterator(options, handle);
  } else {
    _it = db->NewIterator(options);
  }
}

Iterator::~Iterator () {
}

void Iterator::Init() {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(Iterator::New);
  tpl->SetClassName(Nan::New("Iterator").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "seekToFirst", Iterator::SeekToFirst);
  Nan::SetPrototypeMethod(tpl, "seekToLast", Iterator::SeekToLast);
  Nan::SetPrototypeMethod(tpl, "seek", Iterator::Seek);
  Nan::SetPrototypeMethod(tpl, "valid", Iterator::Valid);
  Nan::SetPrototypeMethod(tpl, "next", Iterator::Next);
  Nan::SetPrototypeMethod(tpl, "prev", Iterator::Prev);
  Nan::SetPrototypeMethod(tpl, "key", Iterator::Key);
  Nan::SetPrototypeMethod(tpl, "value", Iterator::Value);
  Nan::SetPrototypeMethod(tpl, "status", Iterator::Status);
  iterator_constructor.Reset(tpl);
}

NAN_METHOD(Iterator::New) {
  // We expect newIterator(<options>, <columnFamilyName>, dbNode), where both options and columnFamilyName are both optional
  int optsIndex = -1;
  int columnFamilyIndex = -1;
  int rocksIndex = -1;

  if (info.Length() == 1) {
    rocksIndex = 0;
  } else if (info.Length() == 2) {
    // newIterator(options, dbNode)
    if (info[0]->IsObject()) {
      optsIndex = 0;
      rocksIndex = 1;
    } else {
      // newIterator(columnFamilyName, dbNode)
      columnFamilyIndex = 0;
      rocksIndex = 1;
    }
  } else if (info.Length() == 3) {
    optsIndex = 0;
    columnFamilyIndex = 1;
    rocksIndex = 2;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  rocksdb::ReadOptions options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessReadOptions(opts, &options);
  }

  DBNode* dbNode = Nan::ObjectWrap::Unwrap<DBNode>(info[rocksIndex].As<v8::Object>());

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (columnFamilyIndex != -1) {
    string family = string(*Nan::Utf8String(info[columnFamilyIndex]));
    columnFamily = dbNode->GetColumnFamily(family);
  }

  Iterator* obj = new Iterator(options, columnFamily, dbNode->db());
  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void Iterator::NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  // Note: we pass an additional argument here which is the DBNode object
  // that is creating the new Iterator. Iterators can't be created anywhere else.
  const unsigned argc = args.Length() + 1;
  v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
  for (unsigned int i=0; i<argc-1; i++) {
    argv[i] = args[i];
  }
  argv[argc-1] = args.Holder();

  v8::Local<v8::FunctionTemplate> cons = Nan::New<v8::FunctionTemplate>(iterator_constructor);

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

NAN_METHOD(Iterator::SeekToFirst) {
  Iterator* it = Nan::ObjectWrap::Unwrap<Iterator>(info.Holder());   
  it->_it->SeekToFirst();
}

NAN_METHOD(Iterator::SeekToLast) {
  Iterator* it = Nan::ObjectWrap::Unwrap<Iterator>(info.Holder());   
  it->_it->SeekToLast();
}

NAN_METHOD(Iterator::Valid) {
  Iterator* it = Nan::ObjectWrap::Unwrap<Iterator>(info.Holder());   
  bool valid = it->_it->Valid();
  info.GetReturnValue().Set(valid);
}

NAN_METHOD(Iterator::Next) {
  Iterator* it = Nan::ObjectWrap::Unwrap<Iterator>(info.Holder());   
  it->_it->Next();
}

NAN_METHOD(Iterator::Prev) {
  Iterator* it = Nan::ObjectWrap::Unwrap<Iterator>(info.Holder());   
  it->_it->Prev();
}

NAN_METHOD(Iterator::Key) {
  // buffer is a special non-rocks option, it's specific to rocksdb-node
  bool buffer = false;
  if (info.Length() == 1) {
    v8::Local<v8::String> key = Nan::New("buffer").ToLocalChecked();  
    v8::Local<v8::Object> opts = info[0].As<v8::Object>();
    if (!opts.IsEmpty() && opts->Has(key)) {
      buffer = opts->Get(key)->BooleanValue();
    }
  }

  Iterator* it = Nan::ObjectWrap::Unwrap<Iterator>(info.Holder());   
  rocksdb::Slice k = it->_it->key();

  if (buffer) {
    info.GetReturnValue().Set(Nan::CopyBuffer((char*)k.data(), k.size()).ToLocalChecked());
  } else {
    info.GetReturnValue().Set(Nan::New<v8::String>(k.ToString()).ToLocalChecked());
  }
}

NAN_METHOD(Iterator::Value) {
  // buffer is a special non-rocks option, it's specific to rocksdb-node
  bool buffer = false;
  if (info.Length() == 1) {
    v8::Local<v8::String> key = Nan::New("buffer").ToLocalChecked();  
    v8::Local<v8::Object> opts = info[0].As<v8::Object>();
    if (!opts.IsEmpty() && opts->Has(key)) {
      buffer = opts->Get(key)->BooleanValue();
    }
  }

  Iterator* it = Nan::ObjectWrap::Unwrap<Iterator>(info.Holder());   
  rocksdb::Slice v = it->_it->value();

  if (buffer) {
    info.GetReturnValue().Set(Nan::CopyBuffer((char*)v.data(), v.size()).ToLocalChecked());
  } else {
    info.GetReturnValue().Set(Nan::New<v8::String>(v.ToString()).ToLocalChecked());
  }
}

NAN_METHOD(Iterator::Seek) {
  // We expect either one or two opts - Options can be optional, and the seek target
  int optsIndex = -1;
  int targetIndex = -1;

  if (info.Length() == 1) {
    targetIndex = 0;
  } else if (info.Length() == 2) {
    optsIndex = 0;
    targetIndex = 1;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  // buffer is a special non-rocks option, it's specific to rocksdb-node
  bool buffer = false;
  if (info.Length() == 1) {
    v8::Local<v8::String> key = Nan::New("buffer").ToLocalChecked();  
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    if (!opts.IsEmpty() && opts->Has(key)) {
      buffer = opts->Get(key)->BooleanValue();
    }
  }

  v8::Local<v8::Object> targetObj = info[targetIndex].As<v8::Object>();
  
  Iterator* it = Nan::ObjectWrap::Unwrap<Iterator>(info.Holder());
  rocksdb::Slice target = node::Buffer::HasInstance(targetObj) ? rocksdb::Slice(node::Buffer::Data(targetObj), node::Buffer::Length(targetObj))
                                                           : rocksdb::Slice(string(*Nan::Utf8String(targetObj)));

  it->_it->Seek(target);
}

NAN_METHOD(Iterator::Status) {
  Iterator* it = Nan::ObjectWrap::Unwrap<Iterator>(info.Holder());   
  rocksdb::Status s = it->_it->status();
  if (s.ok()) {
    info.GetReturnValue().SetNull();
  } else {
    info.GetReturnValue().Set(Nan::Error(s.getState()));
  }
}


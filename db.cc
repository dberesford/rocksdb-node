#include <nan.h>
#include "db.h"  
#include <iostream>

#include "rocksdb/db.h"

using namespace rocksdb;
using namespace std;
using namespace v8;

NAN_METHOD(OpenDB) {
  Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return;
  }

  string path = string(*Nan::Utf8String(info[0]));
  DB* db;

  // TODO - pass options in as param..
  Options options;
  options.create_if_missing = true;
  //options.error_if_exists = true;

  Status s = DB::Open(options, path, &db);
  if (!s.ok()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, s.getState())));
    return;
  }

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "msg"), info[0]->ToString());

  info.GetReturnValue().Set(obj);
}
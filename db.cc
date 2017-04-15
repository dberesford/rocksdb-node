#include <nan.h>
#include "db.h"  
#include <iostream>

#include "rocksdb/db.h"

using namespace rocksdb;
using namespace std;
using namespace v8;

NAN_METHOD(OpenDB) {
  Isolate* isolate = info.GetIsolate();
  if (info.Length() < 2) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return;
  }

  Local<Object> opts = info[0].As<Object>(); 
  string path = string(*Nan::Utf8String(info[1]));
  DB* db;

  Options options;
  
  // TODO - figure out the dynamic way of doing this in c++.. 
  v8::Local<v8::String> create_if_missing = Nan::New("create_if_missing").ToLocalChecked();
  if (opts->Has(create_if_missing)) options.create_if_missing = opts->Get(create_if_missing)->BooleanValue();

  v8::Local<v8::String> error_if_exists = Nan::New("error_if_exists").ToLocalChecked();
  if (opts->Has(error_if_exists)) options.error_if_exists = opts->Get(error_if_exists)->BooleanValue();
  
  Status s = DB::Open(options, path, &db);
  if (!s.ok()) {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, s.getState())));
    return;
  }

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "msg"), info[0]->ToString());

  info.GetReturnValue().Set(obj);
}
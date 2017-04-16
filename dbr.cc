#include <node.h>
#include <nan.h>
#include <iostream>
#include "dbr.h"  
#include "rocksdb/db.h"
#include <list>
using namespace std;

v8::Persistent<v8::Function> DBR::constructor;

DBR::DBR(rocksdb::Options options, string path, rocksdb::DB *db) {
  _options = options;
  _path = path;
  _db = db;
}

DBR::~DBR() {
  delete _db;
}

void DBR::Init(v8::Local<v8::Object> exports) {
  v8::Isolate* isolate = exports->GetIsolate();
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
  tpl->SetClassName(v8::String::NewFromUtf8(isolate, "DBR"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tpl, "put", DBR::Put);
  NODE_SET_PROTOTYPE_METHOD(tpl, "get", DBR::Get);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(v8::String::NewFromUtf8(isolate, "DBR"), tpl->GetFunction());
}

void DBR::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  if (args.IsConstructCall()) {
    
    // TODO - strange node crash if args aren't correct..
    if (args.Length() < 2) {
      isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Wrong number of arguments")));
      return;
    }
  
    v8::Local<v8::Object> opts = args[0].As<v8::Object>(); 
    string path = string(*Nan::Utf8String(args[1]));    
    rocksdb::Options options;
  
    // TODO - figure out the dynamic way of doing this in c++.. 
    v8::Local<v8::String> create_if_missing = Nan::New("create_if_missing").ToLocalChecked();
    if (opts->Has(create_if_missing)) options.create_if_missing = opts->Get(create_if_missing)->BooleanValue();

    v8::Local<v8::String> error_if_exists = Nan::New("error_if_exists").ToLocalChecked();
    if (opts->Has(error_if_exists)) options.error_if_exists = opts->Get(error_if_exists)->BooleanValue();
  
    rocksdb::DB* db;
    rocksdb::Status s = rocksdb::DB::Open(options, path, &db);
    if (!s.ok()) {
      isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, s.getState())));
      return;
    }
  
    DBR* dbr = new DBR(options, path, db);
    dbr->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    const unsigned argc = args.Length();
    v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
    for (unsigned int i=0; i<argc; i++) {
      argv[i] = args[i];
    }

    v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Object> instance = cons->NewInstance(context, argc, argv).ToLocalChecked();
    
    delete [] argv;
    argv = NULL;
    args.GetReturnValue().Set(instance);
  }
}

void DBR::NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  const unsigned argc = args.Length();
  v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
  for (unsigned int i=0; i<argc; i++) {
    argv[i] = args[i];
  }

  v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> instance = cons->NewInstance(context, argc, argv).ToLocalChecked();
  
  delete [] argv;
  argv = NULL;
  args.GetReturnValue().Set(instance);
}

void DBR::Put(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  if (args.Length() < 2) {
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return;
  }

  string key = string(*Nan::Utf8String(args[0])); 
  string value = string(*Nan::Utf8String(args[1])); 

  DBR* dbr = ObjectWrap::Unwrap<DBR>(args.Holder());
  rocksdb::Status s = dbr->_db->Put(rocksdb::WriteOptions(), key, value);
  if (!s.ok()) {
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, s.getState())));
    return;
  }
}

void DBR::Get(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  if (args.Length() < 1) {
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return;
  }

  string key = string(*Nan::Utf8String(args[0])); 
  string value;

  DBR* dbr = ObjectWrap::Unwrap<DBR>(args.Holder());
  rocksdb::Status s = dbr->_db->Get(rocksdb::ReadOptions(), key, &value);
  
  if (s.IsNotFound()) {
    args.GetReturnValue().Set(Nan::Null());
    return;
  }

  if (!s.ok()) {
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, s.getState())));
    return;
  }

  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, value.c_str()));
}

#include <node.h>
#include <nan.h>
#include <iostream>
#include "RocksDBNode.h"  
#include "rocksdb/db.h"
#include <list>
using namespace std;

v8::Persistent<v8::Function> RocksDBNode::constructor;

RocksDBNode::RocksDBNode(rocksdb::Options options, string path, rocksdb::DB *db) {
  _options = options;
  _path = path;
  _db = db;
}

RocksDBNode::~RocksDBNode() {
  delete _db;
}

void RocksDBNode::Init(v8::Local<v8::Object> exports) {
  v8::Isolate* isolate = exports->GetIsolate();
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
  tpl->SetClassName(v8::String::NewFromUtf8(isolate, "RocksDBNode"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tpl, "put", RocksDBNode::Put);
  NODE_SET_PROTOTYPE_METHOD(tpl, "get", RocksDBNode::Get);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(v8::String::NewFromUtf8(isolate, "RocksDBNode"), tpl->GetFunction());
}

void RocksDBNode::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
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
  
    RocksDBNode* rocksDBNode = new RocksDBNode(options, path, db);
    rocksDBNode->Wrap(args.This());
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

void RocksDBNode::NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args) {
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

void RocksDBNode::Put(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  if (args.Length() < 2) {
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return;
  }

  // TODO - check for key undefined or null
  rocksdb::Slice key = node::Buffer::HasInstance(args[0]) ? rocksdb::Slice(node::Buffer::Data(args[0]->ToObject()), node::Buffer::Length(args[0]->ToObject()))
                                                          : rocksdb::Slice(string(*Nan::Utf8String(args[0])));
  rocksdb::Slice value = node::Buffer::HasInstance(args[1]) ? rocksdb::Slice(node::Buffer::Data(args[1]->ToObject()), node::Buffer::Length(args[1]->ToObject()))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(args[1])));
  
  RocksDBNode* rocksDBNode = ObjectWrap::Unwrap<RocksDBNode>(args.Holder());
  rocksdb::Status s;

  s = rocksDBNode->_db->Put(rocksdb::WriteOptions(), key, value);
  if (!s.ok()) {
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, s.getState())));
    return;
  }
}

void RocksDBNode::Get(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  if (args.Length() < 1) {
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return;
  }

  rocksdb::Slice key = node::Buffer::HasInstance(args[0]) ? rocksdb::Slice(node::Buffer::Data(args[0]->ToObject()), node::Buffer::Length(args[0]->ToObject()))
                                                          : rocksdb::Slice(string(*Nan::Utf8String(args[0])));
  // TODO - need to pass an option to Get or something that indicates you want a buffer returned
  string value;

  RocksDBNode* rocksDBNode = ObjectWrap::Unwrap<RocksDBNode>(args.Holder());
  rocksdb::Status s = rocksDBNode->_db->Get(rocksdb::ReadOptions(), key, &value);
  
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

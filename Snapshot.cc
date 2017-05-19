#include <node.h>
#include <nan.h>
#include "rocksdb/db.h"
#include "Snapshot.h"
#include "DBNode.h"
#include <iostream>

Nan::Persistent<v8::FunctionTemplate> snapshot_constructor;

Snapshot::Snapshot (rocksdb::DB* db) {
  _snapshot = db->GetSnapshot();
}

Snapshot::~Snapshot () {
}

void Snapshot::Init() {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(Snapshot::New);
  tpl->SetClassName(Nan::New("Snapshot").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  snapshot_constructor.Reset(tpl);
}

NAN_METHOD(Snapshot::New) {
  // We expect GetSnapshot(<options>, <columnFamilyName>, dbNode), where both options and columnFamilyName are both optional
  int rocksIndex = 0;

  if (info.Length() == 1) {
    rocksIndex = 0;
  } else {
    Nan::ThrowTypeError("Wrong number of arguments");
    return;
  }

  DBNode* dbNode = Nan::ObjectWrap::Unwrap<DBNode>(info[rocksIndex].As<v8::Object>());

  Snapshot* obj = new Snapshot(dbNode->db());
  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void Snapshot::NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  // Note: we pass an additional argument here which is the DBNode object
  // that is creating the new Snapshot. Snapshots can't be created anywhere else.
  const unsigned argc = args.Length() + 1;
  v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
  for (unsigned int i=0; i<argc-1; i++) {
    argv[i] = args[i];
  }
  argv[argc-1] = args.Holder();

  v8::Local<v8::FunctionTemplate> cons = Nan::New<v8::FunctionTemplate>(snapshot_constructor);

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

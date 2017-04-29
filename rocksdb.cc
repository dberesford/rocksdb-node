#include <nan.h>
#include <node.h>
#include "RocksDBNode.h"
#include "Iterator.h"

void CreateDB(const v8::FunctionCallbackInfo<v8::Value>& args) {
  RocksDBNode::NewInstance(args);
}

void InitAll(v8::Local<v8::Object> exports, v8::Local<v8::Object> module) {
  RocksDBNode::Init(exports);
  Iterator::Init();
  NODE_SET_METHOD(module, "exports", CreateDB);
}

NODE_MODULE(addon, InitAll)
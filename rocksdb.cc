#include <nan.h>
#include <node.h>
#include "dbr.h"

void CreateDB(const v8::FunctionCallbackInfo<v8::Value>& args) {
  DBR::NewInstance(args);
}

void InitAll(v8::Local<v8::Object> exports, v8::Local<v8::Object> module) {
  DBR::Init(exports);
  NODE_SET_METHOD(module, "exports", CreateDB);
}

NODE_MODULE(addon, InitAll)
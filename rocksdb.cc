#include <nan.h>
#include <node.h>
#include "RocksDBNode.h"
#include "Iterator.h"

void InitAll(v8::Local<v8::Object> exports, v8::Local<v8::Object> module) {
  RocksDBNode::Init(exports);
  Iterator::Init();
  NODE_SET_METHOD(exports, "open", RocksDBNode::NewInstance);
  NODE_SET_METHOD(exports, "listColumnFamilies", RocksDBNode::ListColumnFamilies);
}

NODE_MODULE(addon, InitAll)
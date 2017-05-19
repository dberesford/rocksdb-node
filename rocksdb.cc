#include <nan.h>
#include <node.h>
#include "DBNode.h"
#include "Iterator.h"
#include "Snapshot.h"
#include "Batch.h"

void InitAll(v8::Local<v8::Object> exports, v8::Local<v8::Object> module) {
  DBNode::Init(exports);
  Iterator::Init();
  Snapshot::Init();
  Batch::Init();
  NODE_SET_METHOD(exports, "open", DBNode::NewInstance);
  NODE_SET_METHOD(exports, "listColumnFamilies", DBNode::ListColumnFamilies);
}

NODE_MODULE(addon, InitAll)

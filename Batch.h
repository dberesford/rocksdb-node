#ifndef Batch_H
#define Batch_H

#include <nan.h>
#include "RocksDBNode.h"
#include "rocksdb/db.h"

using namespace std;

class Batch : public Nan::ObjectWrap {
 public:
  static void Init();
  static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);
  rocksdb::WriteBatch _batch;
  RocksDBNode *_rocksDBNode;

 private:
  explicit Batch(RocksDBNode *rocksDBNode);
  ~Batch();

  static NAN_METHOD(New);
  static NAN_METHOD(Put);
  static NAN_METHOD(Delete);

};

#endif  // Batch_H

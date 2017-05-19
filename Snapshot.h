#ifndef Snapshot_H
#define Snapshot_H

#include <nan.h>
#include "rocksdb/db.h"
using namespace std;

class Snapshot : public Nan::ObjectWrap {
  public:
    static void Init();
    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);
    const rocksdb::Snapshot* _snapshot;
 private: 
   explicit Snapshot(rocksdb::DB* db);
   ~Snapshot();
 
   static NAN_METHOD(New);
};

#endif  // Snapshot_H

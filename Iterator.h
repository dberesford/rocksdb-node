#ifndef Iterator_H
#define Iterator_H

#include <nan.h>
#include "rocksdb/db.h"
using namespace std;

class Iterator : public Nan::ObjectWrap {
  public:
    static void Init();
    static void NewInstance(const Nan::FunctionCallbackInfo<v8::Value>& args);
    //static void NewInstance(const Nan::FunctionCallbackInfo &args);
    rocksdb::Iterator* _it;
 private: 
   explicit Iterator(rocksdb::ReadOptions options, rocksdb::ColumnFamilyHandle *handle, rocksdb::DB* db);
   ~Iterator();
 
   static NAN_METHOD(New);
   static NAN_METHOD(SeekToFirst);
   static NAN_METHOD(SeekToLast);
   static NAN_METHOD(Seek);
   static NAN_METHOD(Valid);
   static NAN_METHOD(Next);
   static NAN_METHOD(Prev);
   static NAN_METHOD(Key);
   static NAN_METHOD(Value);
   static NAN_METHOD(Status);
};

#endif  // Iterator_H

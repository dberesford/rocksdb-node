#ifndef RocksDBNode_H
#define RocksDBNode_H

#include <nan.h>
#include "rocksdb/db.h"
#include <iostream>

using namespace std;

class RocksDBNode : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);
  static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);
  inline rocksdb::DB* db() { return _db;}
  
 private:
  explicit RocksDBNode(rocksdb::Options options, string path, rocksdb::DB* db);
  ~RocksDBNode();

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  
  static v8::Persistent<v8::Function> constructor;
  static void Put(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Get(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Delete(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void NewIterator(const v8::FunctionCallbackInfo<v8::Value>& info);

  rocksdb::DB* _db;
  rocksdb::Options _options;
  string _path;

};

#endif  // RocksDBNode_H

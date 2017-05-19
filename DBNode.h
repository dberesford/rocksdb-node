#ifndef DBNode_H
#define DBNode_H

#include <nan.h>
#include "rocksdb/db.h"
#include <iostream>

using namespace std;

class DBNode : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);
  static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);
  inline rocksdb::DB* db() { return _db;}
  static void ListColumnFamilies(const v8::FunctionCallbackInfo<v8::Value>& info);
  rocksdb::ColumnFamilyHandle* GetColumnFamily(string family);

 private:
  explicit DBNode(rocksdb::Options options, string path, rocksdb::DB *db, std::vector<rocksdb::ColumnFamilyHandle*> *cfHandles);
  ~DBNode();

  rocksdb::Status DeleteColumnFamily(string family);

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  static v8::Persistent<v8::Function> constructor;
  static void Put(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Get(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Delete(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void NewIterator(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void ReleaseIterator(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void GetSnapshot(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void ReleaseSnapshot(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void GetColumnFamilies(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CreateColumnFamily(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void DropColumnFamily(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Batch(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Write(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Close(const v8::FunctionCallbackInfo<v8::Value>& info);

  rocksdb::DB *_db;
  rocksdb::Options _options;
  string _path;
  std::vector<rocksdb::ColumnFamilyHandle*> *_cfHandles;

};

#endif  // DBNode_H

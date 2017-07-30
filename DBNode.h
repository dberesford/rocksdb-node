#ifndef DBNode_H
#define DBNode_H

#include <nan.h>
#include "rocksdb/db.h"
#include <iostream>

using namespace std;

class DBNode : public Nan::ObjectWrap {
 public:
  static void Init();
  static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);
  inline rocksdb::DB* db() { return _db;}

  static NAN_METHOD(ListColumnFamilies);
  static NAN_METHOD(DestroyDB);
  rocksdb::ColumnFamilyHandle* GetColumnFamily(string family);

 private:
  explicit DBNode(rocksdb::Options options, string path, rocksdb::DB *db, std::vector<rocksdb::ColumnFamilyHandle*> *cfHandles);
  ~DBNode();

  rocksdb::Status DeleteColumnFamily(string family);

  static NAN_METHOD(New);

  static v8::Persistent<v8::Function> constructor;
  static NAN_METHOD(Put);
  static NAN_METHOD(Get);
  static NAN_METHOD(Delete);
  static NAN_METHOD(NewIterator);
  static NAN_METHOD(ReleaseIterator);
  static NAN_METHOD(GetSnapshot);
  static NAN_METHOD(ReleaseSnapshot);
  static NAN_METHOD(GetColumnFamilies);
  static NAN_METHOD(CreateColumnFamily);
  static NAN_METHOD(DropColumnFamily);
  static NAN_METHOD(Batch);
  static NAN_METHOD(Write);
  static NAN_METHOD(Close);
  static NAN_METHOD(GetSstFileWriter);
  static NAN_METHOD(IngestExternalFile);

  rocksdb::DB *_db;
  rocksdb::Options _options;
  string _path;
  std::vector<rocksdb::ColumnFamilyHandle*> *_cfHandles;

};

#endif  // DBNode_H

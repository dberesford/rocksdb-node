#ifndef DBWithTTL_H
#define DBWithTTL_H

#include <nan.h>
#include "rocksdb/db.h"
#include "rocksdb/utilities/db_ttl.h"
#include <iostream>
#include "DBNode.h"

using namespace std;

class DBWithTTL : public DBNode {
 public:
  static void Init();
  static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);

 private:
  explicit DBWithTTL(rocksdb::Options options, string path, rocksdb::DBWithTTL *db, std::vector<rocksdb::ColumnFamilyHandle*> *cfHandles);
  ~DBWithTTL();

  static NAN_METHOD(New);

  /*
  rocksdb::DB *_db;
  rocksdb::Options _options;
  string _path;
  */
};

#endif  // DBWithTTL_H

#include <nan.h>
#include "db.h"  
#include <iostream>

#include "rocksdb/db.h"

using namespace rocksdb;
using namespace std;

NAN_METHOD(OpenDB) {
  string path = string(*Nan::Utf8String(info[0]));
  cout << "creating: " << path;

  DB* db;

  // TODO - pass options in..
  
  Options options;
  options.create_if_missing = true;

  // open DB
  Status s = DB::Open(options, path, &db);
  if (!s.ok()) cerr << s.ToString() << endl;


  // TODO!!
  info.GetReturnValue().Set(10);
}
#ifndef FileWriter_H
#define FileWriter_H

#include <nan.h>
#include "rocksdb/db.h"
using namespace std;

class FileWriter : public Nan::ObjectWrap {
  public:
    static void Init();
    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);
    rocksdb::SstFileWriter* _sstFileWriter;
 private: 
   explicit FileWriter(rocksdb::ColumnFamilyHandle *handle, rocksdb::DB* db);
   ~FileWriter();
 
   static NAN_METHOD(New);
   static NAN_METHOD(Open);
   static NAN_METHOD(Add);
   static NAN_METHOD(Finish);
   static NAN_METHOD(FileSize);
};

#endif  // FileWriter_H

#include <node.h>
#include <nan.h>
#include "rocksdb/db.h"
#include "FileWriter.h"
#include "DBNode.h"
#include "Errors.h"
#include <iostream>

Nan::Persistent<v8::FunctionTemplate> filewriter_constructor;

FileWriter::FileWriter (rocksdb::ColumnFamilyHandle *handle, rocksdb::DB* db) {

  rocksdb::EnvOptions env(db->GetDBOptions());
  rocksdb::Options options = db->GetOptions();
  const rocksdb::Comparator* comparator = handle->GetComparator();

  _sstFileWriter = new rocksdb::SstFileWriter(env, options, comparator, handle, true);
}

FileWriter::~FileWriter () {
  if (_sstFileWriter) {
    delete _sstFileWriter;
    _sstFileWriter = NULL;
  }
}

void FileWriter::Init() {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(FileWriter::New);
  tpl->SetClassName(Nan::New("FileWriter").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "open", FileWriter::Open);
  Nan::SetPrototypeMethod(tpl, "add", FileWriter::Add);
  Nan::SetPrototypeMethod(tpl, "finish", FileWriter::Finish);
  Nan::SetPrototypeMethod(tpl, "fileSize", FileWriter::FileSize);
  filewriter_constructor.Reset(tpl);
}

NAN_METHOD(FileWriter::New) {
  // We expect new FileWriter(<columnFamilyName>, dbNode), where columnFamilyName is optional
  int columnFamilyIndex = -1;
  int rocksIndex = -1;

  if (info.Length() == 1) {
    rocksIndex = 0;
  } else if (info.Length() == 2) {
    // newFileWriter(columnFamilyIndex, dbNode)
    columnFamilyIndex = 0;
    rocksIndex = 1;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  DBNode* dbNode = Nan::ObjectWrap::Unwrap<DBNode>(info[rocksIndex].As<v8::Object>());

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (columnFamilyIndex != -1) {
    string family = string(*Nan::Utf8String(info[columnFamilyIndex]));
    columnFamily = dbNode->GetColumnFamily(family);
  } else {
    columnFamily = dbNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  FileWriter* obj = new FileWriter(columnFamily, dbNode->db());
  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void FileWriter::NewInstance(const Nan::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  // Note: we pass an additional argument here which is the DBNode object
  // that is creating the new FileWriter. FileWriters can't be created anywhere else.
  const unsigned argc = args.Length() + 1;
  v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
  for (unsigned int i=0; i<argc-1; i++) {
    argv[i] = args[i];
  }
  argv[argc-1] = args.Holder();

  v8::Local<v8::FunctionTemplate> cons = Nan::New<v8::FunctionTemplate>(filewriter_constructor);

  v8::MaybeLocal<v8::Object> instance;
  v8::Local<v8::Value> err;
  bool hasException = false;
  {
    Nan::TryCatch tc;
    instance = Nan::NewInstance(cons->GetFunction(), argc, argv);
    if (tc.HasCaught()) {
      err = tc.Exception();
      hasException = true;
    }
  }

  if (hasException) {
    isolate->ThrowException(err);
  } else {
    args.GetReturnValue().Set(instance.ToLocalChecked());
  }
  delete [] argv;
  argv = NULL;
}

NAN_METHOD(FileWriter::Open) {
  int openIndex = -1;

  if (info.Length() == 1) {
    openIndex = 0;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }
  string path = string(*Nan::Utf8String(info[openIndex]));
  FileWriter* fw = Nan::ObjectWrap::Unwrap<FileWriter>(info.Holder());
  rocksdb::Status s = fw->_sstFileWriter->Open(path);
  if (!s.ok()) {
    Nan::ThrowError(s.getState());
    return;
  }
}

NAN_METHOD(FileWriter::Add) {
  int keyIndex = -1;
  int valueIndex = -1;

  // 2 args, assume key value
  if (info.Length() == 2) {
    keyIndex = 0;
    valueIndex = 1;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  v8::Local<v8::Object> keyObj = info[keyIndex].As<v8::Object>();
  v8::Local<v8::Object> valueObj = info[valueIndex].As<v8::Object>();

  rocksdb::Slice key = node::Buffer::HasInstance(info[keyIndex]) ? rocksdb::Slice(node::Buffer::Data(keyObj), node::Buffer::Length(keyObj))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(info[keyIndex])));

  rocksdb::Slice value = node::Buffer::HasInstance(info[valueIndex]) ? rocksdb::Slice(node::Buffer::Data(valueObj), node::Buffer::Length(valueObj))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(info[valueIndex])));

  FileWriter* fw = Nan::ObjectWrap::Unwrap<FileWriter>(info.Holder());
  rocksdb::Status s = fw->_sstFileWriter->Add(key, value);
  if (!s.ok()) {
    Nan::ThrowError(s.getState());
    return;
  }
}

NAN_METHOD(FileWriter::Finish) {
  FileWriter* fw = Nan::ObjectWrap::Unwrap<FileWriter>(info.Holder());
  rocksdb::Status s = fw->_sstFileWriter->Finish();
  if (!s.ok()) {
    Nan::ThrowError(s.getState());
    return;
  }
}

NAN_METHOD(FileWriter::FileSize) {
  FileWriter* fw = Nan::ObjectWrap::Unwrap<FileWriter>(info.Holder());
  info.GetReturnValue().Set(Nan::New<v8::Number>(fw->_sstFileWriter->FileSize()));
}

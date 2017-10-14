#include <node.h>
#include <nan.h>
#include <iostream>
#include "DBNode.h"
#include "PutWorker.h"
#include "GetWorker.h"
#include "DeleteWorker.h"
#include "OptionsHelper.h"
#include "Iterator.h"
#include "Snapshot.h"
#include "Batch.h"
#include "FileWriter.h"
#include "rocksdb/db.h"
#include "Errors.h"
#include "CompactRangeWorker.h"
#include "MultiGetWorker.h"
using namespace std;

Nan::Persistent<v8::FunctionTemplate> dbnode_constructor;

DBNode::DBNode(rocksdb::Options options, string path, rocksdb::DB *db, std::vector<rocksdb::ColumnFamilyHandle*> *cfHandles) {
  _options = options;
  _path = path;
  _db = db;
  _cfHandles = cfHandles;
}

DBNode::~DBNode() {
  for (std::vector<rocksdb::ColumnFamilyHandle*>::iterator it = _cfHandles->begin() ; it != _cfHandles->end(); ++it) {
    _db->DestroyColumnFamilyHandle(*it);
  }
  delete _cfHandles;
  delete _db;
}

void DBNode::Init() {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(DBNode::New);

  tpl->SetClassName(Nan::New("DBNode").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  DBNode::InitBaseDBFunctions(tpl);
  dbnode_constructor.Reset(tpl);
}

void DBNode::InitBaseDBFunctions(v8::Local<v8::FunctionTemplate> tpl) {
  Nan::SetPrototypeMethod(tpl, "put", DBNode::Put);
  Nan::SetPrototypeMethod(tpl, "get", DBNode::Get);
  Nan::SetPrototypeMethod(tpl, "del", DBNode::Delete);
  Nan::SetPrototypeMethod(tpl, "newIterator", DBNode::NewIterator);
  Nan::SetPrototypeMethod(tpl, "getSnapshot", DBNode::GetSnapshot);
  Nan::SetPrototypeMethod(tpl, "releaseIterator", DBNode::ReleaseIterator);
  Nan::SetPrototypeMethod(tpl, "releaseSnapshot", DBNode::ReleaseSnapshot);
  Nan::SetPrototypeMethod(tpl, "getColumnFamilies", DBNode::GetColumnFamilies);
  Nan::SetPrototypeMethod(tpl, "createColumnFamily", DBNode::CreateColumnFamily);
  Nan::SetPrototypeMethod(tpl, "dropColumnFamily", DBNode::DropColumnFamily);
  Nan::SetPrototypeMethod(tpl, "batch", DBNode::Batch);
  Nan::SetPrototypeMethod(tpl, "write", DBNode::Write);
  Nan::SetPrototypeMethod(tpl, "close", DBNode::Close);
  Nan::SetPrototypeMethod(tpl, "getSstFileWriter", DBNode::GetSstFileWriter);
  Nan::SetPrototypeMethod(tpl, "ingestExternalFile", DBNode::IngestExternalFile);
  Nan::SetPrototypeMethod(tpl, "compactRange", DBNode::CompactRange);
  Nan::SetPrototypeMethod(tpl, "multiGet", DBNode::MultiGet);
}

NAN_METHOD(DBNode::New){
  Nan::HandleScope scope;
  if (info.IsConstructCall()) {
    if (info.Length() < 2) {
      Nan::ThrowTypeError(ERR_WRONG_ARGS);
      return;
    }

    v8::Local<v8::Object> opts = info[0].As<v8::Object>();
    string path = string(*Nan::Utf8String(info[1]));
    rocksdb::Options options;
    OptionsHelper::ProcessOpenOptions(opts, &options);

    // check for readOnly flag - this is rocksdb-node specific
    bool readOnly = false;
    v8::Local<v8::String> roKey = Nan::New("readOnly").ToLocalChecked();
    if (opts->Has(roKey)) {
      readOnly = opts->Get(roKey)->BooleanValue();
    }

    // support for Column Families
    std::vector<rocksdb::ColumnFamilyDescriptor> families;
    std::vector<std::string> familyNames;
    rocksdb::Status s = rocksdb::DB::ListColumnFamilies(options, path, &familyNames);

    for (std::vector<string>::iterator it = familyNames.begin() ; it != familyNames.end(); ++it) {
      families.push_back(rocksdb::ColumnFamilyDescriptor(*it, rocksdb::ColumnFamilyOptions()));
    }

    // if there's an error listing the column families, assume the database doesn't exist yet and just specify the default column family
    if (!s.ok()) {
      families.push_back(rocksdb::ColumnFamilyDescriptor(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
    }

    rocksdb::DB* db;
    std::vector<rocksdb::ColumnFamilyHandle*> *handles = new std::vector<rocksdb::ColumnFamilyHandle*>();
    s = readOnly ? rocksdb::DB::OpenForReadOnly(options, path, families, handles, &db) 
                                 : rocksdb::DB::Open(options, path, families, handles, &db);
    if (!s.ok()) {
      Nan::ThrowError(s.getState());
      return;
    }

    DBNode* dbNode = new DBNode(options, path, db, handles);
    dbNode->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    const unsigned argc = info.Length();
    v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
    for (unsigned int i=0; i<argc; i++) {
      argv[i] = info[i];
    }

    v8::Local<v8::FunctionTemplate> cons = Nan::New<v8::FunctionTemplate>(dbnode_constructor);
    v8::Local<v8::Object> instance = Nan::NewInstance(cons->GetFunction(), argc, argv).ToLocalChecked();

    delete [] argv;
    argv = NULL;
    info.GetReturnValue().Set(instance);
  }
}

void DBNode::NewInstance(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();

  const unsigned argc = info.Length();
  v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
  for (unsigned int i=0; i<argc; i++) {
    argv[i] = info[i];
  }

  v8::Local<v8::FunctionTemplate> cons = Nan::New<v8::FunctionTemplate>(dbnode_constructor);

  // TODO - seek advice here, exception propagation from the constructor here is non-trivial, there may be a better way of doing this
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
    info.GetReturnValue().Set(instance.ToLocalChecked());
  }
  delete [] argv;
  argv = NULL;
}

NAN_METHOD(DBNode::Put){
  int optsIndex = -1;
  int familyIndex = -1;
  int keyIndex = -1;
  int valueIndex = -1;
  int callbackIndex = -1;

  // 2 args, assume key value
  if (info.Length() == 2) {
    keyIndex = 0;
    valueIndex = 1;
  } else if (info.Length() == 3) {
    // 3 args is either (opts, key, value) or (key, value, callback), or (family, key, value)
    if (info[2]->IsFunction()) {
      keyIndex = 0;
      valueIndex = 1;
      callbackIndex = 2;
    } else if (info[0]->IsObject()) {
      optsIndex = 0;
      keyIndex = 1;
      valueIndex = 2;
    } else {
      familyIndex = 0;
      keyIndex = 1;
      valueIndex = 2;
    }
  } else if (info.Length() == 4) {
    // 4 args is either (opts, key, value, callback) or (family, key, value, callback)
    if (info[0]->IsObject()) {
      optsIndex = 0;
      familyIndex = 1;
      keyIndex = 2;
      valueIndex = 2;
    } else {
      familyIndex = 0;
      keyIndex = 1;
      valueIndex = 2;
      callbackIndex = 3;
    }
  } else if (info.Length() == 5) {
    optsIndex = 0;
    familyIndex = 1;
    keyIndex = 2;
    valueIndex = 3;
    callbackIndex = 4;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  Nan::Callback *callback = NULL;
  if (callbackIndex != -1) {
    callback = new Nan::Callback(info[callbackIndex].As<v8::Function>());
  }

  rocksdb::WriteOptions options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessWriteOptions(opts, &options);
  }

  // TODO - check for key undefined or null
  v8::Local<v8::Object> keyObj = info[keyIndex].As<v8::Object>();
  v8::Local<v8::Object> valueObj = info[valueIndex].As<v8::Object>();

  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());

  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(info[familyIndex]));
    columnFamily = dbNode->GetColumnFamily(family);
  } else {
    columnFamily = dbNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  if (columnFamily == NULL) {
    if (callback) {
      v8::Local<v8::Value> argv[1] = {Nan::New<v8::String>(ERR_CF_DOES_NOT_EXIST).ToLocalChecked()};
      callback->Call(1, argv);
    } else {
      Nan::ThrowError(ERR_CF_DOES_NOT_EXIST);
    }
    return;
  }

  rocksdb::Status s;


  if (callback) {
    PutWorker *pw = new PutWorker(callback, dbNode->_db, options, columnFamily, keyObj, valueObj);
    Nan::AsyncQueueWorker(pw);
  } else {
    rocksdb::Slice key = node::Buffer::HasInstance(info[keyIndex]) ? rocksdb::Slice(node::Buffer::Data(keyObj), node::Buffer::Length(keyObj))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(info[keyIndex])));

    rocksdb::Slice value = node::Buffer::HasInstance(info[valueIndex]) ? rocksdb::Slice(node::Buffer::Data(valueObj), node::Buffer::Length(valueObj))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(info[valueIndex])));
    s = dbNode->_db->Put(options, columnFamily, key, value);
    if (!s.ok()) {
      Nan::ThrowError(s.getState());
    }
  }
}

NAN_METHOD(DBNode::Get){
  v8::Isolate* isolate = info.GetIsolate();

  int optsIndex = -1;
  int familyIndex = -1;
  int keyIndex = -1;
  int callbackIndex = -1;

  // if only one arg, assume it's the key
  if (info.Length() == 1) {
    keyIndex = 0;
  } else if (info.Length() == 2) {
    // the two args are either (opts, key), (key, callback), or (family, key)
    if (info[1]->IsFunction()) {
      keyIndex = 0;
      callbackIndex = 1;
    } else if (info[0]->IsObject()) {
      optsIndex = 0;
      keyIndex = 1;
    } else {
      familyIndex = 0;
      keyIndex = 1;
    }
  } else if (info.Length() == 3) {
    // the three args are either (opts, key, callback), (family, key, callback), or (family, key, callback)
    if (info[0]->IsObject() && info[2]->IsFunction()) {
      optsIndex = 0;
      keyIndex = 1;
      callbackIndex = 2;
    } else if (info[0]->IsObject()) {
      optsIndex = 0;
      familyIndex = 1;
      keyIndex = 2;
    } else {
      familyIndex = 0;
      keyIndex = 1;
      callbackIndex = 2;
    }
  } else if (info.Length() == 4) {
    optsIndex = 0;
    familyIndex = 1;
    keyIndex = 2;
    callbackIndex = 3;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  rocksdb::ReadOptions options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessReadOptions(opts, &options);
  }

  // buffer is a special non-rocks option, it's specific to rocksdb-node
  bool buffer = false;
  if (optsIndex != -1) {
    v8::Local<v8::String> key = Nan::New("buffer").ToLocalChecked();  
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    if (!opts.IsEmpty() && opts->Has(key)) {
      buffer = opts->Get(key)->BooleanValue();
    }
  }

  // check if callback provided
  Nan::Callback *callback = NULL;
  if (callbackIndex != -1) {
    callback = new Nan::Callback(info[callbackIndex].As<v8::Function>());
  }

  v8::Local<v8::Object> keyObj = info[keyIndex].As<v8::Object>();
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());

  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(info[familyIndex]));
    columnFamily = dbNode->GetColumnFamily(family);
  } else {
    columnFamily = dbNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  if (columnFamily == NULL) {
    if (callback) {
      v8::Local<v8::Value> argv[1] = {Nan::New<v8::String>(ERR_CF_DOES_NOT_EXIST).ToLocalChecked()};
      callback->Call(1, argv);
    } else {
      Nan::ThrowError(ERR_CF_DOES_NOT_EXIST);
    }
    return;
  }

  if (callback) {
    Nan::AsyncQueueWorker(new GetWorker(callback, dbNode->_db, buffer, options, columnFamily, keyObj));
  } else {
    rocksdb::Slice key = node::Buffer::HasInstance(keyObj) ? rocksdb::Slice(node::Buffer::Data(keyObj), node::Buffer::Length(keyObj))
                                                           : rocksdb::Slice(string(*Nan::Utf8String(keyObj)));
    string value;
    rocksdb::Status s = dbNode->_db->Get(options, columnFamily, key, &value);

    if (s.IsNotFound()) {
      info.GetReturnValue().Set(Nan::Null());
      return;
    }

    if (!s.ok()) {
      Nan::ThrowError(s.getState());
      return;
    }

    if (buffer) {
      info.GetReturnValue().Set(Nan::CopyBuffer((char*)value.data(), value.size()).ToLocalChecked());
    } else {
      info.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, value.c_str()));
    }
  }
}

NAN_METHOD(DBNode::Delete){
  int optsIndex = -1;
  int familyIndex = -1;
  int keyIndex = -1;
  int callbackIndex = -1;

  // if only one arg, assume it's the key
  if (info.Length() == 1) {
    keyIndex = 0;
  } else if (info.Length() == 2) {
    // the two args are either (opts, key), (key, callback), or (family, key)
    if (info[1]->IsFunction()) {
      keyIndex = 0;
      callbackIndex = 1;
    } else if (info[0]->IsObject()) {
      optsIndex = 0;
      keyIndex = 1;
    } else {
      familyIndex = 0;
      keyIndex = 1;
    }
  } else if (info.Length() == 3) {
    // args are either (opts, key, callback), (opts, family, key), or (family, key, callback)
    if (info[0]->IsObject() && info[2]->IsFunction()) {
      optsIndex = 0;
      keyIndex = 1;
      callbackIndex = 2;
    } else if (info[0]->IsObject()) {
      optsIndex = 0;
      familyIndex = 1;
      keyIndex = 2;
    } else {
      familyIndex = 0;
      keyIndex = 1;
      callbackIndex = 2;
    }
  } else if (info.Length() == 4) {
    optsIndex = 0;
    familyIndex = 1;
    keyIndex = 2;
    callbackIndex = 3;
  }else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;    
  }

  Nan::Callback *callback = NULL;
  if (callbackIndex != -1) {
    callback = new Nan::Callback(info[callbackIndex].As<v8::Function>());
  }

  rocksdb::WriteOptions options;
  if (optsIndex != -1) {  
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessWriteOptions(opts, &options);
  }

  // TODO - check for key undefined or null
  rocksdb::Slice key = node::Buffer::HasInstance(info[keyIndex]) ? rocksdb::Slice(node::Buffer::Data(info[keyIndex]->ToObject()), node::Buffer::Length(info[keyIndex]->ToObject()))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(info[keyIndex])));

  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());

  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(info[familyIndex]));
    columnFamily = dbNode->GetColumnFamily(family);
  } else {
    columnFamily = dbNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  if (columnFamily == NULL) {
    if (callback) {
      v8::Local<v8::Value> argv[1] = {Nan::New<v8::String>(ERR_CF_DOES_NOT_EXIST).ToLocalChecked()};
      callback->Call(1, argv);
    } else {
      Nan::ThrowError(ERR_CF_DOES_NOT_EXIST);
    }
    return;
  }

  rocksdb::Status s;

  if (callback) {
    Nan::AsyncQueueWorker(new DeleteWorker(callback, dbNode->_db, columnFamily, key, options));
  } else {
    s = dbNode->_db->Delete(options, columnFamily, key);
    if (!s.ok()) {
      Nan::ThrowError(s.getState());
    }
  }
}

NAN_METHOD(DBNode::NewIterator){
  Iterator::NewInstance(info);
}

NAN_METHOD(DBNode::ReleaseIterator) {
  int iteratorIndex = -1;

  if (info.Length() == 1) {
    iteratorIndex = 0;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }
  Iterator* iter = Nan::ObjectWrap::Unwrap<Iterator>(info[iteratorIndex].As<v8::Object>());

  if (iter->_it) {
    delete iter->_it;
  }
}

NAN_METHOD(DBNode::GetSnapshot){
  Snapshot::NewInstance(info);
}

NAN_METHOD(DBNode::ReleaseSnapshot) {
  int optsIndex = -1;

  if (info.Length() == 1) {
    optsIndex = 0;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
  v8::Local<v8::String> snapshot = Nan::New("snapshot").ToLocalChecked();
  if (opts->Has(snapshot)) {
    Snapshot* ss = Nan::ObjectWrap::Unwrap<Snapshot>(opts->Get(snapshot).As<v8::Object>());
    dbNode->_db->ReleaseSnapshot(ss->_snapshot);
  }
}

NAN_METHOD(DBNode::GetColumnFamilies){
  std::vector<std::string> families;
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());

  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::Status s;

  v8::Local<v8::Array> arr = Nan::New<v8::Array>();
  for (std::vector<rocksdb::ColumnFamilyHandle*>::iterator it = dbNode->_cfHandles->begin() ; it != dbNode->_cfHandles->end(); ++it) {
    Nan::Set(arr, it - dbNode->_cfHandles->begin(), Nan::New((*it)->GetName()).ToLocalChecked());
  }

  info.GetReturnValue().Set(arr);
}

NAN_METHOD(DBNode::CreateColumnFamily) {
  int optsIndex = -1;
  int nameIndex = -1;

  // if only one arg, assume it's the name
  if (info.Length() == 1) {
    nameIndex = 0;
  } else if (info.Length() == 2) {
    // the two args (opts, name) 
    optsIndex = 0;
    nameIndex = 1;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  rocksdb::ColumnFamilyOptions options;
  // TODO - support https://github.com/facebook/rocksdb/blob/40af2381ecf95445675b7329c4a1c5f89f82d1ab/include/rocksdb/options.h#L81
  //if (optsIndex != -1) {  
    //v8::Local<v8::Object> opts = args[optsIndex].As<v8::Object>();
    //OptionsHelper::ProcessWriteOptions(opts, &options);
  //}

  string name = string(*Nan::Utf8String(info[nameIndex]));
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::Status s;

  rocksdb::ColumnFamilyHandle* cf;
  s = dbNode->_db->CreateColumnFamily(options, name, &cf);
  if (!s.ok()) {
    Nan::ThrowError(s.getState());
  }
  dbNode->_cfHandles->push_back(cf);
}

// Utility function for getting the ColumnFamilyHandle for a Column Family name
rocksdb::ColumnFamilyHandle* DBNode::GetColumnFamily(string family) {
  for (std::vector<rocksdb::ColumnFamilyHandle*>::iterator it = _cfHandles->begin() ; it != _cfHandles->end(); ++it) {
    if ((*it)->GetName() == family) return *it;
  }
  return NULL;
}

rocksdb::Status DBNode::DeleteColumnFamily(string family) {
  int index = -1;
  rocksdb::ColumnFamilyHandle *handle = NULL;

  for (std::vector<rocksdb::ColumnFamilyHandle*>::iterator it = _cfHandles->begin() ; it != _cfHandles->end(); ++it) {
    if ((*it)->GetName() == family) {
      index = it - _cfHandles->begin();
      handle = *it;
      break;
    }
  }

  if (handle) {
    rocksdb::Status s = _db->DestroyColumnFamilyHandle(handle);
    _cfHandles->erase(_cfHandles->begin() + index);
    return s;
  } else {
    rocksdb::Status s = rocksdb::Status::NotFound();
    return s;
  }
}

NAN_METHOD(DBNode::DropColumnFamily) {
  int nameIndex = 0;

  if (info.Length() != 1) {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  string name = string(*Nan::Utf8String(info[nameIndex]));
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::Status s;

  s = dbNode->DeleteColumnFamily(name);
  if (s.IsNotFound()) {
    Nan::ThrowError(ERR_CF_DOES_NOT_EXIST);
    return;
  }

  if (!s.ok()) {
    Nan::ThrowError(s.getState());
  }
}

// TODO - move this out of here
NAN_METHOD(DBNode::ListColumnFamilies) {
  int optsIndex = -1;
  int pathIndex = -1;

  // 2 args, assume (opts, path)
  if (info.Length() == 1) {
    pathIndex = 0;
  } else if (info.Length() == 2) {
    optsIndex = 0;
    pathIndex = 1;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  rocksdb::Options options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessOpenOptions(opts, &options);
  }

  string path = string(*Nan::Utf8String(info[pathIndex]));

  std::vector<std::string> familyNames;
  rocksdb::Status s = rocksdb::DB::ListColumnFamilies(options, path, &familyNames);

  if (!s.ok()) {
    Nan::ThrowError(s.getState());
    return;
  }

  v8::Local<v8::Array> arr = Nan::New<v8::Array>();
  for (std::vector<string>::iterator it = familyNames.begin() ; it != familyNames.end(); ++it) {
    Nan::Set(arr, it - familyNames.begin(), Nan::New(*it).ToLocalChecked());
  }

  info.GetReturnValue().Set(arr);
}

NAN_METHOD(DBNode::Batch) {
  Batch::NewInstance(info);
}

NAN_METHOD(DBNode::Write) {
  int optsIndex = -1;
  int batchIndex = -1;

  if (info.Length() == 1) {
    batchIndex = 0;
  } else if (info.Length() == 2) {
    optsIndex = 0;
    batchIndex = 1;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  rocksdb::WriteOptions options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessWriteOptions(opts, &options);
  }

  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  class Batch* batch = ObjectWrap::Unwrap<class Batch>(info[batchIndex].As<v8::Object>());
  rocksdb::Status s = dbNode->_db->Write(options, &batch->_batch);

  if (!s.ok()) {
    Nan::ThrowError(s.getState());
    return;
  }
}

NAN_METHOD(DBNode::Close) {
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  if (dbNode->_db) {
    delete dbNode->_db;
    dbNode->_db = NULL;
  }
}

NAN_METHOD(DBNode::GetSstFileWriter) {
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  FileWriter::NewInstance(info);
}

NAN_METHOD(DBNode::IngestExternalFile) {
  int pathIndex = -1;

  if (info.Length() == 1) {
    pathIndex = 0;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }
  string path = string(*Nan::Utf8String(info[pathIndex]));

  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::IngestExternalFileOptions ifo;
  rocksdb::Status s = dbNode->_db->IngestExternalFile({path}, ifo);
  if (!s.ok()) {
    Nan::ThrowError(s.getState());
    return;
  }
}

// TODO - move this out of here
NAN_METHOD(DBNode::DestroyDB) {
  int pathIndex = -1;
  int optsIndex = -1;

  if (info.Length() == 1) {
    pathIndex = 0;
  } else if (info.Length() == 2) {
    // expect (path, opts) as per rocks api
    pathIndex = 0;
    optsIndex = 1;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  rocksdb::Options options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessOpenOptions(opts, &options);
  }

  string path = string(*Nan::Utf8String(info[pathIndex]));
  rocksdb::Status s = rocksdb::DestroyDB(path, options);

  if (!s.ok()) {
    Nan::ThrowError(s.getState());
    return;
  }
}

// see https://github.com/facebook/rocksdb/wiki/Compaction
NAN_METHOD(DBNode::CompactRange) {
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());

  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  int optsIndex = -1;
  int familyIndex = -1;
  int fromIndex = -1;
  int toIndex = -1;
  int callbackIndex = -1;

  // if only one arg, assume it's either the options or the callback
  if (info.Length() == 1) {
    if (info[0]->IsFunction()) {
      callbackIndex = 0;
    } else if (info[0]->IsString()) {
      familyIndex = 0;
    } else {
      optsIndex = 0;
    }
  } else if (info.Length() == 2) {
    // assume two args are either (from, to) or (opts, callback) or (opts, columnFamily)
    if (info[0]->IsString() && info[1]->IsString()){
      fromIndex = 0;
      toIndex = 1;
    } else if (info[0]->IsObject() && info[1]->IsFunction()) {
      optsIndex = 0;
      callbackIndex = 1;
    } else {
      optsIndex = 0;
      familyIndex = 1;
    }
  } else if (info.Length() == 3) {
    // three args, assume either (columnFamily, from, to) or (opts, from, to) or (from, to, callback) or (opts, columnFamily, callback)
    if (info[0]->IsString() && info[1]->IsString() && info[2]->IsString()) {
      familyIndex = 0;
      fromIndex = 1;
      toIndex = 2;
    } else if (info[0]->IsObject() && info[1]->IsString() && info[2]->IsString()) {
      optsIndex = 0;
      fromIndex = 1;
      toIndex = 2;
    } else if (info[0]->IsString() && info[1]->IsString() && info[2]->IsFunction()) {
      fromIndex = 0;
      toIndex = 1;
      callbackIndex = 2;
    } else {
      optsIndex = 0;
      familyIndex = 1;
      callbackIndex = 2;
    }
  } else if (info.Length() == 4) {
    // four args, assume either (columnFamily, from, to, callback) or (opts, from, to, callback) or (opts, columnFamily, from, to)
    if (info[0]->IsString() && info[1]->IsString() && info[2]->IsString() && info[3]->IsFunction()) {
      familyIndex = 0;
      fromIndex = 1;
      toIndex = 2;
      callbackIndex = 3;
    } else if (info[0]->IsObject() && info[1]->IsString() && info[2]->IsString() && info[3]->IsFunction()) {
      optsIndex = 0;
      fromIndex = 1;
      toIndex = 2;
      callbackIndex = 3;
    } else {
      optsIndex = 0;
      familyIndex = 1;
      fromIndex = 2;
      toIndex = 3;
    }
  } else if (info.Length() == 5) {
    optsIndex = 0;
    familyIndex = 1;
    fromIndex = 2;
    toIndex = 3;
    callbackIndex = 4;
  } else {
    if (info.Length() != 0) {
      Nan::ThrowTypeError(ERR_WRONG_ARGS);
      return;
    }
  }

  Nan::Callback *callback = NULL;
  if (callbackIndex != -1) {
    callback = new Nan::Callback(info[callbackIndex].As<v8::Function>());
  }

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(info[familyIndex]));
    columnFamily = dbNode->GetColumnFamily(family);
  } else {
    columnFamily = dbNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  if (columnFamily == NULL) {
    if (callback) {
      v8::Local<v8::Value> argv[1] = {Nan::New<v8::String>(ERR_CF_DOES_NOT_EXIST).ToLocalChecked()};
      callback->Call(1, argv);
    } else {
      Nan::ThrowError(ERR_CF_DOES_NOT_EXIST);
    }
    return;
  }

  rocksdb::CompactRangeOptions options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessCompactRangeOptions(opts, &options);
  } else {
    options = rocksdb::CompactRangeOptions();
  }

  rocksdb::Status s;

  v8::Local<v8::Object> fromObj = fromIndex == -1 ? Nan::New<v8::Object>() : info[fromIndex].As<v8::Object>();
  v8::Local<v8::Object> toObj = toIndex == -1 ? Nan::New<v8::Object>() : info[toIndex].As<v8::Object>();

  if (callback) {
    CompactRangeWorker *crw = new CompactRangeWorker(callback, dbNode->_db, options, NULL, fromObj, toObj);
    Nan::AsyncQueueWorker(crw);
  } else {
    rocksdb::Slice* from = NULL;
    if (fromIndex != -1) {
      from = node::Buffer::HasInstance(fromObj) ? new rocksdb::Slice(node::Buffer::Data(fromObj), node::Buffer::Length(fromObj))
        : new rocksdb::Slice(string(*Nan::Utf8String(fromObj)));
    }

    rocksdb::Slice* to = NULL;
    if (toIndex != -1) {
      to = node::Buffer::HasInstance(toObj) ? new rocksdb::Slice(node::Buffer::Data(toObj), node::Buffer::Length(toObj))
        : new rocksdb::Slice(string(*Nan::Utf8String(toObj)));
    }

    s = dbNode->_db->CompactRange(options, from, to);
    if (!s.ok()) {
      Nan::ThrowError(s.getState());
    }

    if (from) delete from;
    if (to) delete to;
  }
}

NAN_METHOD(DBNode::MultiGet) {
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(info.Holder());

  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  int optsIndex = -1;
  int familyIndex = -1;
  int keysIndex = -1;
  int callbackIndex = -1;

  if (info.Length() == 1) {
    // one arg assume array of keys
    keysIndex = 0;
  } else if (info.Length() == 2) {
    // assume two args could be (opts, keys), (keys, callback) or (col, keys)
    if (info[0]->IsObject() && info[1]->IsArray()) {
      optsIndex = 0;
      callbackIndex = 1;
    } else if(info[0]->IsString() && info[1]->IsArray()) {
      familyIndex = 0;
      keysIndex = 1;
    } else {
      keysIndex = 0;
      callbackIndex = 1;
    }
  } else if (info.Length() == 3) {
    // three args, assume either (columnFamily, keys, callback) or (opts, columnFamily, keys) or (opts, keys, callback)
    if (info[0]->IsString() && info[1]->IsArray() && info[2]->IsFunction()) {
      familyIndex = 0;
      keysIndex = 1;
      callbackIndex = 2;
    } else if (info[0]->IsObject() && info[1]->IsString() && info[2]->IsArray()) {
      optsIndex = 0;
      familyIndex = 1;
      keysIndex = 2;
    } else {
      optsIndex = 0;
      keysIndex = 1;
      callbackIndex = 2;
    }
  } else if (info.Length() == 4) {
    // four args, assume (opts, columnFamily, keys, callback) 
    optsIndex = 0;
    familyIndex = 1;
    keysIndex = 2;
    callbackIndex = 3;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  Nan::Callback *callback = NULL;
  if (callbackIndex != -1) {
    callback = new Nan::Callback(info[callbackIndex].As<v8::Function>());
  }

  rocksdb::ReadOptions options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessReadOptions(opts, &options);
  }

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(info[familyIndex]));
    columnFamily = dbNode->GetColumnFamily(family);
  } else {
    columnFamily = dbNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  if (columnFamily == NULL) {
    if (callback) {
      v8::Local<v8::Value> argv[1] = {Nan::New<v8::String>(ERR_CF_DOES_NOT_EXIST).ToLocalChecked()};
      callback->Call(1, argv);
    } else {
      Nan::ThrowError(ERR_CF_DOES_NOT_EXIST);
    }
    return;
  }

  v8::Local<v8::Array> keysArray = info[keysIndex].As<v8::Array>();
  if (callback) {
    Nan::AsyncQueueWorker(new MultiGetWorker(callback, dbNode->_db, options, columnFamily, keysArray));
  } else {

    // TODO - handle buffer keys, maybe pass in opts array?
    std::vector<rocksdb::Slice> keys;
    for (unsigned int i = 0; i < keysArray->Length(); i++) {
      std::string *str = new std::string(*Nan::Utf8String(keysArray->Get(i)));
      rocksdb::Slice s = rocksdb::Slice(*str);
      keys.push_back(s);
    }

    // Currently just one column family passed, i.e. multigets only supported in one column
    // Change here in future to support passing array of cf handles
    std::vector<rocksdb::ColumnFamilyHandle*>families;
    for (unsigned int i = 0; i < keysArray->Length(); i++) {
      families.push_back(columnFamily);
    }

    std::vector<rocksdb::Status> statuss;
    std::vector<std::string> values;

    statuss = dbNode->_db->MultiGet(options, families, keys, &values);

    v8::Local<v8::Array> arr = Nan::New<v8::Array>();
    for(unsigned i = 0; i != values.size(); i++) {
      rocksdb::Status s = statuss[i];
      if (s.ok()) {
        std::string val = values[i];
        Nan::Set(arr, i, Nan::New(val).ToLocalChecked());
      } else if (s.IsNotFound()) {
        Nan::Set(arr, i, Nan::Null());
      } else {
        // TODO - verify this is correct...]
        Nan::ThrowError(s.getState());
      }
    }

    info.GetReturnValue().Set(arr);
  }

  // TODO - free keys?!
  // TODO - buffer k/v
  // TODO - test all arg combos
  // TODO - document

/*
  rocksdb::CompactRangeOptions options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = info[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessCompactRangeOptions(opts, &options);
  } else {
    options = rocksdb::CompactRangeOptions();
  }

  rocksdb::Status s;

  v8::Local<v8::Object> fromObj = fromIndex == -1 ? Nan::New<v8::Object>() : info[fromIndex].As<v8::Object>();
  v8::Local<v8::Object> toObj = toIndex == -1 ? Nan::New<v8::Object>() : info[toIndex].As<v8::Object>();

  if (callback) {
    CompactRangeWorker *crw = new CompactRangeWorker(callback, dbNode->_db, options, NULL, fromObj, toObj);
    Nan::AsyncQueueWorker(crw);
  } else {
    rocksdb::Slice* from = NULL;
    if (fromIndex != -1) {
      from = node::Buffer::HasInstance(fromObj) ? new rocksdb::Slice(node::Buffer::Data(fromObj), node::Buffer::Length(fromObj))
        : new rocksdb::Slice(string(*Nan::Utf8String(fromObj)));
    }

    rocksdb::Slice* to = NULL;
    if (toIndex != -1) {
      to = node::Buffer::HasInstance(toObj) ? new rocksdb::Slice(node::Buffer::Data(toObj), node::Buffer::Length(toObj))
        : new rocksdb::Slice(string(*Nan::Utf8String(toObj)));
    }

    s = dbNode->_db->CompactRange(options, from, to);
    if (!s.ok()) {
      Nan::ThrowError(s.getState());
    }

    if (from) delete from;
    if (to) delete to;
  }
*/
}

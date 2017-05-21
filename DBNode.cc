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
using namespace std;

v8::Persistent<v8::Function> DBNode::constructor;

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

void DBNode::Init(v8::Local<v8::Object> exports) {
  v8::Isolate* isolate = exports->GetIsolate();
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
  tpl->SetClassName(v8::String::NewFromUtf8(isolate, "DBNode"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tpl, "put", DBNode::Put);
  NODE_SET_PROTOTYPE_METHOD(tpl, "get", DBNode::Get);
  NODE_SET_PROTOTYPE_METHOD(tpl, "del", DBNode::Delete);
  NODE_SET_PROTOTYPE_METHOD(tpl, "newIterator", DBNode::NewIterator);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getSnapshot", DBNode::GetSnapshot);
  NODE_SET_PROTOTYPE_METHOD(tpl, "releaseIterator", DBNode::ReleaseIterator);
  NODE_SET_PROTOTYPE_METHOD(tpl, "releaseSnapshot", DBNode::ReleaseSnapshot);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getColumnFamilies", DBNode::GetColumnFamilies);
  NODE_SET_PROTOTYPE_METHOD(tpl, "createColumnFamily", DBNode::CreateColumnFamily);
  NODE_SET_PROTOTYPE_METHOD(tpl, "dropColumnFamily", DBNode::DropColumnFamily);
  NODE_SET_PROTOTYPE_METHOD(tpl, "batch", DBNode::Batch);
  NODE_SET_PROTOTYPE_METHOD(tpl, "write", DBNode::Write);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", DBNode::Close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getSstFileWriter", DBNode::GetSstFileWriter);
  NODE_SET_PROTOTYPE_METHOD(tpl, "ingestExternalFile", DBNode::IngestExternalFile);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(v8::String::NewFromUtf8(isolate, "DBNode"), tpl->GetFunction());
} 

void DBNode::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Nan::HandleScope scope;
  v8::Isolate* isolate = args.GetIsolate();
  if (args.IsConstructCall()) {
    if (args.Length() < 2) {
      Nan::ThrowTypeError(ERR_WRONG_ARGS);
      return;
    }

    v8::Local<v8::Object> opts = args[0].As<v8::Object>();
    string path = string(*Nan::Utf8String(args[1]));
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
    dbNode->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    const unsigned argc = args.Length();
    v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
    for (unsigned int i=0; i<argc; i++) {
      argv[i] = args[i];
    }

    v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Object> instance = cons->NewInstance(context, argc, argv).ToLocalChecked();

    delete [] argv;
    argv = NULL;
    args.GetReturnValue().Set(instance);
  }
}

void DBNode::NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  const unsigned argc = args.Length();
  v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
  for (unsigned int i=0; i<argc; i++) {
    argv[i] = args[i];
  }

  v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  // TODO - seek advice here, exception propagation from the constructor here is non-trivial, there may be a better way of doing this
  v8::MaybeLocal<v8::Object> instance;
  v8::Local<v8::Value> err;
  bool hasException = false;
  {
    Nan::TryCatch tc;
    instance = cons->NewInstance(context, argc, argv);
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

void DBNode::Put(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int optsIndex = -1;
  int familyIndex = -1;
  int keyIndex = -1;
  int valueIndex = -1;
  int callbackIndex = -1;

  // 2 args, assume key value
  if (args.Length() == 2) {
    keyIndex = 0;
    valueIndex = 1;
  } else if (args.Length() == 3) {
    // 3 args is either (opts, key, value) or (key, value, callback), or (family, key, value)
    if (args[2]->IsFunction()) {
      keyIndex = 0;
      valueIndex = 1;
      callbackIndex = 2;
    } else if (args[0]->IsObject()) {
      optsIndex = 0;
      keyIndex = 1;
      valueIndex = 2;
    } else {
      familyIndex = 0;
      keyIndex = 1;
      valueIndex = 2;
    }
  } else if (args.Length() == 4) {
    // 4 args is either (opts, key, value, callback) or (family, key, value, callback)
    if (args[0]->IsObject()) {
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
  } else if (args.Length() == 5) {
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
    callback = new Nan::Callback(args[callbackIndex].As<v8::Function>());
  }

  rocksdb::WriteOptions options;
  if (optsIndex != -1) {  
    v8::Local<v8::Object> opts = args[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessWriteOptions(opts, &options);
  }

  // TODO - check for key undefined or null
  v8::Local<v8::Object> keyObj = args[keyIndex].As<v8::Object>();
  v8::Local<v8::Object> valueObj = args[valueIndex].As<v8::Object>();
  
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());

  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(args[familyIndex]));
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
    rocksdb::Slice key = node::Buffer::HasInstance(args[keyIndex]) ? rocksdb::Slice(node::Buffer::Data(keyObj), node::Buffer::Length(keyObj))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(args[keyIndex])));

    rocksdb::Slice value = node::Buffer::HasInstance(args[valueIndex]) ? rocksdb::Slice(node::Buffer::Data(valueObj), node::Buffer::Length(valueObj))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(args[valueIndex])));
    s = dbNode->_db->Put(options, columnFamily, key, value);
    if (!s.ok()) {
      Nan::ThrowError(s.getState());
    }
  }
}

void DBNode::Get(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  int optsIndex = -1;
  int familyIndex = -1;
  int keyIndex = -1;
  int callbackIndex = -1;

  // if only one arg, assume it's the key
  if (args.Length() == 1) {
    keyIndex = 0;
  } else if (args.Length() == 2) {
    // the two args are either (opts, key), (key, callback), or (family, key)
    if (args[1]->IsFunction()) {
      keyIndex = 0;
      callbackIndex = 1;
    } else if (args[0]->IsObject()) {
      optsIndex = 0;
      keyIndex = 1;
    } else {
      familyIndex = 0;
      keyIndex = 1;
    }
  } else if (args.Length() == 3) {
    // the three args are either (opts, key, callback), (family, key, callback), or (family, key, callback)
    if (args[0]->IsObject() && args[2]->IsFunction()) {
      optsIndex = 0;
      keyIndex = 1;
      callbackIndex = 2;
    } else if (args[0]->IsObject()) {
      optsIndex = 0;
      familyIndex = 1;
      keyIndex = 2;
    } else {
      familyIndex = 0;
      keyIndex = 1;
      callbackIndex = 2;
    }
  } else if (args.Length() == 4) {
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
    v8::Local<v8::Object> opts = args[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessReadOptions(opts, &options);
  }

  // buffer is a special non-rocks option, it's specific to rocksdb-node
  bool buffer = false;
  if (optsIndex != -1) {
    v8::Local<v8::String> key = Nan::New("buffer").ToLocalChecked();  
    v8::Local<v8::Object> opts = args[optsIndex].As<v8::Object>();
    if (!opts.IsEmpty() && opts->Has(key)) {
      buffer = opts->Get(key)->BooleanValue();
    }
  }

  // check if callback provided
  Nan::Callback *callback = NULL;
  if (callbackIndex != -1) {
    callback = new Nan::Callback(args[callbackIndex].As<v8::Function>());
  }

  v8::Local<v8::Object> keyObj = args[keyIndex].As<v8::Object>();
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());

  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(args[familyIndex]));
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
      args.GetReturnValue().Set(Nan::Null());
      return;
    }

    if (!s.ok()) {
      Nan::ThrowError(s.getState());
      return;
    }

    if (buffer) {
      args.GetReturnValue().Set(Nan::CopyBuffer((char*)value.data(), value.size()).ToLocalChecked());
    } else {
      args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, value.c_str()));
    }
  }
}

void DBNode::Delete(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int optsIndex = -1;
  int familyIndex = -1;
  int keyIndex = -1;
  int callbackIndex = -1;

  // if only one arg, assume it's the key
  if (args.Length() == 1) {
    keyIndex = 0;
  } else if (args.Length() == 2) {
    // the two args are either (opts, key), (key, callback), or (family, key)
    if (args[1]->IsFunction()) {
      keyIndex = 0;
      callbackIndex = 1;
    } else if (args[0]->IsObject()) {
      optsIndex = 0;
      keyIndex = 1;
    } else {
      familyIndex = 0;
      keyIndex = 1;
    }
  } else if (args.Length() == 3) {
    // args are either (opts, key, callback), (opts, family, key), or (family, key, callback)
    if (args[0]->IsObject() && args[2]->IsFunction()) {
      optsIndex = 0;
      keyIndex = 1;
      callbackIndex = 2;
    } else if (args[0]->IsObject()) {
      optsIndex = 0;
      familyIndex = 1;
      keyIndex = 2;
    } else {
      familyIndex = 0;
      keyIndex = 1;
      callbackIndex = 2;
    }
  } else if (args.Length() == 4) {
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
    callback = new Nan::Callback(args[callbackIndex].As<v8::Function>());
  }

  rocksdb::WriteOptions options;
  if (optsIndex != -1) {  
    v8::Local<v8::Object> opts = args[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessWriteOptions(opts, &options);
  }

  // TODO - check for key undefined or null
  rocksdb::Slice key = node::Buffer::HasInstance(args[keyIndex]) ? rocksdb::Slice(node::Buffer::Data(args[keyIndex]->ToObject()), node::Buffer::Length(args[keyIndex]->ToObject()))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(args[keyIndex])));

  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());

  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(args[familyIndex]));
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

void DBNode::NewIterator(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Iterator::NewInstance(args);
}

void DBNode::ReleaseIterator(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int iteratorIndex = -1;

  if (args.Length() == 1) {
    iteratorIndex = 0;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }
  Iterator* iter = Nan::ObjectWrap::Unwrap<Iterator>(args[iteratorIndex].As<v8::Object>());

  if (iter->_it) {
    delete iter->_it;
  }
}

void DBNode::GetSnapshot(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Snapshot::NewInstance(args);
}

void DBNode::ReleaseSnapshot(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int optsIndex = -1;

  if (args.Length() == 1) {
    optsIndex = 0;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  v8::Local<v8::Object> opts = args[optsIndex].As<v8::Object>();
  v8::Local<v8::String> snapshot = Nan::New("snapshot").ToLocalChecked();
  if (opts->Has(snapshot)) {
    Snapshot* ss = Nan::ObjectWrap::Unwrap<Snapshot>(opts->Get(snapshot).As<v8::Object>());
    dbNode->_db->ReleaseSnapshot(ss->_snapshot);
  }
}

void DBNode::GetColumnFamilies(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::vector<std::string> families;
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());

  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  rocksdb::Status s;

  v8::Local<v8::Array> arr = Nan::New<v8::Array>();
  for (std::vector<rocksdb::ColumnFamilyHandle*>::iterator it = dbNode->_cfHandles->begin() ; it != dbNode->_cfHandles->end(); ++it) {
    Nan::Set(arr, it - dbNode->_cfHandles->begin(), Nan::New((*it)->GetName()).ToLocalChecked());
  }

  args.GetReturnValue().Set(arr);
}


void DBNode::CreateColumnFamily(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int optsIndex = -1;
  int nameIndex = -1;
  
  // if only one arg, assume it's the name
  if (args.Length() == 1) {
    nameIndex = 0;
  } else if (args.Length() == 2) {
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

  string name = string(*Nan::Utf8String(args[nameIndex]));    
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());
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

void DBNode::DropColumnFamily(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int nameIndex = 0;

  if (args.Length() != 1) {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  string name = string(*Nan::Utf8String(args[nameIndex]));
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());
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
void DBNode::ListColumnFamilies(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int optsIndex = -1;
  int pathIndex = -1;

  // 2 args, assume (opts, path)
  if (args.Length() == 1) {
    pathIndex = 0;
  } else if (args.Length() == 2) {
    optsIndex = 0;
    pathIndex = 1;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  rocksdb::Options options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = args[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessOpenOptions(opts, &options);
  }

  string path = string(*Nan::Utf8String(args[pathIndex]));

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

  args.GetReturnValue().Set(arr);
}

void DBNode::Batch(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Batch::NewInstance(args);
}

void DBNode::Write(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int optsIndex = -1;
  int batchIndex = -1;

  if (args.Length() == 1) {
    batchIndex = 0;
  } else if (args.Length() == 2) {
    optsIndex = 0;
    batchIndex = 1;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  rocksdb::WriteOptions options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = args[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessWriteOptions(opts, &options);
  }

  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  class Batch* batch = ObjectWrap::Unwrap<class Batch>(args[batchIndex].As<v8::Object>());
  rocksdb::Status s = dbNode->_db->Write(options, &batch->_batch);

  if (!s.ok()) {
    Nan::ThrowError(s.getState());
    return;
  }
}

void DBNode::Close(const v8::FunctionCallbackInfo<v8::Value>& args) {
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  if (dbNode->_db) {
    delete dbNode->_db;
    dbNode->_db = NULL;
  }
}

void DBNode::GetSstFileWriter(const v8::FunctionCallbackInfo<v8::Value>& args) {
  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());
  if (!dbNode->_db) {
    Nan::ThrowError(ERR_DB_NOT_OPEN);
    return;
  }

  FileWriter::NewInstance(args);
}

void DBNode::IngestExternalFile(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int pathIndex = -1;

  if (args.Length() == 1) {
    pathIndex = 0;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }
  string path = string(*Nan::Utf8String(args[pathIndex]));

  DBNode* dbNode = ObjectWrap::Unwrap<DBNode>(args.Holder());
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
void DBNode::DestroyDB(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int pathIndex = -1;
  int optsIndex = -1;

  if (args.Length() == 1) {
    pathIndex = 0;
  } else if (args.Length() == 2) {
    // expect (path, opts) as per rocks api
    pathIndex = 0;
    optsIndex = 1;
  } else {
    Nan::ThrowTypeError(ERR_WRONG_ARGS);
    return;
  }

  rocksdb::Options options;
  if (optsIndex != -1) {
    v8::Local<v8::Object> opts = args[optsIndex].As<v8::Object>();
    OptionsHelper::ProcessOpenOptions(opts, &options);
  }

  string path = string(*Nan::Utf8String(args[pathIndex]));
  rocksdb::Status s = rocksdb::DestroyDB(path, options);

  if (!s.ok()) {
    Nan::ThrowError(s.getState());
    return;
  }
}

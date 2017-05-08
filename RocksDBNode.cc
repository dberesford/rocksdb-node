#include <node.h>
#include <nan.h>
#include <iostream>
#include "RocksDBNode.h"  
#include "PutWorker.h"
#include "GetWorker.h"
#include "DeleteWorker.h"
#include "OptionsHelper.h"
#include "Iterator.h"
#include "rocksdb/db.h"
using namespace std;

v8::Persistent<v8::Function> RocksDBNode::constructor;

RocksDBNode::RocksDBNode(rocksdb::Options options, string path, rocksdb::DB *db, std::vector<rocksdb::ColumnFamilyHandle*> *cfHandles) {
  _options = options;
  _path = path;
  _db = db;
  _cfHandles = cfHandles;
}

RocksDBNode::~RocksDBNode() {
  for (std::vector<rocksdb::ColumnFamilyHandle*>::iterator it = _cfHandles->begin() ; it != _cfHandles->end(); ++it) {
    _db->DestroyColumnFamilyHandle(*it);
  }
  delete _cfHandles;
  delete _db;
}

void RocksDBNode::Init(v8::Local<v8::Object> exports) {
  v8::Isolate* isolate = exports->GetIsolate();
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
  tpl->SetClassName(v8::String::NewFromUtf8(isolate, "RocksDBNode"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tpl, "put", RocksDBNode::Put);
  NODE_SET_PROTOTYPE_METHOD(tpl, "get", RocksDBNode::Get);
  NODE_SET_PROTOTYPE_METHOD(tpl, "del", RocksDBNode::Delete);
  NODE_SET_PROTOTYPE_METHOD(tpl, "newIterator", RocksDBNode::NewIterator);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getColumnFamilies", RocksDBNode::GetColumnFamilies);
  NODE_SET_PROTOTYPE_METHOD(tpl, "createColumnFamily", RocksDBNode::CreateColumnFamily);
  NODE_SET_PROTOTYPE_METHOD(tpl, "dropColumnFamily", RocksDBNode::DropColumnFamily);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(v8::String::NewFromUtf8(isolate, "RocksDBNode"), tpl->GetFunction());
} 

void RocksDBNode::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Nan::HandleScope scope;
  v8::Isolate* isolate = args.GetIsolate();
  if (args.IsConstructCall()) {
    if (args.Length() < 2) {
      Nan::ThrowTypeError("Wrong number of arguments");
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
  
    RocksDBNode* rocksDBNode = new RocksDBNode(options, path, db, handles);
    rocksDBNode->Wrap(args.This());
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

void RocksDBNode::NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args) {
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

void RocksDBNode::Put(const v8::FunctionCallbackInfo<v8::Value>& args) {
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
    Nan::ThrowTypeError("Wrong number of arguments");
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
  
  RocksDBNode* rocksDBNode = ObjectWrap::Unwrap<RocksDBNode>(args.Holder());

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(args[familyIndex]));
    columnFamily = rocksDBNode->GetColumnFamily(family);
  } else {
    columnFamily = rocksDBNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  if (columnFamily == NULL) {
    if (callback) {
      v8::Local<v8::Value> argv[1] = {Nan::New<v8::String>("Column Family does not exist").ToLocalChecked()};
      callback->Call(1, argv);
    } else {
      Nan::ThrowError("Column Family does not exist");
    }
    return;
  }

  rocksdb::Status s;

  if (callback) {  
    PutWorker *pw = new PutWorker(callback, rocksDBNode->_db, options, columnFamily, keyObj, valueObj);
    Nan::AsyncQueueWorker(pw);
  } else {
    rocksdb::Slice key = node::Buffer::HasInstance(args[keyIndex]) ? rocksdb::Slice(node::Buffer::Data(keyObj), node::Buffer::Length(keyObj))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(args[keyIndex])));

    rocksdb::Slice value = node::Buffer::HasInstance(args[valueIndex]) ? rocksdb::Slice(node::Buffer::Data(valueObj), node::Buffer::Length(valueObj))
                                                            : rocksdb::Slice(string(*Nan::Utf8String(args[valueIndex])));
    s = rocksDBNode->_db->Put(options, columnFamily, key, value);
    if (!s.ok()) {
      Nan::ThrowError(s.getState());
    }
  }
}

void RocksDBNode::Get(const v8::FunctionCallbackInfo<v8::Value>& args) {
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
    Nan::ThrowTypeError("Wrong number of arguments");
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
  RocksDBNode* rocksDBNode = ObjectWrap::Unwrap<RocksDBNode>(args.Holder());

  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(args[familyIndex]));
    columnFamily = rocksDBNode->GetColumnFamily(family);
  } else {
    columnFamily = rocksDBNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  if (columnFamily == NULL) {
    if (callback) {
      v8::Local<v8::Value> argv[1] = {Nan::New<v8::String>("Column Family does not exist").ToLocalChecked()};
      callback->Call(1, argv);
    } else {
      Nan::ThrowError("Column Family does not exist");
    }
    return;
  }
  
  if (callback) {
    Nan::AsyncQueueWorker(new GetWorker(callback, rocksDBNode->_db, buffer, options, columnFamily, keyObj));
  } else {
    rocksdb::Slice key = node::Buffer::HasInstance(keyObj) ? rocksdb::Slice(node::Buffer::Data(keyObj), node::Buffer::Length(keyObj))
                                                           : rocksdb::Slice(string(*Nan::Utf8String(keyObj)));
    string value;
    rocksdb::Status s = rocksDBNode->_db->Get(options, columnFamily, key, &value);
  
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

void RocksDBNode::Delete(const v8::FunctionCallbackInfo<v8::Value>& args) {
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
    Nan::ThrowTypeError("Wrong number of arguments");
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
  
  RocksDBNode* rocksDBNode = ObjectWrap::Unwrap<RocksDBNode>(args.Holder());
  
  rocksdb::ColumnFamilyHandle *columnFamily = NULL;
  if (familyIndex != -1) {
    string family = string(*Nan::Utf8String(args[familyIndex]));
    columnFamily = rocksDBNode->GetColumnFamily(family);
  } else {
    columnFamily = rocksDBNode->GetColumnFamily(rocksdb::kDefaultColumnFamilyName);
  }

  if (columnFamily == NULL) {
    if (callback) {
      v8::Local<v8::Value> argv[1] = {Nan::New<v8::String>("Column Family does not exist").ToLocalChecked()};
      callback->Call(1, argv);
    } else {
      Nan::ThrowError("Column Family does not exist");
    }
    return;
  }
  
  rocksdb::Status s;

  if (callback) {
    Nan::AsyncQueueWorker(new DeleteWorker(callback, rocksDBNode->_db, columnFamily, key, options));
  } else {
    s = rocksDBNode->_db->Delete(options, columnFamily, key);
    if (!s.ok()) {
      Nan::ThrowError(s.getState());
    }
  }
}

void RocksDBNode::NewIterator(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Iterator::NewInstance(args);
}

void RocksDBNode::GetColumnFamilies(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::vector<std::string> families;
  RocksDBNode* rocksDBNode = ObjectWrap::Unwrap<RocksDBNode>(args.Holder());
  rocksdb::Status s;

  v8::Local<v8::Array> arr = Nan::New<v8::Array>();
  for (std::vector<rocksdb::ColumnFamilyHandle*>::iterator it = rocksDBNode->_cfHandles->begin() ; it != rocksDBNode->_cfHandles->end(); ++it) {
    Nan::Set(arr, it - rocksDBNode->_cfHandles->begin(), Nan::New((*it)->GetName()).ToLocalChecked());
  }
  
  args.GetReturnValue().Set(arr);
}


void RocksDBNode::CreateColumnFamily(const v8::FunctionCallbackInfo<v8::Value>& args) {
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
    Nan::ThrowTypeError("Wrong number of arguments");
    return;    
  }

  rocksdb::ColumnFamilyOptions options;
  // TODO - support https://github.com/facebook/rocksdb/blob/40af2381ecf95445675b7329c4a1c5f89f82d1ab/include/rocksdb/options.h#L81
  //if (optsIndex != -1) {  
    //v8::Local<v8::Object> opts = args[optsIndex].As<v8::Object>();
    //OptionsHelper::ProcessWriteOptions(opts, &options);
  //}

  string name = string(*Nan::Utf8String(args[nameIndex]));    
  RocksDBNode* rocksDBNode = ObjectWrap::Unwrap<RocksDBNode>(args.Holder());
  rocksdb::Status s;

  rocksdb::ColumnFamilyHandle* cf;
  s = rocksDBNode->_db->CreateColumnFamily(options, name, &cf);
  if (!s.ok()) {
    Nan::ThrowError(s.getState());
  }
  rocksDBNode->_cfHandles->push_back(cf);  
}

// Utility function for getting the ColumnFamilyHandle for a Column Family name
rocksdb::ColumnFamilyHandle* RocksDBNode::GetColumnFamily(string family) {
  for (std::vector<rocksdb::ColumnFamilyHandle*>::iterator it = _cfHandles->begin() ; it != _cfHandles->end(); ++it) {    
    if ((*it)->GetName() == family) return *it;
  }
  return NULL;
}

rocksdb::Status RocksDBNode::DeleteColumnFamily(string family) {
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

void RocksDBNode::DropColumnFamily(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int nameIndex = 0;
  
  if (args.Length() != 1) {
    Nan::ThrowTypeError("Wrong number of arguments");
    return;    
  }

  string name = string(*Nan::Utf8String(args[nameIndex]));    
  RocksDBNode* rocksDBNode = ObjectWrap::Unwrap<RocksDBNode>(args.Holder());
  rocksdb::Status s;

  s = rocksDBNode->DeleteColumnFamily(name);
  if (s.IsNotFound()) {
    Nan::ThrowError("Column Family not found");
    return;
  }
  
  if (!s.ok()) {
    Nan::ThrowError(s.getState());
  }    
}

// TODO - move this out of here
void RocksDBNode::ListColumnFamilies(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int optsIndex = -1;
  int pathIndex = -1;
  
  // 2 args, assume (opts, path)
  if (args.Length() == 1) {
    pathIndex = 0;
  } else if (args.Length() == 2) {
    optsIndex = 0;
    pathIndex = 1;
  } else {
    Nan::ThrowTypeError("Wrong number of arguments");
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


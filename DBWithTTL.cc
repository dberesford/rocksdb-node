#include <node.h>
#include <nan.h>
#include <iostream>
#include "rocksdb/db.h"
#include "rocksdb/utilities/db_ttl.h"
#include "Errors.h"
#include "DBWithTTL.h"
#include "OptionsHelper.h"

using namespace std;

Nan::Persistent<v8::FunctionTemplate> dbwithttl_constructor;

DBWithTTL::DBWithTTL(rocksdb::Options options, string path, rocksdb::DBWithTTL *db, std::vector<rocksdb::ColumnFamilyHandle*> *cfHandles) : DBNode(options, path, db, cfHandles){
}

DBWithTTL::~DBWithTTL() {
}

void DBWithTTL::Init() {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(DBWithTTL::New);

  tpl->SetClassName(Nan::New("DBWithTTL").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  DBNode::InitBaseDBFunctions(tpl);
  dbwithttl_constructor.Reset(tpl);
}

NAN_METHOD(DBWithTTL::New){
  Nan::HandleScope scope;
  if (info.IsConstructCall()) {
    if (info.Length() < 3) {
      Nan::ThrowTypeError(ERR_WRONG_ARGS);
      return;
    }

    v8::Local<v8::Object> opts = info[0].As<v8::Object>();
    string path = string(*Nan::Utf8String(info[1]));
    int ttl = Nan::To<int>(info[2]).FromJust();
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
    std::vector<int32_t> ttls;
    rocksdb::Status s = rocksdb::DB::ListColumnFamilies(options, path, &familyNames);

    for (std::vector<string>::iterator it = familyNames.begin() ; it != familyNames.end(); ++it) {
      families.push_back(rocksdb::ColumnFamilyDescriptor(*it, rocksdb::ColumnFamilyOptions()));
      ttls.push_back(ttl);
    }

    // if there's an error listing the column families, assume the database doesn't exist yet and just specify the default column family
    if (!s.ok()) {
      families.push_back(rocksdb::ColumnFamilyDescriptor(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
      ttls.push_back(ttl);
    }

    rocksdb::DBWithTTL* db;
    std::vector<rocksdb::ColumnFamilyHandle*> *handles = new std::vector<rocksdb::ColumnFamilyHandle*>();
    s = rocksdb::DBWithTTL::Open(options, path, families, handles, &db, ttls, readOnly);
    if (!s.ok()) {
      Nan::ThrowError(s.getState());
      return;
    }

    DBWithTTL* dbNode = new DBWithTTL(options, path, db, handles);
    dbNode->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    const unsigned argc = info.Length();
    v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
    for (unsigned int i=0; i<argc; i++) {
      argv[i] = info[i];
    }

    v8::Local<v8::FunctionTemplate> cons = Nan::New<v8::FunctionTemplate>(dbwithttl_constructor);
    v8::Local<v8::Object> instance = Nan::NewInstance(cons->GetFunction(), argc, argv).ToLocalChecked();

    delete [] argv;
    argv = NULL;
    info.GetReturnValue().Set(instance);
  }
}

void DBWithTTL::NewInstance(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();

  const unsigned argc = info.Length();
  v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
  for (unsigned int i=0; i<argc; i++) {
    argv[i] = info[i];
  }

  v8::Local<v8::FunctionTemplate> cons = Nan::New<v8::FunctionTemplate>(dbwithttl_constructor);

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


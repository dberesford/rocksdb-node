#ifndef OptionsHelper_h
#define OptionsHelper_h

#include <nan.h>
#include "rocksdb/db.h"

class OptionsHelper {
 public:
  static void ProcessOpenOptions(v8::Local<v8::Object>, rocksdb::Options *opts);
};

#define BOOLEAN_OPTION(name, opts, options)                             \
  v8::Local<v8::String> name = Nan::New(#name).ToLocalChecked();        \
  if (opts->Has(name)) options->name = opts->Get(name)->BooleanValue();

#endif  // OptionsHelper_h
// Copyright 2008 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TOOLS_IDLGLUE_NG_STATIC_GLUE_NPAPI_COMMON_H__
#define TOOLS_IDLGLUE_NG_STATIC_GLUE_NPAPI_COMMON_H__

#include <npapi.h>
#include <npruntime.h>
#include <string>
#include <vector>


#define NPVARIANT_TO_NUMBER(_v)  (NPVARIANT_IS_INT32(_v) ? \
  NPVARIANT_TO_INT32(_v) : \
  NPVARIANT_IS_DOUBLE(_v) ? NPVARIANT_TO_DOUBLE(_v) : 0)

#define NPVARIANT_IS_NUMBER(_v)  (NPVARIANT_IS_INT32(_v) || \
  NPVARIANT_IS_DOUBLE(_v))

// Converts a UTF16 wide string to a UTF8 string
bool String16ToUTF8(const wchar_t *in, int len, std::string *out8);

// Converts a UTF8 string to a UTF16 wide string
bool UTF8ToString16(const char *in, int len, std::wstring *out16);

// Converts a UTF16 string to a NPVariant
bool String16ToNPVariant(const std::wstring &in, NPVariant *variant);

// Converts a UTF8 string to a NPVariant
bool StringToNPVariant(const std::string &in, NPVariant *variant);

// Converts an unsigned int to a std::string representation
std::string UIntToString(unsigned int value);

// Gets the i-th element of a JavaScript array.
bool GetNPArrayProperty(NPP npp, NPObject *object, int index,
                        NPVariant *output);

// Gets a property from a JavaScript object.
bool GetNPObjectProperty(NPP npp, NPObject *object, const char *name,
                         NPVariant *output);

// Creates an empty JavaScript array.
NPObject *CreateArray(NPP npp);

// ScopeId used to retrieve the text representation of a NPIdentifier, with
// automatic memory management.
class ScopedId {
 public:
  explicit ScopedId(NPIdentifier name);
  ~ScopedId();
  NPUTF8 *text() const { return text_; }
 private:
  NPUTF8 *text_;
};

// DebugScopedId works like ScopedId, but does nothing in release mode. It
// can be used to help debugging NPAPI dispatch functions, with no overhead in
// release.
#ifdef _DEBUG
class DebugScopedId : public ScopedId {
 public:
  explicit DebugScopedId(NPIdentifier name): ScopedId(name) {}
};
#else
class DebugScopedId {
 public:
  explicit DebugScopedId(NPIdentifier name) {}
  NPUTF8 *text() const { return NULL; }
};
#endif

// Variant is used to wrap a JavaScript variant, for use with user-glue
// functions with 'variant' parameter(s) in IDL.
class Variant {
  NPP npp_;
  NPVariant value_;
 public:
  Variant() : npp_(NULL), value_() {}
  Variant(NPP npp, NPVariant value) : npp_(npp), value_(value) {}
  Variant(const Variant &var) : npp_(var.npp_), value_(var.value_) {}
  bool GetObjectProperty(const char *name, Variant *variant) const {
    if (!NPVARIANT_IS_OBJECT(value_)) return false;
    NPVariant output;
    bool r = GetNPObjectProperty(npp_, NPVARIANT_TO_OBJECT(value_), name,
                                 &output);
    if (r) *variant = Variant(npp_, output);
    return r;
  }
  bool GetArrayProperty(int index, Variant *variant) const {
    if (!NPVARIANT_IS_OBJECT(value_)) return false;
    NPVariant output;
    bool r = GetNPArrayProperty(npp_, NPVARIANT_TO_OBJECT(value_), index,
                                &output);
    if (r) *variant = Variant(npp_, output);
    return r;
  }
  bool IsInt() const {
    return NPVARIANT_IS_INT32(value_);
  }
  int AsInt() const {
    return NPVARIANT_TO_INT32(value_);
  }
  void SetInt(int v) { INT32_TO_NPVARIANT(v, value_); }
  bool IsFloat() const {
    return NPVARIANT_IS_DOUBLE(value_);
  }
  float AsFloat() const {
    return static_cast<float>(NPVARIANT_TO_DOUBLE(value_));
  }
  void SetFloat(float f) {
    DOUBLE_TO_NPVARIANT(static_cast<double>(f), value_);
  }
  bool IsNumber() const {
    return IsInt() || IsFloat();
  }
  float AsNumber() const {
    if (IsInt())
      return static_cast<float>(AsInt());
    else
      return AsFloat();
  }
};

// An object that holds a callable NPObject and some arguments and allows the
// callable object to be called with those arguments either synchronously or
// asynchronously.
class NPCallback : public NPObject {
 public:
  // Returns whether asynchronous calls are supported by the browser.
  static bool SupportsAsync();

  // Create a new NPCallback.
  static NPCallback* Create(NPP npp);

  // Set the function and arguments.
  void Set(NPObject* function, const NPVariant* args, int num_args);

  // Call synchronously.
  bool Call(NPVariant* result);

  // Call asynchronously.
  bool CallAsync();

 private:
  explicit NPCallback(NPP npp);
  ~NPCallback();

  // Disallow copy constructor and assignment operator.
  // These are deliberately unimplemented.
  NPCallback(const NPCallback&);
  void operator=(const NPCallback&);

  static NPObject* Allocate(NPP npp, NPClass* the_class);
  static void Deallocate(NPObject* object);
  static void Invalidate(NPObject* object);
  const static NPClass np_class_;
  NPP npp_;
  NPObject* function_;
  std::vector<NPVariant> args_;
};

namespace glue {
namespace globals {

// This function must be implemented by the user of the glue generator.
// It need not do anything, but it's where errors in the glue will be reported.
// Currently the glue code only reports user errors such as parameter type
// mismatches.
void SetLastError(NPP npp, const char *error);

#ifdef PROFILE_GLUE

#define GLUE_SCOPED_PROFILE(npp, key, name) \
  glue::globals::ScopedProfile name((npp), (key))
#define GLUE_SCOPED_PROFILE_STOP(name) name.Stop()
#define GLUE_PROFILE_START(npp, key) glue::globals::ProfileStart((npp), (key))
#define GLUE_PROFILE_STOP(npp, key) glue::globals::ProfileStop((npp), (key))
#define GLUE_PROFILE_RESET(npp) glue::globals::ProfileReset(npp)
#define GLUE_PROFILE_TO_STRING(npp) glue::globals::ProfileToString(npp)

// These functions must be implemented by the user of the glue generator if
// profiling is desired.
void ProfileStart(NPP npp, const std::string& key);
void ProfileStop(NPP npp, const std::string& key);
void ProfileReset(NPP npp);
std::string ProfileToString(NPP npp);

class ScopedProfile {
 public:
  ScopedProfile(NPP npp, const std::string& key) : npp_(npp), key_(key),
      stopped_(false) {
    GLUE_PROFILE_START(npp_, key_);
  }
  ~ScopedProfile() {
    if (!stopped_) {
      GLUE_PROFILE_STOP(npp_, key_);
    }
  }
  void Stop() {
    GLUE_PROFILE_STOP(npp_, key_);
    stopped_ = true;
  }
 private:
  std::string key_;
  NPP npp_;
  bool stopped_;

  // Disallow implicit contructors.
  ScopedProfile(const ScopedProfile&);
  void operator=(const ScopedProfile&);
};

#else  // PROFILE_GLUE

#define GLUE_SCOPED_PROFILE(npp, key, name)
#define GLUE_SCOPED_PROFILE_STOP(name)
#define GLUE_PROFILE_START(npp, key)
#define GLUE_PROFILE_STOP(npp, key)
#define GLUE_PROFILE_RESET(npp)
#define GLUE_PROFILE_TO_STRING(npp) ""

#endif  // PROFILE_GLUE

}  // namespace globals
}  // namespace glue

#endif  // TOOLS_IDLGLUE_NG_STATIC_GLUE_NPAPI_COMMON_H__

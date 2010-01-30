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

#ifndef TOOLS_IDLGLUE_NG_STATIC_GLUE_NPAPI_STATIC_OBJECT_H__
#define TOOLS_IDLGLUE_NG_STATIC_GLUE_NPAPI_STATIC_OBJECT_H__

#include "common.h"

namespace glue {
namespace globals {

class NPAPIObject : public NPObject {
 public:
  explicit NPAPIObject(NPP npp);
  ~NPAPIObject();
  void set_base(NPAPIObject *base) { base_ = base; }
  NPAPIObject *base() { return base_; }
  void set_names(NPIdentifier *names) { names_ = names; }
  NPIdentifier *names() { return names_; }
  int count() { return count_; }
  NPP npp() {return npp_;}
  void AllocateNamespaceObjects(int count);
  void SetNamespaceObject(int i, NPAPIObject *object) {
    namespaces_[i] = object;
  }
  NPAPIObject *GetNamespaceObjectByIndex(int i) {
    return namespaces_[i];
  }
  NPAPIObject *GetNamespaceObject(NPIdentifier name) {
    DebugScopedId id(name);  // debug helper
    for (int i = 0; i < count_; ++i)
      if (name == names_[i])
        return namespaces_[i];
    return NULL;
  }
 private:
  NPP npp_;
  NPAPIObject **namespaces_;
  NPIdentifier *names_;
  int count_;
  NPAPIObject *base_;
};

NPObject *Allocate(NPP npp, NPClass *theClass);
void Deallocate(NPObject *header);
bool HasProperty(NPObject *header, NPIdentifier name);
bool GetProperty(NPObject *header, NPIdentifier name,
                 NPVariant *variant);
bool SetProperty(NPObject *header, NPIdentifier name,
                 const NPVariant *variant);

}  // namespace globals
}  // namespace glue

#endif  // TOOLS_IDLGLUE_NG_STATIC_GLUE_NPAPI_STATIC_OBJECT_H__

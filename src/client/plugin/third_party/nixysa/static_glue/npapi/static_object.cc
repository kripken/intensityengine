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

// This file implements some generic NPAPI glue for the "static object" of a
// class or namespace, as described in npapi_generator.py. It simply associates
// NPObjects to identifiers.

#include <string.h>
#include "static_object.h"

namespace glue {
namespace globals {

NPObject *Allocate(NPP npp, NPClass *theClass);
void Deallocate(NPObject *header);
static bool HasMethod(NPObject *header, NPIdentifier name);
static bool Invoke(NPObject *header, NPIdentifier name, const NPVariant *args,
                   uint32_t argCount, NPVariant *result);
bool HasProperty(NPObject *header, NPIdentifier name);
bool GetProperty(NPObject *header, NPIdentifier name, NPVariant *variant);
bool SetProperty(NPObject *header, NPIdentifier name, const NPVariant *variant);

bool Enumerate(NPObject *header, NPIdentifier **names, uint32_t *count);

static NPClass npclass = {
  NP_CLASS_STRUCT_VERSION,
  Allocate,
  Deallocate,
  0,
  HasMethod,
  Invoke,
  0,
  HasProperty,
  GetProperty,
  SetProperty,
  0,
  Enumerate
};

NPClass *GetNPClass(void) {
  return &npclass;
}

NPObject *Allocate(NPP npp, NPClass *theClass) {
  return new NPAPIObject(npp);
}

void Deallocate(NPObject *header) {
  delete static_cast<NPAPIObject *>(header);
}

static bool HasMethod(NPObject *header, NPIdentifier name) {
  DebugScopedId id(name);  // debug helper
  return false;
}

bool HasProperty(NPObject *header, NPIdentifier name) {
  DebugScopedId id(name);  // debug helper
  NPAPIObject *object = static_cast<NPAPIObject *>(header);
  return object->GetNamespaceObject(name) != NULL;
}

static bool Invoke(NPObject *header, NPIdentifier name, const NPVariant *args,
                   uint32_t argCount, NPVariant *result) {
  DebugScopedId id(name);  // debug helper
  return false;
}

bool GetProperty(NPObject *header, NPIdentifier name,
                       NPVariant *variant) {
  DebugScopedId id(name);  // debug helper
  NPAPIObject *object = static_cast<NPAPIObject *>(header);
  NPAPIObject *namespace_object = object->GetNamespaceObject(name);
  if (namespace_object) {
    NPN_RetainObject(namespace_object);
    OBJECT_TO_NPVARIANT(namespace_object, *variant);
    return true;
  }
  return false;
}

bool SetProperty(NPObject *header, NPIdentifier name,
                       const NPVariant *variant) {
  DebugScopedId id(name);  // debug helper
  NPAPIObject *object = static_cast<NPAPIObject *>(header);
  return false;
}

bool Enumerate(NPObject *header, NPIdentifier **names, uint32_t *count) {
  NPAPIObject *object = static_cast<NPAPIObject *>(header);
  *count = object->count();
  *names = static_cast<NPIdentifier *>(
      NPN_MemAlloc(*count * sizeof(NPIdentifier)));
  memcpy(*names, object->names(), *count*sizeof(NPIdentifier));
  return true;
}

NPAPIObject::NPAPIObject(NPP npp)
    : npp_(npp),
      namespaces_(NULL),
      names_(NULL),
      count_(0),
      base_(NULL) {
}

NPAPIObject::~NPAPIObject() {
  if (namespaces_) delete [] namespaces_;
}

void NPAPIObject::AllocateNamespaceObjects(int count) {
  if (namespaces_) delete [] namespaces_;
  namespaces_ = new NPAPIObject *[count];
  count_ = count;
}

}  // namespace globals
}  // namespace glue

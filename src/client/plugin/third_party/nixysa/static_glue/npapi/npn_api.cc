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

// Implementation of the NPN API
// The NPN API is defined in npapi.h, but isn't actually exposed as such by the
// browser. It is exposed through a static set of functions passed to the
// initialization entrypoint. This file contains the glue between the NPN
// functions and the browser functions.

#include <npfunctions.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

static NPNetscapeFuncs g_browser_functions;

// Gets the NPAPI major version.
static inline uint16_t GetMajorVersion(uint16_t version) {
  return version >> 8;
}

// Gets the NPAPI minor version.
static inline uint16_t GetMinorVersion(uint16_t version) {
  return version & 0xff;
}

// Work-around of NPN_HasProperty, using NPN_Enumerate.
static bool HasPropertyWorkaround(NPP npp,
                                  NPObject *npobj,
                                  NPIdentifier property_name) {
  NPIdentifier *identifiers = NULL;
  uint32_t count;
  bool result = NPN_Enumerate(npp, npobj, &identifiers, &count);
  if (!result || !identifiers)
    return false;
  bool found = false;
  for (unsigned int i = 0; i < count; ++i) {
    if (identifiers[i] == property_name) {
      found = true;
      break;
    }
  }
  NPN_MemFree(identifiers);
  return result;
}

// Check for the work-around case.
bool IsHasPropertyWorkaround() {
  return g_browser_functions.hasproperty == HasPropertyWorkaround;
}

NPError InitializeNPNApi(NPNetscapeFuncs *funcs) {
  if (!funcs)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if (GetMajorVersion(funcs->version) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  // We need at least NPRuntime.
  if (GetMinorVersion(funcs->version) < NPVERS_HAS_NPRUNTIME_SCRIPTING)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  // np_runtime_size is the offset of the function directly after the last
  // NPRuntime function, that is, the minimum NPRuntime size.
  size_t np_runtime_size =
    reinterpret_cast<char *>(&g_browser_functions.pushpopupsenabledstate) -
    reinterpret_cast<char *>(&g_browser_functions);

  // Did browser lie about supporting NPRuntime ?
  if (funcs->size < np_runtime_size)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  memset(&g_browser_functions, 0, sizeof(g_browser_functions));
  // Only copy functions we know about, and that the browser has.
  size_t size = std::min(sizeof(g_browser_functions),
                         static_cast<size_t>(funcs->size));
  memcpy(&g_browser_functions, funcs, size);

  // Safari doesn't implement NPN_HasProperty although it claims to support
  // NPRuntime. We have a (slower) workaround using NPN_Enumerate.
  // NOTE: it doesn't implement NPN_HasMethod or NPN_IntFromIdentifier either.
  if (!g_browser_functions.hasproperty && g_browser_functions.enumerate) {
    g_browser_functions.hasproperty = HasPropertyWorkaround;
  }
  return NPERR_NO_ERROR;
}

// npapi.h functions

void NP_LOADDS NPN_Version(int* plugin_major,
                           int* plugin_minor,
                           int* netscape_major,
                           int* netscape_minor) {
  *plugin_major = NP_VERSION_MAJOR;
  *plugin_minor = NP_VERSION_MINOR;
  *netscape_major = GetMajorVersion(g_browser_functions.version);
  *netscape_minor = GetMinorVersion(g_browser_functions.version);
}

NPError NP_LOADDS NPN_GetURLNotify(NPP instance,
                                   const char* url,
                                   const char* target,
                                   void* notify_data) {
  if (GetMinorVersion(g_browser_functions.version) < NPVERS_HAS_NOTIFICATION)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  return g_browser_functions.geturlnotify(instance, url, target, notify_data);
}

NPError NP_LOADDS NPN_GetURL(NPP instance,
                             const char* url,
                             const char* target) {
  return g_browser_functions.geturl(instance, url, target);
}

NPError NP_LOADDS NPN_PostURLNotify(NPP instance,
                                    const char* url,
                                    const char* target,
                                    uint32_t len,
                                    const char* buf,
                                    NPBool file,
                                    void* notify_data) {
  if (GetMinorVersion(g_browser_functions.version) < NPVERS_HAS_NOTIFICATION)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  return g_browser_functions.posturlnotify(instance, url, target, len, buf,
                                           file, notify_data);
}

NPError NP_LOADDS NPN_PostURL(NPP instance,
                              const char* url,
                              const char* target,
                              uint32_t len,
                              const char* buf,
                              NPBool file) {
  return g_browser_functions.posturl(instance, url, target, len, buf, file);
}

NPError NP_LOADDS NPN_RequestRead(NPStream* stream, NPByteRange* range_list) {
  return g_browser_functions.requestread(stream, range_list);
}

NPError NP_LOADDS NPN_NewStream(NPP instance,
                                NPMIMEType type,
                                const char* target,
                                NPStream** stream) {
  if (GetMinorVersion(g_browser_functions.version) < NPVERS_HAS_STREAMOUTPUT)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  return g_browser_functions.newstream(instance, type, target, stream);
}

int32_t NP_LOADDS NPN_Write(NPP instance,
                            NPStream* stream,
                            int32_t len,
                            void* buffer) {
  if (GetMinorVersion(g_browser_functions.version) < NPVERS_HAS_STREAMOUTPUT)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  return g_browser_functions.write(instance, stream, len, buffer);
}

NPError NP_LOADDS NPN_DestroyStream(NPP instance,
                                    NPStream* stream,
                                    NPReason reason) {
  if (GetMinorVersion(g_browser_functions.version) < NPVERS_HAS_STREAMOUTPUT)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  return g_browser_functions.destroystream(instance, stream, reason);
}

void NP_LOADDS NPN_Status(NPP instance, const char* message) {
  g_browser_functions.status(instance, message);
}

const char* NP_LOADDS NPN_UserAgent(NPP instance) {
  return g_browser_functions.uagent(instance);
}

void* NP_LOADDS NPN_MemAlloc(uint32_t size) {
  return g_browser_functions.memalloc(size);
}

void NP_LOADDS NPN_MemFree(void* ptr) {
  g_browser_functions.memfree(ptr);
}

uint32_t NP_LOADDS NPN_MemFlush(uint32_t size) {
  return g_browser_functions.memflush(size);
}

void NP_LOADDS NPN_ReloadPlugins(NPBool reload_pages) {
  g_browser_functions.reloadplugins(reload_pages);
}

NPError NP_LOADDS NPN_GetValue(NPP instance,
                               NPNVariable variable,
                               void *value) {
  return g_browser_functions.getvalue(instance, variable, value);
}

NPError NP_LOADDS NPN_SetValue(NPP instance,
                               NPPVariable variable,
                               void *value) {
  return g_browser_functions.setvalue(instance, variable, value);
}

void NP_LOADDS NPN_InvalidateRect(NPP instance, NPRect *invalid_rect) {
  g_browser_functions.invalidaterect(instance, invalid_rect);
}

void NP_LOADDS NPN_InvalidateRegion(NPP instance, NPRegion invalid_region) {
  g_browser_functions.invalidateregion(instance, invalid_region);
}

void NP_LOADDS NPN_ForceRedraw(NPP instance) {
  g_browser_functions.forceredraw(instance);
}

void NP_LOADDS NPN_PushPopupsEnabledState(NPP instance, NPBool enabled) {
  if (GetMinorVersion(g_browser_functions.version) <
      NPVERS_HAS_POPUPS_ENABLED_STATE)
    return;
  g_browser_functions.pushpopupsenabledstate(instance, enabled);
}

void NP_LOADDS NPN_PopPopupsEnabledState(NPP instance) {
  if (GetMinorVersion(g_browser_functions.version) <
      NPVERS_HAS_POPUPS_ENABLED_STATE)
    return;
  g_browser_functions.poppopupsenabledstate(instance);
}

void NP_LOADDS NPN_PluginThreadAsyncCall(NPP instance,
                                         void (*func) (void *),
                                         void *user_data) {
  if (GetMinorVersion(g_browser_functions.version) <
      NPVERS_HAS_PLUGIN_THREAD_ASYNC_CALL)
    return;
  g_browser_functions.pluginthreadasynccall(instance, func, user_data);
}

// npruntime.h functions

void NPN_ReleaseVariantValue(NPVariant *variant) {
  g_browser_functions.releasevariantvalue(variant);
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name) {
  return g_browser_functions.getstringidentifier(name);
}

void NPN_GetStringIdentifiers(const NPUTF8 **names,
                              int32_t count,
                              NPIdentifier *identifiers) {
#ifdef OS_LINUX
  // GetStringIdentifiers is broken in nspluginwrapper 1.2.0 and 1.2.2 so we
  // use GetStringIdentifier instead for maximum compatibility. See
  // https://www.redhat.com/archives/nspluginwrapper-devel-list/2009-June/msg00000.html
  for (int32_t i = 0; i < count; ++i) {
    identifiers[i] = NPN_GetStringIdentifier(names[i]);
  }
#else
  g_browser_functions.getstringidentifiers(names, count, identifiers);
#endif
}

NPIdentifier NPN_GetIntIdentifier(int32_t intid) {
  return g_browser_functions.getintidentifier(intid);
}

bool NPN_IdentifierIsString(NPIdentifier identifier) {
  return g_browser_functions.identifierisstring(identifier);
}

NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier identifier) {
  return g_browser_functions.utf8fromidentifier(identifier);
}

int32_t NPN_IntFromIdentifier(NPIdentifier identifier) {
  if (g_browser_functions.intfromidentifier != NULL) {
    return g_browser_functions.intfromidentifier(identifier);
  } else {
    int32_t result = 0;
    NPUTF8 *str = NPN_UTF8FromIdentifier(identifier);
    if (str != NULL) {
      result = atoi(str);
      NPN_MemFree(str);
    } else if (identifier != 0) {  // Safari 3.0 just gives us an int*
      result = *static_cast<int32_t*>(identifier);
    }
    return result;
  }
}

NPObject *NPN_CreateObject(NPP npp, NPClass *_class) {
  return g_browser_functions.createobject(npp, _class);
}

NPObject *NPN_RetainObject(NPObject *npobj) {
  return g_browser_functions.retainobject(npobj);
}

void NPN_ReleaseObject(NPObject *npobj) {
  g_browser_functions.releaseobject(npobj);
}

bool NPN_Invoke(NPP npp,
                NPObject *npobj,
                NPIdentifier method_name,
                const NPVariant *args,
                uint32_t arg_count,
                NPVariant *result) {
  return g_browser_functions.invoke(npp, npobj, method_name, args, arg_count,
                                    result);
}

bool NPN_InvokeDefault(NPP npp,
                       NPObject *npobj,
                       const NPVariant *args,
                       uint32_t arg_count,
                       NPVariant *result) {
  return g_browser_functions.invokeDefault(npp, npobj, args, arg_count, result);
}

bool NPN_Evaluate(NPP npp,
                  NPObject *npobj,
                  NPString *script,
                  NPVariant *result) {
  return g_browser_functions.evaluate(npp, npobj, script, result);
}

bool NPN_GetProperty(NPP npp,
                     NPObject *npobj,
                     NPIdentifier property_name,
                     NPVariant *result) {
  return g_browser_functions.getproperty(npp, npobj, property_name, result);
}

bool NPN_SetProperty(NPP npp,
                     NPObject *npobj,
                     NPIdentifier property_name,
                     const NPVariant *value) {
  return g_browser_functions.setproperty(npp, npobj, property_name, value);
}

bool NPN_RemoveProperty(NPP npp, NPObject *npobj, NPIdentifier property_name) {
  return g_browser_functions.removeproperty(npp, npobj, property_name);
}

bool NPN_HasProperty(NPP npp, NPObject *npobj, NPIdentifier property_name) {
  return g_browser_functions.hasproperty(npp, npobj, property_name);
}

bool NPN_HasMethod(NPP npp, NPObject *npobj, NPIdentifier method_name) {
  return g_browser_functions.hasmethod(npp, npobj, method_name);
}

bool NPN_Enumerate(NPP npp,
                   NPObject *npobj,
                   NPIdentifier **identifier,
                   uint32_t *count) {
  if (GetMinorVersion(g_browser_functions.version) < NPVERS_HAS_NPOBJECT_ENUM)
    return false;
  return g_browser_functions.enumerate(npp, npobj, identifier, count);
}

bool NPN_Construct(NPP npp,
                   NPObject *npobj,
                   const NPVariant *args,
                   uint32_t arg_count,
                   NPVariant *result) {
  return g_browser_functions.construct(npp, npobj, args, arg_count, result);
}

void NPN_SetException(NPObject *npobj, const NPUTF8 *message) {
  g_browser_functions.setexception(npobj, message);
}

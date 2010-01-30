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

// This file provides a stub implementation of most plugin entrypoints.

#include <npapi.h>
#include "npn_api.h"
#include "globals_glue.h"

namespace glue {
namespace globals {

void SetLastError(NPP npp, const char *error) {
}

}
}

extern "C" {
  NPError OSCALL NP_GetEntryPoints(NPPluginFuncs *pluginFuncs) {
    pluginFuncs->version = 11;
    pluginFuncs->size = sizeof(*pluginFuncs);
    pluginFuncs->newp = NPP_New;
    pluginFuncs->destroy = NPP_Destroy;
    pluginFuncs->setwindow = NPP_SetWindow;
    pluginFuncs->newstream = NPP_NewStream;
    pluginFuncs->destroystream = NPP_DestroyStream;
    pluginFuncs->asfile = NPP_StreamAsFile;
    pluginFuncs->writeready = NPP_WriteReady;
    pluginFuncs->write = NPP_Write;
    pluginFuncs->print = NPP_Print;
    pluginFuncs->event = NPP_HandleEvent;
    pluginFuncs->urlnotify = NPP_URLNotify;
    pluginFuncs->getvalue = NPP_GetValue;
    pluginFuncs->setvalue = NPP_SetValue;

    return NPERR_NO_ERROR;
  }

#if defined(OS_WINDOWS) || defined(OS_MACOSX)
  NPError OSCALL NP_Initialize(NPNetscapeFuncs *browserFuncs) {
    return InitializeNPNApi(browserFuncs);
  }
#else
  NPError OSCALL NP_Initialize(NPNetscapeFuncs *browserFuncs,
                               NPPluginFuncs *pluginFuncs) {
    NPError retval = InitializeNPNApi(browserFuncs);
    if (retval != NPERR_NO_ERROR) return retval;
    NP_GetEntryPoints(pluginFuncs);
    return NPERR_NO_ERROR;
  }
#endif

  NPError OSCALL NP_Shutdown(void) {
    return NPERR_NO_ERROR;
  }

  NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode,
                  int16_t argc, char *argn[], char *argv[],
                  NPSavedData *saved) {
    glue::InitializeGlue(instance);
    NPObject *object = glue::CreateStaticNPObject(instance);
    instance->pdata = object;
    return NPERR_NO_ERROR;
  }

  NPError NPP_Destroy(NPP instance, NPSavedData **save) {
    NPObject *object = static_cast<NPObject*>(instance->pdata);
    if (object) {
      NPN_ReleaseObject(object);
      instance->pdata = NULL;
    }
    return NPERR_NO_ERROR;
  }

  NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value) {
    switch (variable) {
      case NPPVpluginScriptableNPObject: {
        void **v = static_cast<void **>(value);
        NPObject *obj = static_cast<NPObject *>(instance->pdata);
        NPN_RetainObject(obj);
        *v = obj;
        break;
      }
      case NPPVpluginNeedsXEmbed:
        *static_cast<NPBool *>(value) = true;
        break;
      default:
        return NPERR_INVALID_PARAM;
        break;
    }
    return NPERR_NO_ERROR;
  }

  NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value) {
    return NPERR_GENERIC_ERROR;
  }


  NPError NPP_SetWindow(NPP instance, NPWindow *window) {
    return NPERR_NO_ERROR;
  }

  void NPP_StreamAsFile(NPP instance, NPStream *stream, const char *fname) {
  }

  int16_t NPP_HandleEvent(NPP instance, void *event) {
    return 0;
  }

  NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream *stream,
                        NPBool seekable, uint16_t *stype) {
    return NPERR_NO_ERROR;
  }

  NPError NPP_DestroyStream(NPP instance, NPStream *stream, NPReason reason) {
    return NPERR_NO_ERROR;
  }

  int32_t NPP_WriteReady(NPP instance, NPStream *stream) {
    return 0;
  }

  int32_t NPP_Write(NPP instance, NPStream *stream, int32_t offset, int32_t len,
                    void *buffer) {
    return 0;
  }

  void NPP_Print(NPP instance, NPPrint *platformPrint) {
  }

  void NPP_URLNotify(NPP instance, const char *url, NPReason reason,
                     void *notifyData) {
  }
}  // extern "C"

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

#ifndef TOOLS_IDLGLUE_NG_STATIC_GLUE_NPAPI_NPN_API_H_
#define TOOLS_IDLGLUE_NG_STATIC_GLUE_NPAPI_NPN_API_H_

#include <npfunctions.h>

// Initializes the NPN API, by copying the browser functions given by the
// browser.
// NOTE: this function will fail if the browser doesn't support NPRuntime
// (minor version at least 14).
// Parameters:
//   functions: the function table provided by the browser on plug-in
//   initialization.
// Returns:
//   NPERR_NO_ERROR if no error.
//   NPERR_INVALID_FUNCTABLE_ERROR if the function table passed in has an
//   incorrect size
//   NPERR_INCOMPATIBLE_VERSION_ERROR if the major NPAPI version in the
//   function table doesn't match the expected one
NPError InitializeNPNApi(NPNetscapeFuncs *functions);

// Checks whether NPN_HasProperty is implemented with a slower work-around.
// Safari claims to support NPRuntime, but doesn't implement NPN_HasProperty,
// so we use a work-around using NPN_Enumerate, but it is (much) slower.
// This function allows the user to test for this case (and avoid
// NPN_HasProperty if possible).
bool IsHasPropertyWorkaround();

#endif  // TOOLS_IDLGLUE_NG_STATIC_GLUE_NPAPI_NPN_API_H_

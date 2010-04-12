
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "intensity_cegui.pkg"
/*
** Lua binding: 
** Generated automatically by tolua++-1.0.92 on Mon Dec 22 20:41:20 2008.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
int tolua__open (lua_State* tolua_S);


/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"CEGUI::Window");
}

/* function: IntensityCEGUI::luaNewMap */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaNewMap00
static int tolua__IntensityCEGUI_luaNewMap00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  IntensityCEGUI::luaNewMap();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaNewMap'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaStartTest */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaStartTest00
static int tolua__IntensityCEGUI_luaStartTest00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  IntensityCEGUI::luaStartTest();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaStartTest'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaQuit */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaQuit00
static int tolua__IntensityCEGUI_luaQuit00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  IntensityCEGUI::luaQuit();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaQuit'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaCharacterView */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaCharacterView00
static int tolua__IntensityCEGUI_luaCharacterView00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  IntensityCEGUI::luaCharacterView();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaCharacterView'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaGetUsername */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaGetUsername00
static int tolua__IntensityCEGUI_luaGetUsername00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::luaGetUsername();
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaGetUsername'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaGetPassword */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaGetPassword00
static int tolua__IntensityCEGUI_luaGetPassword00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::luaGetPassword();
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaGetPassword'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaLogin */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaLogin00
static int tolua__IntensityCEGUI_luaLogin00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_iscppstring(tolua_S,1,0,&tolua_err) ||
 !tolua_iscppstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  std::string username = ((std::string)  tolua_tocppstring(tolua_S,1,0));
  std::string password = ((std::string)  tolua_tocppstring(tolua_S,2,0));
 {
  IntensityCEGUI::luaLogin(username,password);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaLogin'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaSetHorns */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaSetHorns00
static int tolua__IntensityCEGUI_luaSetHorns00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  int id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  IntensityCEGUI::luaSetHorns(id);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaSetHorns'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaSetArmor */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaSetArmor00
static int tolua__IntensityCEGUI_luaSetArmor00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  int id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  IntensityCEGUI::luaSetArmor(id);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaSetArmor'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaSetRightWeapon */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaSetRightWeapon00
static int tolua__IntensityCEGUI_luaSetRightWeapon00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  int id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  IntensityCEGUI::luaSetRightWeapon(id);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaSetRightWeapon'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaSetLeftWeapon */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaSetLeftWeapon00
static int tolua__IntensityCEGUI_luaSetLeftWeapon00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  int id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  IntensityCEGUI::luaSetLeftWeapon(id);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaSetLeftWeapon'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaSetRightWeaponSparkle */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaSetRightWeaponSparkle00
static int tolua__IntensityCEGUI_luaSetRightWeaponSparkle00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  int id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  IntensityCEGUI::luaSetRightWeaponSparkle(id);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaSetRightWeaponSparkle'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaToggleEdit */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaToggleEdit00
static int tolua__IntensityCEGUI_luaToggleEdit00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  IntensityCEGUI::luaToggleEdit();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaToggleEdit'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaGetEditedUniqueId */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaGetEditedUniqueId00
static int tolua__IntensityCEGUI_luaGetEditedUniqueId00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::luaGetEditedUniqueId();
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaGetEditedUniqueId'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaGetEditedClass */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaGetEditedClass00
static int tolua__IntensityCEGUI_luaGetEditedClass00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::luaGetEditedClass();
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaGetEditedClass'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::lauGetCurrEditedEntityKey */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_lauGetCurrEditedEntityKey00
static int tolua__IntensityCEGUI_lauGetCurrEditedEntityKey00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::lauGetCurrEditedEntityKey();
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'lauGetCurrEditedEntityKey'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::lauGetCurrEditedEntityGUIName */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_lauGetCurrEditedEntityGUIName00
static int tolua__IntensityCEGUI_lauGetCurrEditedEntityGUIName00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::lauGetCurrEditedEntityGUIName();
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'lauGetCurrEditedEntityGUIName'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::lauGetCurrEditedEntityValue */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_lauGetCurrEditedEntityValue00
static int tolua__IntensityCEGUI_lauGetCurrEditedEntityValue00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::lauGetCurrEditedEntityValue();
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'lauGetCurrEditedEntityValue'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaSetCurrEditedEntityValue */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaSetCurrEditedEntityValue00
static int tolua__IntensityCEGUI_luaSetCurrEditedEntityValue00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_iscppstring(tolua_S,1,0,&tolua_err) ||
 !tolua_iscppstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  std::string key = ((std::string)  tolua_tocppstring(tolua_S,1,0));
  std::string value = ((std::string)  tolua_tocppstring(tolua_S,2,0));
 {
  IntensityCEGUI::luaSetCurrEditedEntityValue(key,value);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaSetCurrEditedEntityValue'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaExecuteSauerCommand */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaExecuteSauerCommand00
static int tolua__IntensityCEGUI_luaExecuteSauerCommand00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_iscppstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  std::string command = ((std::string)  tolua_tocppstring(tolua_S,1,0));
 {
  IntensityCEGUI::luaExecuteSauerCommand(command);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaExecuteSauerCommand'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaGetSauerVariable */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaGetSauerVariable00
static int tolua__IntensityCEGUI_luaGetSauerVariable00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_iscppstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  std::string variable = ((std::string)  tolua_tocppstring(tolua_S,1,0));
 {
  int tolua_ret = (int)  IntensityCEGUI::luaGetSauerVariable(variable);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaGetSauerVariable'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaExecutePythonScript */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaExecutePythonScript00
static int tolua__IntensityCEGUI_luaExecutePythonScript00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_iscppstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  std::string script = ((std::string)  tolua_tocppstring(tolua_S,1,0));
 {
  IntensityCEGUI::luaExecutePythonScript(script);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaExecutePythonScript'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaEvaluatePythonScriptInteger */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaEvaluatePythonScriptInteger00
static int tolua__IntensityCEGUI_luaEvaluatePythonScriptInteger00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_iscppstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  std::string script = ((std::string)  tolua_tocppstring(tolua_S,1,0));
 {
  int tolua_ret = (int)  IntensityCEGUI::luaEvaluatePythonScriptInteger(script);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaEvaluatePythonScriptInteger'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaEvaluatePythonScriptFloat */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaEvaluatePythonScriptFloat00
static int tolua__IntensityCEGUI_luaEvaluatePythonScriptFloat00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_iscppstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  std::string script = ((std::string)  tolua_tocppstring(tolua_S,1,0));
 {
  float tolua_ret = (float)  IntensityCEGUI::luaEvaluatePythonScriptFloat(script);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaEvaluatePythonScriptFloat'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaEvaluatePythonScriptString */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaEvaluatePythonScriptString00
static int tolua__IntensityCEGUI_luaEvaluatePythonScriptString00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_iscppstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  std::string script = ((std::string)  tolua_tocppstring(tolua_S,1,0));
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::luaEvaluatePythonScriptString(script);
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaEvaluatePythonScriptString'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaGetQueuedMessageTitle */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaGetQueuedMessageTitle00
static int tolua__IntensityCEGUI_luaGetQueuedMessageTitle00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::luaGetQueuedMessageTitle();
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaGetQueuedMessageTitle'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaGetQueuedMessageText */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaGetQueuedMessageText00
static int tolua__IntensityCEGUI_luaGetQueuedMessageText00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::luaGetQueuedMessageText();
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaGetQueuedMessageText'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::lauGetCurrClass */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_lauGetCurrClass00
static int tolua__IntensityCEGUI_lauGetCurrClass00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  std::string tolua_ret = (std::string)  IntensityCEGUI::lauGetCurrClass();
 tolua_pushcppstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'lauGetCurrClass'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::queueEntityCreation */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_queueEntityCreation00
static int tolua__IntensityCEGUI_queueEntityCreation00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_iscppstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  std::string _class = ((std::string)  tolua_tocppstring(tolua_S,1,0));
 {
  IntensityCEGUI::queueEntityCreation(_class);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'queueEntityCreation'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaPopulateEntityList */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaPopulateEntityList00
static int tolua__IntensityCEGUI_luaPopulateEntityList00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"CEGUI::Window",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  CEGUI::Window* entityListWindow = ((CEGUI::Window*)  tolua_tousertype(tolua_S,1,0));
 {
  IntensityCEGUI::luaPopulateEntityList(entityListWindow);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaPopulateEntityList'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaFocusOnEntity */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaFocusOnEntity00
static int tolua__IntensityCEGUI_luaFocusOnEntity00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  int uniqueId = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  IntensityCEGUI::luaFocusOnEntity(uniqueId);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaFocusOnEntity'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaPopulateSettings */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaPopulateSettings00
static int tolua__IntensityCEGUI_luaPopulateSettings00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"CEGUI::Window",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  CEGUI::Window* settingsWindow = ((CEGUI::Window*)  tolua_tousertype(tolua_S,1,0));
 {
  IntensityCEGUI::luaPopulateSettings(settingsWindow);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaPopulateSettings'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: IntensityCEGUI::luaApplySoundSettings */
#ifndef TOLUA_DISABLE_tolua__IntensityCEGUI_luaApplySoundSettings00
static int tolua__IntensityCEGUI_luaApplySoundSettings00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"CEGUI::Window",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  CEGUI::Window* settingsWindow = ((CEGUI::Window*)  tolua_tousertype(tolua_S,1,0));
 {
  IntensityCEGUI::luaApplySoundSettings(settingsWindow);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'luaApplySoundSettings'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
int tolua__open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
 tolua_module(tolua_S,"IntensityCEGUI",0);
 tolua_beginmodule(tolua_S,"IntensityCEGUI");
  tolua_function(tolua_S,"luaNewMap",tolua__IntensityCEGUI_luaNewMap00);
  tolua_function(tolua_S,"luaStartTest",tolua__IntensityCEGUI_luaStartTest00);
  tolua_function(tolua_S,"luaQuit",tolua__IntensityCEGUI_luaQuit00);
  tolua_function(tolua_S,"luaCharacterView",tolua__IntensityCEGUI_luaCharacterView00);
  tolua_function(tolua_S,"luaGetUsername",tolua__IntensityCEGUI_luaGetUsername00);
  tolua_function(tolua_S,"luaGetPassword",tolua__IntensityCEGUI_luaGetPassword00);
  tolua_function(tolua_S,"luaLogin",tolua__IntensityCEGUI_luaLogin00);
  tolua_function(tolua_S,"luaSetHorns",tolua__IntensityCEGUI_luaSetHorns00);
  tolua_function(tolua_S,"luaSetArmor",tolua__IntensityCEGUI_luaSetArmor00);
  tolua_function(tolua_S,"luaSetRightWeapon",tolua__IntensityCEGUI_luaSetRightWeapon00);
  tolua_function(tolua_S,"luaSetLeftWeapon",tolua__IntensityCEGUI_luaSetLeftWeapon00);
  tolua_function(tolua_S,"luaSetRightWeaponSparkle",tolua__IntensityCEGUI_luaSetRightWeaponSparkle00);
  tolua_function(tolua_S,"luaToggleEdit",tolua__IntensityCEGUI_luaToggleEdit00);
  tolua_function(tolua_S,"luaGetEditedUniqueId",tolua__IntensityCEGUI_luaGetEditedUniqueId00);
  tolua_function(tolua_S,"luaGetEditedClass",tolua__IntensityCEGUI_luaGetEditedClass00);
  tolua_function(tolua_S,"lauGetCurrEditedEntityKey",tolua__IntensityCEGUI_lauGetCurrEditedEntityKey00);
  tolua_function(tolua_S,"lauGetCurrEditedEntityGUIName",tolua__IntensityCEGUI_lauGetCurrEditedEntityGUIName00);
  tolua_function(tolua_S,"lauGetCurrEditedEntityValue",tolua__IntensityCEGUI_lauGetCurrEditedEntityValue00);
  tolua_function(tolua_S,"luaSetCurrEditedEntityValue",tolua__IntensityCEGUI_luaSetCurrEditedEntityValue00);
  tolua_function(tolua_S,"luaExecuteSauerCommand",tolua__IntensityCEGUI_luaExecuteSauerCommand00);
  tolua_function(tolua_S,"luaGetSauerVariable",tolua__IntensityCEGUI_luaGetSauerVariable00);
  tolua_function(tolua_S,"luaExecutePythonScript",tolua__IntensityCEGUI_luaExecutePythonScript00);
  tolua_function(tolua_S,"luaEvaluatePythonScriptInteger",tolua__IntensityCEGUI_luaEvaluatePythonScriptInteger00);
  tolua_function(tolua_S,"luaEvaluatePythonScriptFloat",tolua__IntensityCEGUI_luaEvaluatePythonScriptFloat00);
  tolua_function(tolua_S,"luaEvaluatePythonScriptString",tolua__IntensityCEGUI_luaEvaluatePythonScriptString00);
  tolua_function(tolua_S,"luaGetQueuedMessageTitle",tolua__IntensityCEGUI_luaGetQueuedMessageTitle00);
  tolua_function(tolua_S,"luaGetQueuedMessageText",tolua__IntensityCEGUI_luaGetQueuedMessageText00);
  tolua_function(tolua_S,"lauGetCurrClass",tolua__IntensityCEGUI_lauGetCurrClass00);
  tolua_function(tolua_S,"queueEntityCreation",tolua__IntensityCEGUI_queueEntityCreation00);
  tolua_function(tolua_S,"luaPopulateEntityList",tolua__IntensityCEGUI_luaPopulateEntityList00);
  tolua_function(tolua_S,"luaFocusOnEntity",tolua__IntensityCEGUI_luaFocusOnEntity00);
  tolua_function(tolua_S,"luaPopulateSettings",tolua__IntensityCEGUI_luaPopulateSettings00);
  tolua_function(tolua_S,"luaApplySoundSettings",tolua__IntensityCEGUI_luaApplySoundSettings00);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 int luaopen_ (lua_State* tolua_S) {
 return tolua__open(tolua_S);
};
#endif


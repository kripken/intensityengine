
//=======================================\\
// These patches are against CEGUI 0.6.1 \\
//=======================================\\

//
// The following is a patch for CEGUI/ScriptingModules/CEGUILua/LuaScriptModule/src/CEGUILuaFunctor.cpp
// Replace the operator() therein with the following one.
//
// This has the following effects:
//
//  1. Actually uses Lua event return values, so a Lua script can *not* handle an event. In CEGUI as-is,
//      all Lua scripts effectively set 'handled' to true!
//  2. Asserts that all Lua event handlers return a boolean value, like the standard in other GUIs
//
// This has been reported to CEGUI on their forums, http://www.cegui.org.uk/phpBB2/viewtopic.php?t=3474 ,
// and it seems they will patch it in a later release.
//

bool LuaFunctor::operator()(const EventArgs& args) const
{
    // is this a late binding?
    if (needs_lookup)
    {
        pushNamedFunction(L, function_name);
        // reference function
        index = luaL_ref(L, LUA_REGISTRYINDEX);
        needs_lookup = false;
        CEGUI_LOGINSANE("Late binding of callback '"+function_name+"' performed");
        function_name.clear();
    } // if (needs_lookup)

	ScriptWindowHelper* helper = 0;
	//Set a global for this window
	if(args.d_hasWindow)
	{
		WindowEventArgs& we = (WindowEventArgs&)args;
		helper = new ScriptWindowHelper(we.window);
		tolua_pushusertype(L,(void*)helper,"CEGUI::ScriptWindowHelper");
		lua_setglobal(L,"this");
	}

    // retrieve function
    lua_rawgeti(L, LUA_REGISTRYINDEX, index);

    // possibly self as well?
    int nargs = 1;
    if (self != LUA_NOREF)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, self);
        ++nargs;
    }

    // push EventArgs  parameter
    tolua_pushusertype(L, (void*)&args, "const CEGUI::EventArgs");

    // call it
    int error = lua_pcall(L, nargs, 1, 0); // Kripken: Add return value

    // handle errors
    if (error)
    {
        String errStr(lua_tostring(L, -1));
        lua_pop(L, 1);
		if(helper)
		{
			delete helper;
			helper = 0;
		}
        throw ScriptException("Unable to call Lua event handler:\n\n"+errStr+"\n");
    } // if (error)

    // Kripken: retrieve result
    bool ret = true;
    if (lua_isboolean(L, -1))
        ret = lua_toboolean(L, -1);
    else
        throw ScriptException("Lua script did not return a boolean\r\n");
    lua_pop(L, 1);

	if(helper)
	{
		delete helper;
		helper = 0;
	}

    return ret;
}



//================================================================================================
//
// The following is a patch to let Lua use TreeEventArgs. It has been submitted to CEGUI here
//      http://www.cegui.org.uk/phpBB2/viewtopic.php?p=16785#16785
//
//================================================================================================

=== modified file 'ScriptingModules/CEGUILua/LuaScriptModule/package/HelperFunctions.pkg'
7a8,11
> function CEGUI.toTreeEventArgs(e)
>     return tolua.cast(e,"const CEGUI::TreeEventArgs")
> end
> 
35a40
> CEGUI.EventArgs.toTreeEventArgs             = CEGUI.toTreeEventArgs

=== modified file 'ScriptingModules/CEGUILua/LuaScriptModule/package/InputEvent.pkg'
94a95,104
> /***********************************************************************
> 	TreeEventArgs
> ***********************************************************************/
> class TreeEventArgs : public WindowEventArgs
> {
> 	TreeItem *treeItem;
> 
> 	TreeEventArgs(Window* wnd);
> };
> 


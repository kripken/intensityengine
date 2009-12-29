
/*
 *=============================================================================
 * Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
 *
 * This file is part of the Intensity Engine project,
 *    http://www.intensityengine.com
 *
 * The Intensity Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * The Intensity Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Intensity Engine.  If not, see
 *     http://www.gnu.org/licenses/
 *     http://www.gnu.org/licenses/agpl-3.0.html
 *=============================================================================
 */


#include "RendererModules/OpenGLGUIRenderer/openglrenderer.h"
#include "CEGUIDefaultResourceProvider.h"
#include "CEGUILua.h"

#include "CEGUISimpleResourceProvider.h"

#include "pch.h"
#include "engine.h"

#include "logging.h"
#include "client_system.h"
#include "targeting.h"
#include "editing_system.h"

#include "intensity_cegui.h"


// FIXME: For some reason, tolua changes between these two... very strange. Here, and where it is called, need
// to be changed now and then...
//int tolua_sauer_cegui_open (lua_State* tolua_S);
int tolua__open (lua_State* tolua_S);


//using namespace CEGUI;

namespace IntensityCEGUI
{

// Tools

CEGUI::uint SDLKeyToCEGUIKey(SDLKey key)
{
    switch (key)
    {
        case SDLK_BACKSPACE:    return CEGUI::Key::Backspace;
        case SDLK_TAB:          return CEGUI::Key::Tab;
        case SDLK_RETURN:       return CEGUI::Key::Return;
        case SDLK_PAUSE:        return CEGUI::Key::Pause;
        case SDLK_ESCAPE:       return CEGUI::Key::Escape;
        case SDLK_SPACE:        return CEGUI::Key::Space;
        case SDLK_COMMA:        return CEGUI::Key::Comma;
        case SDLK_MINUS:        return CEGUI::Key::Minus;
        case SDLK_PERIOD:       return CEGUI::Key::Period;
        case SDLK_SLASH:        return CEGUI::Key::Slash;
        case SDLK_0:            return CEGUI::Key::Zero;
        case SDLK_1:            return CEGUI::Key::One;
        case SDLK_2:            return CEGUI::Key::Two;
        case SDLK_3:            return CEGUI::Key::Three;
        case SDLK_4:            return CEGUI::Key::Four;
        case SDLK_5:            return CEGUI::Key::Five;
        case SDLK_6:            return CEGUI::Key::Six;
        case SDLK_7:            return CEGUI::Key::Seven;
        case SDLK_8:            return CEGUI::Key::Eight;
        case SDLK_9:            return CEGUI::Key::Nine;
        case SDLK_COLON:        return CEGUI::Key::Colon;
        case SDLK_SEMICOLON:    return CEGUI::Key::Semicolon;
        case SDLK_EQUALS:       return CEGUI::Key::Equals;
        case SDLK_LEFTBRACKET:  return CEGUI::Key::LeftBracket;
        case SDLK_BACKSLASH:    return CEGUI::Key::Backslash;
        case SDLK_RIGHTBRACKET: return CEGUI::Key::RightBracket;
        case SDLK_a:            return CEGUI::Key::A;
        case SDLK_b:            return CEGUI::Key::B;
        case SDLK_c:            return CEGUI::Key::C;
        case SDLK_d:            return CEGUI::Key::D;
        case SDLK_e:            return CEGUI::Key::E;
        case SDLK_f:            return CEGUI::Key::F;
        case SDLK_g:            return CEGUI::Key::G;
        case SDLK_h:            return CEGUI::Key::H;
        case SDLK_i:            return CEGUI::Key::I;
        case SDLK_j:            return CEGUI::Key::J;
        case SDLK_k:            return CEGUI::Key::K;
        case SDLK_l:            return CEGUI::Key::L;
        case SDLK_m:            return CEGUI::Key::M;
        case SDLK_n:            return CEGUI::Key::N;
        case SDLK_o:            return CEGUI::Key::O;
        case SDLK_p:            return CEGUI::Key::P;
        case SDLK_q:            return CEGUI::Key::Q;
        case SDLK_r:            return CEGUI::Key::R;
        case SDLK_s:            return CEGUI::Key::S;
        case SDLK_t:            return CEGUI::Key::T;
        case SDLK_u:            return CEGUI::Key::U;
        case SDLK_v:            return CEGUI::Key::V;
        case SDLK_w:            return CEGUI::Key::W;
        case SDLK_x:            return CEGUI::Key::X;
        case SDLK_y:            return CEGUI::Key::Y;
        case SDLK_z:            return CEGUI::Key::Z;
        case SDLK_DELETE:       return CEGUI::Key::Delete;
        case SDLK_KP0:          return CEGUI::Key::Numpad0;
        case SDLK_KP1:          return CEGUI::Key::Numpad1;
        case SDLK_KP2:          return CEGUI::Key::Numpad2;
        case SDLK_KP3:          return CEGUI::Key::Numpad3;
        case SDLK_KP4:          return CEGUI::Key::Numpad4;
        case SDLK_KP5:          return CEGUI::Key::Numpad5;
        case SDLK_KP6:          return CEGUI::Key::Numpad6;
        case SDLK_KP7:          return CEGUI::Key::Numpad7;
        case SDLK_KP8:          return CEGUI::Key::Numpad8;
        case SDLK_KP9:          return CEGUI::Key::Numpad9;
        case SDLK_KP_PERIOD:    return CEGUI::Key::Decimal;
        case SDLK_KP_DIVIDE:    return CEGUI::Key::Divide;
        case SDLK_KP_MULTIPLY:  return CEGUI::Key::Multiply;
        case SDLK_KP_MINUS:     return CEGUI::Key::Subtract;
        case SDLK_KP_PLUS:      return CEGUI::Key::Add;
        case SDLK_KP_ENTER:     return CEGUI::Key::NumpadEnter;
        case SDLK_KP_EQUALS:    return CEGUI::Key::NumpadEquals;
        case SDLK_UP:           return CEGUI::Key::ArrowUp;
        case SDLK_DOWN:         return CEGUI::Key::ArrowDown;
        case SDLK_RIGHT:        return CEGUI::Key::ArrowRight;
        case SDLK_LEFT:         return CEGUI::Key::ArrowLeft;
        case SDLK_INSERT:       return CEGUI::Key::Insert;
        case SDLK_HOME:         return CEGUI::Key::Home;
        case SDLK_END:          return CEGUI::Key::End;
        case SDLK_PAGEUP:       return CEGUI::Key::PageUp;
        case SDLK_PAGEDOWN:     return CEGUI::Key::PageDown;
        case SDLK_F1:           return CEGUI::Key::F1;
        case SDLK_F2:           return CEGUI::Key::F2;
        case SDLK_F3:           return CEGUI::Key::F3;
        case SDLK_F4:           return CEGUI::Key::F4;
        case SDLK_F5:           return CEGUI::Key::F5;
        case SDLK_F6:           return CEGUI::Key::F6;
        case SDLK_F7:           return CEGUI::Key::F7;
        case SDLK_F8:           return CEGUI::Key::F8;
        case SDLK_F9:           return CEGUI::Key::F9;
        case SDLK_F10:          return CEGUI::Key::F10;
        case SDLK_F11:          return CEGUI::Key::F11;
        case SDLK_F12:          return CEGUI::Key::F12;
        case SDLK_F13:          return CEGUI::Key::F13;
        case SDLK_F14:          return CEGUI::Key::F14;
        case SDLK_F15:          return CEGUI::Key::F15;
        case SDLK_NUMLOCK:      return CEGUI::Key::NumLock;
        case SDLK_SCROLLOCK:    return CEGUI::Key::ScrollLock;
        case SDLK_RSHIFT:       return CEGUI::Key::RightShift;
        case SDLK_LSHIFT:       return CEGUI::Key::LeftShift;
        case SDLK_RCTRL:        return CEGUI::Key::RightControl;
        case SDLK_LCTRL:        return CEGUI::Key::LeftControl;
        case SDLK_RALT:         return CEGUI::Key::RightAlt;
        case SDLK_LALT:         return CEGUI::Key::LeftAlt;
        case SDLK_LSUPER:       return CEGUI::Key::LeftWindows;
        case SDLK_RSUPER:       return CEGUI::Key::RightWindows;
        case SDLK_SYSREQ:       return CEGUI::Key::SysRq;
        case SDLK_MENU:         return CEGUI::Key::AppMenu;
        case SDLK_POWER:        return CEGUI::Key::Power;
        default:                return 0;
    }

    conoutf("Warning: Cannot understand SDL key: %d\r\n", key);
    return 0;
}

//

CEGUI::Font* drawTextFont = NULL;

//

void init()
{
  try
  {
	Logging::log(Logging::DEBUG, "Initializing CEGUI\r\n");
	INDENT_LOG(Logging::DEBUG);

    // Rendering
    CEGUI::OpenGLRenderer* myRenderer = new CEGUI::OpenGLRenderer( 0 );
	Logging::log(Logging::DEBUG, "Renderer: %d\r\n", myRenderer != NULL);

#ifdef LINUX
    // Resources - we use a copy of DefaultResourceProvider, as CEGUI's version causes 'illegal instruction' crashes, oddly,
    // http://www.cegui.org.uk/phpBB2/viewtopic.php?t=3691&sid=d447e856b966dab7ede5bbdb73f3b23c
    // Just using it in this way, instead of CEGUI's DefaultResourceProvider, avoid the crash - not sure why.
    CEGUI::SimpleResourceProvider* rp = new CEGUI::SimpleResourceProvider();
    Logging::log(Logging::DEBUG, "Resources: %d\r\n", rp != NULL);

    // System
    Logging::log(Logging::DEBUG, "System\r\n");
    new CEGUI::System( myRenderer, rp );

#else // WINDOWS (OS X also?) Use the normal CEGUI DefaultResourceProvider - works fine
    Logging::log(Logging::DEBUG, "System\r\n");
    new CEGUI::System( myRenderer );

    CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>
       (CEGUI::System::getSingleton().getResourceProvider());
#endif

    rp->setResourceGroupDirectory("schemes",     "./datafiles/schemes/");
    rp->setResourceGroupDirectory("imagesets",   "./datafiles/imagesets/");
    rp->setResourceGroupDirectory("fonts",       "./datafiles/fonts/");
    rp->setResourceGroupDirectory("layouts",     "./datafiles/layouts/");
    rp->setResourceGroupDirectory("looknfeels",  "./datafiles/looknfeel/");
    rp->setResourceGroupDirectory("lua_scripts", "./datafiles/lua_scripts/");

    CEGUI::Imageset::setDefaultResourceGroup         ("imagesets");
    CEGUI::Font::setDefaultResourceGroup             ("fonts");
    CEGUI::Scheme::setDefaultResourceGroup           ("schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup    ("layouts");
    CEGUI::ScriptModule::setDefaultResourceGroup     ("lua_scripts");

    // Initialize scripting
    Logging::log(Logging::DEBUG, "Scripting\r\n");

	CEGUI::LuaScriptModule* script = new CEGUI::LuaScriptModule();
    CEGUI::System::getSingleton().setScriptingModule(script);

    // Init tolua++ package
    Logging::log(Logging::DEBUG, "tolua++\r\n");

    tolua__open ( script->getLuaState() );
//    tolua_sauer_cegui_open ( script->getLuaState() );

    // Lua preparations
    Logging::log(Logging::DEBUG, "Lua Library and Init scripts\r\n");

    CEGUI::System::getSingleton().executeScriptFile("Library.lua"); // General-purpose functions
    CEGUI::System::getSingleton().executeScriptFile("Init.lua");    // Initialization, before doing anything

    // Fonts
    Logging::log(Logging::DEBUG, "Fonts\r\n");

    drawTextFont = CEGUI::FontManager::getSingleton().getFont("DejaVuSans-10");

  } catch (CEGUI::Exception e)
  {
	  printf("CEGUI raised an exception during initialization, crashing the program: %s \r\n\r\nTo see more cause, look in CEGUI.log\r\n", e.getMessage().c_str());
	  throw;
  }
}

void quit()
{
//    delete CEGUI::System::getSingleton().getScriptingModule(); // Supposedly needed, but causes crash on exit
    CEGUI::Renderer* myRenderer = CEGUI::System::getSingleton().getRenderer();
    delete CEGUI::System::getSingletonPtr();
    delete myRenderer;
}

void toggleMainMenu()
{
printf("Flushing actions\r\n");
    PlayerControl::flushActions(); // Stop all actions, perhaps the player wanted to abort by pressing Esc
    CEGUI::System::getSingleton().executeScriptFile("ToggleMainMenu.lua");
}

void drawGui()
{
    // Advance the clock (for fading effects, etc.)
    static Uint32 last_ticks = 0;
    Uint32 new_ticks = SDL_GetTicks();
    CEGUI::System::getSingleton().injectTimePulse(float(new_ticks - last_ticks)/1000.0f);
    last_ticks = new_ticks;

    // Render the gui
    CEGUI::System::getSingleton().renderGUI();
}

void drawText(std::string str, int left, int top, int r, int g, int b, int a, int cursor, int maxwidth)
{
#if 0 // Ridiculously slow...
    if (drawTextFont)
        drawTextFont->drawText(
            str,
            CEGUI::Rect(left, top, getvar("scr_w")-1, getvar("scr_h")-1),
            1.0f
        );
    else
#endif
    Logging::log(Logging::INFO, "Writing drawText to console: %s\r\n", str.c_str());
}

bool handleKeypress(SDLKey sym, int unicode, bool isdown)
{
    Logging::log(Logging::INFO, "Handling (SDL, CEGUI, unicode, down) %d, %d, %d, %d", sym, SDLKeyToCEGUIKey(sym), unicode, isdown);

    bool handled = false;

    if (sym == 9) // When 9 (tab) is pressed with shift, oddly there is no unicode. HACK (FIXME?)
        unicode = 9;

    if (unicode && isdown)
    {
        Logging::log(Logging::INFO, "Injecting unicode %d", unicode);

        switch (unicode)
        {
            case 0x08:  // backspace
                handled = handled || CEGUI::System::getSingleton().injectKeyDown(CEGUI::Key::Backspace);
                break;
            case 0x7F:  // delete
                handled = handled || CEGUI::System::getSingleton().injectKeyDown(CEGUI::Key::Delete);
                break;
            case 0x1B:  // Escape
                handled = handled || CEGUI::System::getSingleton().injectKeyDown(CEGUI::Key::Escape); // FIXME: Should not be done...

                // Wipe any entity creations. TODO: really should do this in a more flexible manner.
                EditingSystem::QueuedEntity::_class = "";
                execute("create_new_ent"); // Wipes the mouse cursor, etc.

                toggleMainMenu();
                break;
            case 0x0D:  // CR (Return)
                handled = handled || CEGUI::System::getSingleton().injectChar(CEGUI::Key::Return);
                break;
            default:
                // inject Character code
                handled = handled || CEGUI::System::getSingleton().injectChar(static_cast<CEGUI::utf32>(unicode));
                break;
        }

        // For character entry, we do a small sound. Probably bad for tabs, though
        if (unicode != 9 && handled)
        {
            if (ClientSystem::playerLogicEntity.get())
            {
                vec playerLoc = ClientSystem::playerLogicEntity.get()->getOrigin(); // FIXME: Shouldn't need to do this.
                                                                                    // Need a SoundSystem class...
                playsound(15, &playerLoc); // TODO: Better sound system, no hardcoded codes
            }
        }
    }
    else
    {
        Logging::log(Logging::INFO, "Injecting keydown or keyup\r\n");
        if (isdown)
            handled = handled || CEGUI::System::getSingleton().injectKeyDown( SDLKeyToCEGUIKey(sym) );
        else
            handled = handled || CEGUI::System::getSingleton().injectKeyUp  ( SDLKeyToCEGUIKey(sym) );
    }

    if (!handled) // If the layout has a modal window, we 'handle' events so Sauer doesn't get them
        handled = CEGUI::System::getSingleton().getScriptingModule()->executeScriptGlobal("isLayoutModal");

    Logging::log(Logging::INFO, "Handled: %d\r\n", handled);

    return handled;
}

bool handleMousebuttonupEvent(Uint8 SDLbutton)
{
    bool handled = false;
    // Convert SDL codes to CEGUI ones
    switch (SDLbutton)
    {
        case SDL_BUTTON_LEFT   :
            handled = CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::LeftButton);
            break;
        case SDL_BUTTON_MIDDLE :
            handled =  CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::MiddleButton);
            break;
        case SDL_BUTTON_RIGHT  :
            handled =  CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::RightButton);
            break;
        case 4 : // Hack, but there is no SDL notation for this!
            handled =  CEGUI::System::getSingleton().injectMouseWheelChange(-0.1f);
            break;
        case 5 : // Hack, but there is no SDL notation for this!
            handled =  CEGUI::System::getSingleton().injectMouseWheelChange(+0.1f);
            break;
        default:
            Logging::log(Logging::WARNING, "Odd, a non-standard SDL mousebutton pressed up: %d\r\n", SDLbutton);
            handled =  false;
    }

    if (!handled) // If the layout has a modal window, we 'handle' events so Sauer doesn't get them
        handled = CEGUI::System::getSingleton().getScriptingModule()->executeScriptGlobal("isLayoutModal");

    Logging::log(Logging::INFO, "IntensityCEGUI handling upclick: %d\r\n", handled);

    return handled;
}

bool handleMousebuttondownEvent(Uint8 SDLbutton)
{
    bool handled = false;
    // Convert SDL codes to CEGUI ones

    switch (SDLbutton)
    {
        case SDL_BUTTON_LEFT   :
            handled = CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::LeftButton);
            break;
        case SDL_BUTTON_MIDDLE :
            handled =  CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::MiddleButton);
            break;
        case SDL_BUTTON_RIGHT  :
            handled =  CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::RightButton);
            break;
        case 4 : // Hack, but there is no SDL notation for this!
            handled =  CEGUI::System::getSingleton().injectMouseWheelChange(-0.1f);
            break;
        case 5 : // Hack, but there is no SDL notation for this!
            handled =  CEGUI::System::getSingleton().injectMouseWheelChange(+0.1f);
            break;
        default:
            Logging::log(Logging::WARNING, "Odd, a non-standard SDL mousebutton pressed down: %d\r\n", SDLbutton);
            handled =  false;
    }

    if (!handled) // If the layout has a modal window, we 'handle' events so Sauer doesn't get them
        handled = CEGUI::System::getSingleton().getScriptingModule()->executeScriptGlobal("isLayoutModal");

    if (!handled)
        CEGUI::System::getSingleton().executeScriptFile("HandleOutsideClick.lua");

    Logging::log(Logging::INFO, "IntensityCEGUI handling downclick: %d\r\n", handled);

    return handled;
}


//! Some sauer-language commands to run CEGUI actions from inside sauer .cfg files

//! show_layout: Shows one of the major layouts: login screen, in-game stuff, character creation, etc.
//! We call a lua script with the same name (+".lua") that shows the layout

void show_layout(char *layoutName)
{
    std::string luaName = layoutName + std::string(".lua");
    CEGUI::System::getSingleton().executeScriptFile(luaName);
}

COMMAND(show_layout, "s");

void showLayout(std::string layout)
{
    show_layout((char *)layout.c_str());
}


//
// Entity Editing
//

LogicEntityPtr EditedEntity::currEntity;

EditedEntity::StateDataMap EditedEntity::stateData;

std::vector<std::string> EditedEntity::stateDataKeys;

unsigned int EditedEntity::currStateDataIndex;

//! Upon clicking on a LogicEntity in edit mode, shows a gui for editing that entity's properties

void showEditEntityGUI()
{
    // Prepare state data that CEGUI can read and place, as well as C++ stores for comparison of changed values.
    // We do not send this with executeScriptString, because that is vulnerable to injection attacks

    EditedEntity::stateData.clear();
    EditedEntity::stateDataKeys.clear();
    EditedEntity::currStateDataIndex = 0;

    if ( !TargetingControl::targetLogicEntity.get()->isNone() )
    {
        EditedEntity::currEntity = TargetingControl::targetLogicEntity;

        python::object pythonStateData = python::object(EditedEntity::currEntity->scriptEntity->getPropertyString("stateData"));
        python::object keys = pythonStateData.attr("keys")();

        std::string _class = EditedEntity::currEntity->scriptEntity->getPropertyString("_class");

        REFLECT_PYTHON( len );
//        REFLECT_PYTHON( get_state_data_gui_name ); FIXME

        for (int i = 0; i < python::extract<int>(len(keys)); i++)
        {

            std::string key = python::extract<std::string>( keys[i] );
            std::string guiName = key; // python::extract<std::string>( get_state_data_gui_name(_class, key) ); FIXME

            EditedEntity::stateData.insert(
                EditedEntity::StateDataMap::value_type(
                    key,
                    std::pair<std::string, std::string>(
                        guiName,
                        python::extract<std::string>( pythonStateData[key] )
                    )
                )
            );

            std::string a = python::extract<std::string>( keys[i] );
            std::string b = python::extract<std::string>( pythonStateData[keys[i]] );

            EditedEntity::stateDataKeys.push_back( python::extract<std::string>( keys[i] ) );
        }

        sort( EditedEntity::stateDataKeys.begin(), EditedEntity::stateDataKeys.end() );

        CEGUI::System::getSingleton().executeScriptGlobal("initEditedEntityValues");
    } else {
        EditedEntity::currEntity = LogicEntityPtr();

        CEGUI::System::getSingleton().executeScriptGlobal("hideEditedEntityValues");
    }
}

void editentity_gui()
{
    showEditEntityGUI();
}

COMMAND(editentity_gui, "");


std::string IntensityCEGUI::QueuedMessage::title = "-";
std::string IntensityCEGUI::QueuedMessage::text = "--";

void showMessage(std::string title, std::string text, bool chat)
{
    IntensityCEGUI::QueuedMessage::title = title;
    IntensityCEGUI::QueuedMessage::text = text;

    // Done like this to prevent Lua injection attacks
    if (!chat)
        CEGUI::System::getSingleton().executeScriptGlobal("triggerShowMessage");
    else
        CEGUI::System::getSingleton().executeScriptGlobal("triggerAddChatText");
}

void runLuaFunction(std::string command)
{
    CEGUI::System::getSingleton().executeScriptGlobal(command);
}

}


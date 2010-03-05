
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


#include "cube.h"
#include "engine.h"
#include "game.h"

#include "message_system.h"

#include "world_system.h"
#include "fpsserver_interface.h"
#include "editing_system.h"
#include "script_engine_manager.h"
#include "utility.h"
#include "client_engine_additions.h"
#include "targeting.h"
#include "intensity_gui.h"
#include "intensity_texture.h"
#include "master.h"
#ifdef INTENSITY_PLUGIN
    #include "intensity_plugin_listener.h"
#endif

#include "client_system.h"

#include "shared_module_members_boost.h"


std::string ClientSystem::blankPassword = "1111111111"; // TODO: We should ensure the users can never have this for a real password!
                                                        // Note: Sending CEGUI characters that are invalid to enter in the field might
                                                        // seem like a nice solution here, but CEGUI has issues with that

int            ClientSystem::playerNumber       = -1;
LogicEntityPtr ClientSystem::playerLogicEntity;
bool           ClientSystem::loggedIn           = false;
bool           ClientSystem::editingAlone       = false;
int            ClientSystem::uniqueId           = -1;
std::string    ClientSystem::currMap            = "";
std::string ClientSystem::currTransactionCode = "MISSING_TRANSACTION_CODE";
std::string ClientSystem::currHost = "";
int ClientSystem::currPort = -1;
std::string ClientSystem::currScenarioCode = "";

bool _scenarioStarted = false;
bool _mapCompletelyReceived = false;


#define USER_INFO_SECTION "UserInfo"
#define VIDEO_SECTION "Video"

std::string ClientSystem::getUsername()
{
    return Utility::Config::getString(USER_INFO_SECTION, "username", "");
}

std::string ClientSystem::getHashedPassword()
{
    return Utility::Config::getString(USER_INFO_SECTION, "password", "");
}

std::string ClientSystem::getVisualPassword()
{
    if (getHashedPassword() != "")
        return blankPassword; // Visually, we show just 8 or so letters, not the entire hash of course
    else
        return "";
}

void ClientSystem::connect(std::string host, int port)
{
//    // Tell scripting there is no player entity until we finish logging in
//    ScriptEngineManager::getGlobal()->call("setPlayerUniqueId", ScriptEngineManager::getNull());


    editingAlone = false;

    currHost = host;
    currPort = port;

    connectserv((char *)host.c_str(), port, "");
}

void ClientSystem::login(int clientNumber)
{
    Logging::log(Logging::DEBUG, "ClientSystem::login()\r\n");

    playerNumber = clientNumber;

    MessageSystem::send_LoginRequest(currTransactionCode);
//        Utility::Config::getString(USER_INFO_SECTION, "username", "*error1*"),
//        Utility::Config::getString(USER_INFO_SECTION, "password", "*error2*")
//    );
}

void ClientSystem::finishLogin(bool local)
{
    editingAlone = local;
    loggedIn = true;

    // There is no Python LE for the player yet, we await its arrival from the server just like any other LE. When
    // it does arrive, we will point playerLogicEntity to it. See messages.template.
 
    Logging::log(Logging::DEBUG, "Now logged in, with unique_ID: %d\r\n", uniqueId);
}

void ClientSystem::doDisconnect()
{
    disconnect();
}

void ClientSystem::onDisconnect()
{
    editingAlone = false;
    playerNumber = -1;
    loggedIn     = false;
    _scenarioStarted  = false;
    _mapCompletelyReceived = false;

    LogicSystem::clear();
}

void ClientSystem::clearPlayerEntity()
{
    Logging::log(Logging::DEBUG, "ClientSystem::clearPlayerEntity\r\n");
    playerLogicEntity.reset();
}

void ClientSystem::sendSavedMap()
{
    assert(0); // Deprecated
}

/*
bool ClientSystem::mapCompletelyReceived()
{
    return _mapCompletelyReceived;
}
*/

bool ClientSystem::scenarioStarted()
{
    if (!_mapCompletelyReceived)
        Logging::log(Logging::INFO, "Map not completely received, so scenario not started\r\n");

    // If not already started, test if indeed started
    if (_mapCompletelyReceived && !_scenarioStarted)
    {
        if (ScriptEngineManager::hasEngine())
            _scenarioStarted = ScriptEngineManager::getGlobal()->call("testScenarioStarted")->getBool();
    }

    return _mapCompletelyReceived && _scenarioStarted;
}

VAR(can_edit, 0, 0, 1);

void ClientSystem::frameTrigger(int curtime)
{
    if (scenarioStarted())
    {
        PlayerControl::handleExtraPlayerMovements(curtime);
        TargetingControl::determineMouseTarget();
        can_edit = int(isAdmin());
        IntensityTexture::doBackgroundLoading();
    }

    ClientSystem::cleanupHUD();

    #ifdef INTENSITY_PLUGIN
        PluginListener::frameTrigger();
    #endif
}

void ClientSystem::gotoLoginScreen()
{
    assert(0);
#if 0
    Logging::log(Logging::DEBUG, "Going to login screen\r\n");
    INDENT_LOG(Logging::DEBUG);

    LogicSystem::init(); // This is also done later, but as the mainloop assumes there is always a ScriptEngine, we do it here as well

    ClientSystem::onDisconnect(); // disconnect has several meanings...

    localconnect();
    game::gameconnect(false);

//    FPSServerInterface::vote("login", 0);

    game::changemap("login");

    Logging::log(Logging::DEBUG, "Going to login screen complete\r\n");
#endif
}

// Boost access for Python

extern void sethomedir(const char *dir); // shared/tools.cpp

int saved_argc = 0;
char **saved_argv = NULL;

extern int sauer_main(int argc, char **argv); // from main.cpp

void client_main()
{
    Logging::log(Logging::DEBUG, "client_main");

    sauer_main(saved_argc, saved_argv);
}

void setTransactionCode(std::string code)
{
    ClientSystem::currTransactionCode = code;
}

void show_message(std::string title, std::string content)
{
    IntensityGUI::showMessage(title, content);
    printf("%s : %s\r\n", title.c_str(), content.c_str());
}


//
// HUD
//

struct queuedHUDRect
{
    float x1, y1, x2, y2;
    int color;
};

std::vector<queuedHUDRect> queuedHUDRects;

void ClientSystem::addHUDRect(float x1, float y1, float x2, float y2, int color)
{
    queuedHUDRect q;
    q.x1 = x1;
    q.y1 = y1;
    q.x2 = x2;
    q.y2 = y2;
    q.color = color;
    queuedHUDRects.push_back(q);
}

struct queuedHUDImage
{
    std::string tex;
    float centerX, centerY; //!< In relative coordinates (to each axis, the center of where to draw the HUD
//    float widthInX, heightInY; //!< In axis-relative coordinates, how big the HUD should be.
//                               //!< E.g. widthInX 0.5 means its width is half of the X dimension
    float width, height;

    queuedHUDImage()
    {
        tex = "";
        centerX = 0.5; centerY = 0.5;
//        widthInX = 0; heightInY = 0;
        width = 0.61803399; height = 0.61803399;
    }
};

std::vector<queuedHUDImage> queuedHUDImages;

void ClientSystem::addHUDImage(std::string tex, float centerX, float centerY, float width, float height)
{
    queuedHUDImage q;
    q.tex = tex;
    q.centerX = centerX;
    q.centerY = centerY;
    q.width = width;
    q.height = height;
    queuedHUDImages.push_back(q);
}

struct queuedHUDText
{
    std::string text;
    float x, y, scale;
    int color;
};

std::vector<queuedHUDText> queuedHUDTexts;

void ClientSystem::addHUDText(std::string text, float x, float y, float scale, int color)
{
    queuedHUDText q;
    q.text = text;
    q.x = x;
    q.y = y;
    q.scale = scale;
    q.color = color;
    queuedHUDTexts.push_back(q);
}

void ClientSystem::drawHUD(int w, int h)
{
    if (g3d_windowhit(true, false)) return; // Showing sauer GUI - do not show HUD

    float wFactor = float(h)/max(w,h);
    float hFactor = float(w)/max(w,h);

    // Rects

    glPushMatrix();
    glScalef(w, h, 1);

    for (unsigned int i = 0; i < queuedHUDRects.size(); i++)
    {
        queuedHUDRect& q = queuedHUDRects[i];
        if (q.x2 < 0)
        {
            float x1 = q.x1, y1 = q.y1;
            q.x1 = x1 - wFactor*fabs(q.x2)/2;
            q.y1 = y1 - hFactor*fabs(q.y2)/2;
            q.x2 = x1 + wFactor*fabs(q.x2)/2;
            q.y2 = y1 + hFactor*fabs(q.y2)/2;
        }

        vec rgb(q.color>>16, (q.color>>8)&0xFF, q.color&0xFF);
        rgb.mul(1.0/256.0);
        glColor3f(rgb[0], rgb[1], rgb[2]);

        glDisable(GL_TEXTURE_2D);
        notextureshader->set();
        glBegin(GL_QUADS);
        glVertex2f(q.x1, q.y1);
        glVertex2f(q.x2, q.y1);
        glVertex2f(q.x2, q.y2);
        glVertex2f(q.x1, q.y2);
        glEnd();
        glEnable(GL_TEXTURE_2D);
        defaultshader->set();	
    }

    glPopMatrix();

    // Images

    glPushMatrix();
    glScalef(w, h, 1);

    for (unsigned int i = 0; i < queuedHUDImages.size(); i++)
    {
        queuedHUDImage& q = queuedHUDImages[i];
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float x1 = q.centerX - (wFactor*q.width/2);
        float y1 = q.centerY - (hFactor*q.height/2);
        float x2 = q.centerX + (wFactor*q.width/2);
        float y2 = q.centerY + (hFactor*q.height/2);
        glColor3f(1, 1, 1);
        settexture(q.tex.c_str(), 3);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(x1, y1);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(x2, y1);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(x2, y2);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(x1, y2);
        glEnd();
    }

    glPopMatrix();

    // Texts

    for (unsigned int i = 0; i < queuedHUDTexts.size(); i++)
    {
        queuedHUDText& q = queuedHUDTexts[i];

        glPushMatrix();
        glScalef(q.scale, q.scale, 1);

        int b = q.color & 255;
        q.color = q.color >> 8;
        int g = q.color & 255;
        int r = q.color >> 8;

        draw_text(q.text.c_str(), w*q.x/q.scale - text_width(q.text.c_str())/2, h*q.y/q.scale - FONTH/2, r, g, b);

        glPopMatrix();
    }
}

void ClientSystem::cleanupHUD()
{
    queuedHUDRects.clear();
    queuedHUDImages.clear();
    queuedHUDTexts.clear();
}

int get_escape()
{
    return SDLK_ESCAPE;
}

extern void checkinput();

void upload_texture_data(std::string name, int x, int y, int w, int h, long long int pixels)
{
    IntensityTexture::uploadTextureData(name, x, y, w, h, (void*)pixels);
}


//! Main starting point - initialize Python, set up the embedding, and
//! run the main Python script that sets everything in motion
int main(int argc, char **argv)
{
    saved_argc = argc;
    saved_argv = argv;

    initPython(argc, argv);

    // Expose client-related functions to Python

    exposeToPython("main", client_main);
    exposeToPython("set_transaction_code", setTransactionCode);
    exposeToPython("connect", ClientSystem::connect);
    exposeToPython("disconnect", ClientSystem::doDisconnect);
    exposeToPython("logout", MasterServer::logout);
    exposeToPython("show_message", show_message);
    exposeToPython("intercept_key", interceptkey);
    exposeToPython("get_escape", get_escape);

    exposeToPython("inject_mouse_position", IntensityGUI::injectMousePosition);
    exposeToPython("inject_mouse_click", IntensityGUI::injectMouseClick);
    exposeToPython("inject_key_press", IntensityGUI::injectKeyPress);
    exposeToPython("flush_input_events", checkinput);

    exposeToPython("upload_texture_data", upload_texture_data);

    // Shared exposed stuff stuff with the server module
    #include "shared_module_members.boost"

    // Start the main Python script that runs it all
    EXEC_PYTHON_FILE("../../intensity_client.py");

    return 0;
}

void ClientSystem::finishLoadWorld()
{
    extern bool finish_load_world();
    finish_load_world();

    _mapCompletelyReceived = true; // We have the original map + static entities (still, scenarioStarted might want more stuff)

    EditingSystem::madeChanges = false; // Clean the slate

    ClientSystem::editingAlone = false; // Assume not in this mode

    mainmenu = 0; // (see prepareForMap)
}

void ClientSystem::prepareForNewScenario(std::string scenarioCode)
{
    _mapCompletelyReceived = false; // We no longer have a map. This implies scenarioStarted will return false, thus
                                    // stopping sending of position updates, as well as rendering

    mainmenu = 1; // Keep showing GUI meanwhile (in particular, to show the message about a new map on the way

    // Clear the logic system, as it is no longer valid - were it running, we might try to process messages from
    // the new map being set up on the server, even though they are irrelevant to the existing engine, set up for
    // another map with its Classes etc.
    LogicSystem::clear();

    currScenarioCode = scenarioCode;
}

void ClientSystem::handleConfigSettings()
{
    assert(0);
#if 0
    // Shaders
    extern int useshaders, shaderprecision; 
    int n = Utility::Config::getInt(VIDEO_SECTION, "shaders", 2);
    useshaders = n ? 1 : 0;
    shaderprecision = min(max(n - 1, 0), 3);

    // Fullscreen
    extern int fullscreen;
    fullscreen = Utility::Config::getInt(VIDEO_SECTION, "fullscreen", 0);

    extern int scr_w, scr_h;
    scr_w = Utility::Config::getInt(VIDEO_SECTION, "screen_width", 1024);
    scr_h = Utility::Config::getInt(VIDEO_SECTION, "screen_height", 768);

    // Shadows
    std::string shadows = Utility::Config::getString(VIDEO_SECTION, "shadows", "medium");
    extern int dynshadow, shadowmap, shadowmapsize, blurshadowmap;
    if (shadows == "none")
    {
        dynshadow = 0;
        shadowmap = 0;
    } else if (shadows == "low")
    {
        dynshadow = 60;
        shadowmap = 1;
        shadowmapsize = 8;
        blurshadowmap = 1;
    } else if (shadows == "medium")
    {
        dynshadow = 60;
        shadowmap = 1;
        shadowmapsize = 9;
        blurshadowmap = 1;
    } else if (shadows == "high")
    {
        dynshadow = 60;
        shadowmap = 1;
        shadowmapsize = 10;
        blurshadowmap = 2;
    } else if (shadows == "ultra")
    {
        dynshadow = 60;
        shadowmap = 1;
        shadowmapsize = 11;
        blurshadowmap = 2;
    } else {
        printf("Invalid value for Video::shadows; valid values are among: none, low, medium, high, ultra\r\n");
    }
#endif
}

bool ClientSystem::isAdmin()
{
    if (!loggedIn) return false;
    if (!playerLogicEntity.get()) return false;

    if ( !playerLogicEntity.get()->scriptEntity->hasProperty("_canEdit") ) return false;

    return playerLogicEntity.get()->scriptEntity->getPropertyBool("_canEdit");
}


// cubescript stuff

void connect_to_instance(char *instance_id)
{
    REFLECT_PYTHON( login_to_instance );

    std::string instanceId = instance_id;

    login_to_instance(instanceId);
}

COMMAND(connect_to_instance, "s");


void connect_to_lobby()
{
    REFLECT_PYTHON_ALTNAME( connect_to_lobby, ctl );
    ctl();
}
COMMAND(connect_to_lobby, "");


void connect_to_selected_instance()
{
    REFLECT_PYTHON_ALTNAME( connect_to_selected_instance, ctji );
    ctji();
}
COMMAND(connect_to_selected_instance, "");


// Get instance data and create a GUI for it
void show_instances()
{
    REFLECT_PYTHON( get_possible_instances );

    boost::python::object instances = get_possible_instances();

    REFLECT_PYTHON( None );

    if (instances == None)
    {
        setsvar("error_message", "Could not get the list of instances");
        showgui("error");
        return;
    }

    std::string command =
        "newgui instances [\n"
        "    guitext \"Pick an instance to enter:\"\n"
        "    guibar\n";

    int numInstances = boost::python::extract<int>(instances.attr("__len__")());

    for (int i = 0; i < numInstances; i++)
    {
        boost::python::object instance = instances[i];
        std::string instance_id = boost::python::extract<std::string>(instance.attr("__getitem__")("instance_id"));
        std::string event_name = boost::python::extract<std::string>(instance.attr("__getitem__")("event_name"));

        assert( Utility::validateAlphaNumeric(instance_id) );
        assert( Utility::validateAlphaNumeric(event_name, " (),.;") ); // XXX: Allow more than alphanumeric+spaces: ()s, .s, etc.

        command += "    guibutton \"" + event_name + "\" \"connect_to_instance " + instance_id + "\"\n";
    }

    command += "]\n";
    command += "showgui instances\n";

    Logging::log(Logging::DEBUG, "Instances GUI: %s\r\n", command.c_str());

    execute(command.c_str());
}

COMMAND(show_instances, "");


//! Tries to compile a script, and prints errors if any
bool checkCompile(std::string filename)
{
    std::string script = Utility::readFile(filename);
    std::string errors = ScriptEngineManager::compileScript(script);

    if (errors != "")
    {
        IntensityGUI::showMessage("Compilation failed", errors);
        return false;
    } else {
        return true;
    }
}

//! The asset ID of the last saved map. This is useful if we want to reload it (if it
//! crashed the server, for example
SVARP(last_uploaded_map_asset, "");

//! The user picks 'upload map' in the GUI.
//! First we save the map. Then we call Python, which packages the map
//! and uploads it to the correct asset server, then notify the instance we
//! are connected to that the map has been updated, which then gets and runs
//! that new map. That process causes it to tell all clients of a new map that
//! they should get, which makes them get the new version. Among those clients is
//! this one, the uploader, which we do not treat differently in that respect.
void do_upload()
{
    renderprogress(0.1, "compiling scripts...");

    // Make sure the script compiles ok TODO: All scripts, not just the main one
    REFLECT_PYTHON( get_map_script_filename );
    std::string filename = boost::python::extract<std::string>( get_map_script_filename() );
    if (!checkCompile(filename))
        return;

    // Save ogz
    renderprogress(0.3, "generating map...");
    save_world(game::getclientmap());

// load_world: ogzname, mname, cname: packages/base/spiral/map.ogz,base/spiral/map,(null)
// save_world ogzname, mname, cname: packages//packages.ogz,/packages

    // Save entities (and backup)
    renderprogress(0.4, "exporting entities...");
    REFLECT_PYTHON( export_entities );
    export_entities("entities.json");

    // Do the upload
    renderprogress(0.5, "uploading map...");
    REFLECT_PYTHON( upload_map );
    upload_map();

    // Remember asset
    REFLECT_PYTHON( get_curr_map_asset_id );
    std::string assetId = boost::python::extract<std::string>( get_curr_map_asset_id() );
    setsvar("last_uploaded_map_asset", assetId.c_str());
}

COMMAND(do_upload, "");

//! Upload the last map once more. This does NOT save the world - it can be called without
//! even having a world loaded (e.g., from the main menu, right after startup). It simply
//! packages the files in the map directory and uploads them. This is useful for making
//! fixes to map scripts that crash the server, but can also be used after manually
//! replacing the .ogz file, etc.
void repeat_upload()
{
    std::string lastUploadedMapAsset = last_uploaded_map_asset;

    // Acquire the asset info, so we know its file locations, asset server, etc.
    renderprogress(0.2, "getting map asset info...");
    REFLECT_PYTHON( AssetManager );
    boost::python::object assetInfo = AssetManager.attr("get_info")( lastUploadedMapAsset );

    // Set the map asset ID to the last one uploaded
    REFLECT_PYTHON( set_curr_map_asset_id );
    set_curr_map_asset_id( lastUploadedMapAsset );
    REFLECT_PYTHON( World );
    World.attr("asset_info") = assetInfo;

//    // Make sure the script compiles ok TODO: All scripts, not just the main one
//    renderprogress(0.5, "compiling scripts...");
//    REFLECT_PYTHON( get_map_script_filename );
//    std::string filename = boost::python::extract<std::string>( get_map_script_filename() );
//    if (!checkCompile(filename)) XXX - need engine for this!
//        return;

    // Do the upload
    renderprogress(0.7, "compiling scripts...");
    REFLECT_PYTHON( upload_map );
    upload_map();

    conoutf("Upload complete.");
}

COMMAND(repeat_upload, "");

// Restart server/map
void restart_map()
{
    MessageSystem::send_RestartMap();
}

COMMAND(restart_map, "");


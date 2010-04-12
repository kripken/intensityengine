
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "game.h"
#include "crypto.h"

#include "intensity.h"
#include "character_render.h"
#include "client_system.h"
#include "script_engine_manager.h"


namespace game
{      
    void rendergame(bool mainpass)
    {
        if (!ClientSystem::loggedIn) // If not logged in remotely, do not render, because entities lack all the fields like model_name
                                     // in the future, perhaps add these, if we want local rendering
        {
            Logging::log(Logging::INFO, "Not logged in remotely, so not rendering\r\n");
            return;
        }

        startmodelbatches();

#if 0
        fpsent *exclude = isthirdperson() ? NULL : followingplayer(), *d;
        loopv(players)
        {
            d = players[i];
            if (d != player1 && d->state!=CS_SPECTATOR && d->state!=CS_SPAWNING && d!=exclude)
                CharacterRendering::render(d); // INTENSITY
        }
        if(isthirdperson() && !followingplayer())
        {
            Logging::log(Logging::INFO, "Rendering self\r\n");
            CharacterRendering::render(player1); // INTENSITY
        }


        /* TODO:
            // INTENSITY: Class above head in edit mode
            if (editmode)
            {
                std::string _class = '@' + logicEntity.get()->getClass(); // '@' makes Sauer create a copy
                particle_text(entity->abovehead(), _class.c_str(), 16, 1);
            }
        */
#else // Scripting rendering system
//        fpsent *exclude = isthirdperson() ? NULL : followingplayer(), *d; // XXX: Apply this!
//        if(isthirdperson() && !followingplayer()) // XXX Apply this!

        ScriptEngineManager::getGlobal()->call("renderDynamic", isthirdperson());
#endif

//        ExtraRendering::renderShadowingMapmodels(); // Kripken: Mapmodels with dynamic shadows, we draw them now
        entities::renderentities();

        endmodelbatches();
    }

    int swaymillis = 0;
    vec swaydir(0, 0, 0);

    void swayhudgun(int curtime)
    {
        fpsent *d = hudplayer();
        if(d->state!=CS_SPECTATOR)
        {
            if(d->physstate>=PHYS_SLOPE) swaymillis += curtime;
            float k = pow(0.7f, curtime/10.0f);
            swaydir.mul(k);
            vec vel(d->vel);
            vel.add(d->falling);
            swaydir.add(vec(vel).mul((1-k)/(15*max(vel.magnitude(), d->maxspeed))));
        }
    }

    void renderavatar()
    {
        ScriptEngineManager::getGlobal()->call("renderHUDModels");
    }
}


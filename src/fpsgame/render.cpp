
/*
 *=============================================================================
 * Copyright (C) 2001-2006 Wouter van Oortmerssen.
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


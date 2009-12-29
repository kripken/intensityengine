// creation of scoreboard
#include "cube.h"
#include "engine.h"
#include "game.h"

#include "network_system.h"
#include "script_engine_manager.h"
#include "utility.h"


namespace game
{
    VARP(scoreboard2d, 0, 1, 1);
    VARP(showpj, 0, 1, 1); // Kripken
    VARP(showping, 0, 1, 1);
    VARP(showspectators, 0, 1, 1);

    void renderscoreboard(g3d_gui &g, bool firstpass)
    {
        const char *mname = getclientmap();
        defformatstring(modemapstr)("%s: %s", "Syntensity", mname[0] ? mname : "[new map]");

        g.text(modemapstr, 0xFFFF80, "server");

        g.pushlist(); // vertical
        g.pushlist(); // horizontal
            g.background(0x808080, 5);

            ScriptValuePtr text = ScriptEngineManager::getGlobal()->getProperty("ApplicationManager")->getProperty("instance")->call("getScoreboardText");
            int numLines = text->getPropertyInt("length");

            for (int i = 0; i < numLines; i++)
            {
                ScriptValuePtr lineData = text->getProperty(Utility::toString(i));
                int lineUniqueId = lineData->getPropertyInt("0");
                std::string lineText = lineData->getPropertyString("1");

                if (lineUniqueId != -1)
                {
                    LogicEntityPtr entity = LogicSystem::getLogicEntity(lineUniqueId);
                    if (entity.get())
                    {
                        fpsent *p = dynamic_cast<fpsent*>(entity->dynamicEntity);
                        assert(p);

                        if (showpj)
                        {
                            if (p->state == CS_LAGGED)
                                lineText += "LAG";
                            else
                                lineText += " pj: " + Utility::toString(p->plag);
                        }
                        if (!showpj && p->state == CS_LAGGED)
                            lineText += "LAG";
                        else
                            lineText += " p: " + Utility::toString(p->ping);
                    }
                }

                g.text(lineText.c_str(), 0xFFFFDD, NULL);
            }
        g.poplist();
        g.poplist();

        // Show network stats
        static int laststatus = 0; 
        float seconds = float(totalmillis-laststatus)/1024.0f;
        static std::string netStats = "";
        if (seconds >= 0.5)
        {
            laststatus = totalmillis;
            netStats = NetworkSystem::Cataloger::briefSummary(seconds);
        }
        g.text(netStats.c_str(), 0xFFFF80, "server");
    }

    struct scoreboardgui : g3d_callback
    {
        bool showing;
        vec menupos;
        int menustart;

        scoreboardgui() : showing(false) {}

        void show(bool on)
        {
            if(!showing && on)
            {
                menupos = menuinfrontofplayer();
                menustart = starttime();
            }
            showing = on;
        }

        void gui(g3d_gui &g, bool firstpass)
        {
            g.start(menustart, 0.03f, NULL, false);
            renderscoreboard(g, firstpass);
            g.end();
        }

        void render()
        {
            if(showing) g3d_addgui(this, menupos, scoreboard2d ? GUI_FORCE_2D : GUI_2D | GUI_FOLLOW);
        }

    } scoreboard;

    void g3d_gamemenus()
    {
        scoreboard.render();
    }

    VARFN(scoreboard, showscoreboard, 0, 0, 1, scoreboard.show(showscoreboard!=0));

    void showscores(bool on)
    {
        showscoreboard = on ? 1 : 0;
        scoreboard.show(on);
    }
    ICOMMAND(showscores, "D", (int *down), showscores(*down!=0));
}


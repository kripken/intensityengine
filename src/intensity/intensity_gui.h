
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

namespace IntensityGUI
{
    extern void showMessage(std::string title, std::string content, int originClientNumber=-1);

    extern void showInputDialog(std::string title, std::string content);

    //! Check whether we can quit. We might want to say no and show a warning about
    //! unsaved changes, etc.
    bool canQuit();

    //! Simulates the user moving the mouse to position (x,y), in [0,1] coordinates
    //! (set logging level to DEBUG to see the SDL events from your interaction, which you can then plug in here)
    void injectMousePosition(float x, float y, bool immediate=false);

    //! Simulates the user clicking the mouse
    //! (set logging level to DEBUG to see the SDL events from your interaction, which you can then plug in here)
    void injectMouseClick(int button, bool down);

    //! Simulates the user pressing a key
    //! (set logging level to DEBUG to see the SDL events from your interaction, which you can then plug in here)
    void injectKeyPress(int sym, int unicode, bool down);
}



// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

namespace IntensityGUI
{
    extern void setKeyRepeat(int delay, int interval);

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
    void injectKeyPress(int sym, int unicode, bool down, bool isRepeat);
}


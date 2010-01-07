
#=============================================================================
# Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
#
# This file is part of the Intensity Engine project,
#    http://www.intensityengine.com
#
# The Intensity Engine is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, version 3.
#
# The Intensity Engine is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with the Intensity Engine.  If not, see
#     http://www.gnu.org/licenses/
#     http://www.gnu.org/licenses/agpl-3.0.html
#=============================================================================


from __future__ import with_statement

import sys, threading, functools, time

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtWebKit import *

from intensity.base import *
from intensity.logging import *
from intensity.signals import signal_component


# Constants

FPS = 25
#MAX_BROWSERS = 10

# Globals

class Browser:
    def __init__(self, texture, url):
        ## Texture is a unique identifier of a Browser (makes no sense to render twice to the same texture!)
        self.texture = texture
        self.url = url
        self.webview = None
        self.width = int(texture.split('/')[3]) # Assuming here that the texture is slt: packages/models/videoscreen/1024/...
        self.height = int(self.width*0.75) # Assuming here 4/3 aspect ratio
        self.image = QImage(self.width, self.height, QImage.Format_RGB888)
        self.bits = int(self.image.bits())

        self.events = []
        self.active = True

class Browsers:
    lock = threading.Lock()
    browsers = []
    old_frame_done = True
    events_pending = False

# Main loop

def main_loop():
    # Main setup - done in same thread that will do Qt commands
    app = QApplication(sys.argv)

    web_settings = QWebSettings.globalSettings()
    web_settings.setAttribute(QWebSettings.PluginsEnabled, True) # Enables Flash - but cannot see it yet, only hear it

    old_time = time.time()
    while True:
        time.sleep(0.01) # Always sleep a tiny bit, no matter what
        while (time.time() - old_time < 1/float(FPS) and not Browsers.events_pending) or not Browsers.old_frame_done:
            time.sleep(0.1/float(FPS))
#        print "Slept:", time.time() - old_time
        old_time = time.time()
        Browsers.old_frame_done = False

        temp_browsers = []

        with Browsers.lock:
#            assert(len(Browsers.browsers) < MAX_BROWSERS)
            Browsers.browsers = filter(lambda browser: browser.active, Browsers.browsers)

            for browser in Browsers.browsers:
                for event in browser.events:
                    data = event
                    parts = data.split('|')
                    command = parts[0]
                    params = '|'.join(parts[1:])
                    if command == 'click':
                        texture, x, y, button, down = params.split('|')
#                        print "click", x, y, button, down
                        point = QPoint(int(float(x)*browser.width),int(float(y)*browser.height))
                        button = get_button(int(button))
                        buttons = Qt.MouseButtons(button)
                        keyboard_modifiers = Qt.KeyboardModifiers()
                        event = QMouseEvent(
                            QEvent.MouseButtonPress if down == '1' else QEvent.MouseButtonRelease,
                            point,
                            button,
                            buttons,
                            keyboard_modifiers
                        )
                    elif command == 'keypress':
                        texture, key, down = params.split('|')
#                        print "keypress", key, ':', down
                        event = QKeyEvent(
                            QEvent.KeyPress if down == '1' else QEvent.KeyRelease,
                            Qt.Key_G,#(ord(key)),
                            Qt.KeyboardModifiers()
                        )
                    browser.webview.event(event)
                browser.events = []
            Browsers.events_pending = False

            temp_browsers = Browsers.browsers[:]

        app.processEvents() # Necessary for the web views to update, load, etc.

        # Loop on a copy, because this may be slow - do not want to hold up the main thread. Also used in action() below
        for browser in temp_browsers:
            # Create if not yet created
            if browser.webview is None:
                browser.webview = QWebView()
                browser.webview.load(QUrl(browser.url))
                browser.webview.resize(browser.width, browser.height)
                #browser.webview.show() # We use offscreen rendering

            # Render and upload
            browser.webview.render(browser.image)

        if len(temp_browsers) > 0:
            def action(browsers):
                for browser in browsers:
                    CModule.upload_texture_data(browser.texture, 0, (browser.width-browser.height)/2, browser.width, browser.height, browser.bits)
                Browsers.old_frame_done = True
            main_actionqueue.add_action(functools.partial(action, temp_browsers)) # Upload image data in main thread
        else:
            Browsers.old_frame_done = True

thread = threading.Thread(target=main_loop)
thread.setDaemon(True)
thread.start()

# Component interactions

print "Loading WebKit component"

def get_button(button):
    if button == 1:
        return Qt.LeftButton
    elif button == 2:
        return Qt.RightButton
    elif button == 3:
        return Qt.MidButton
    else:
        return Qt.NoButton

def receive(sender, **kwargs):
    component_id = kwargs['component_id']
    data = kwargs['data']

#    print "C,D:", component_id, data

    try:
        if component_id == 'WebBrowser':
            parts = data.split('|')
            command = parts[0]
            params = '|'.join(parts[1:])
            if command == 'new':
                texture = params.split('|')[0]
                url = '|'.join(params.split('|')[1:])
                with Browsers.lock:
                    Browsers.browsers.append(Browser(texture, url))
            elif command == 'delete':
                texture = params
                with Browsers.lock:
                    for browser in filter(lambda browser: browser.texture == texture, Browsers.browsers):
                        browser.active = False
            elif command == 'click' or command == 'keypress':
                texture = params.split('|')[0]
                with Browsers.lock:
                    for browser in filter(lambda browser: browser.texture == texture, Browsers.browsers):
                        browser.events.append(data)
                    Browsers.events_pending = True
    except Exception, e:
        log(logging.ERROR, "Error in WebKit component: " + str(e))

    return ''

signal_component.connect(receive, weak=False)


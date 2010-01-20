#=============================================================================
# Copyright 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
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


import os, signal, threading, time
from multiprocessing import Process, Queue

from intensity.base import *
from intensity.logging import *
from intensity.signals import signal_component, shutdown


## A general framework for a component, run in a separate process
class ComponentDriver:
    class RESPONSE:
        Callback = 0
        Error = 1

    def __init__(self, name, component_main, keep_alive_always=False, keep_alive_when_outgoing=False):
        self.name = name
        self.component_main = component_main
        self.keep_alive_always = keep_alive_always
        self.keep_alive_when_outgoing = keep_alive_when_outgoing

        self.to_component = Queue()
        self.from_component = Queue()

        self.proc = None
        self.proc_counter = 0
        self.kickstart()

        thread = threading.Thread(target=self.main_loop)
        thread.setDaemon(True)
        thread.start()

        if self.keep_alive_always or self.keep_alive_when_outgoing:
            thread = threading.Thread(target=self.keepalive_loop)
            thread.setDaemon(True)
            thread.start()

        signal_component.connect(self.receive, weak=False)

    def kickstart(self):
        curr_proc_counter = self.proc_counter
        self.proc_counter += 1

        try:
            shutdown.disconnect(self.proc.dispatch_uid)
        except:
            pass
        try:
            self.proc.terminate()
        except:
            pass
        self.proc = Process(target=self.component_main, args=(self.to_component, self.from_component))
        self.proc.daemon = True
        self.proc.start()

        # Daemon flag seems not to work, so do this
        curr_proc = self.proc
        curr_proc.dispatch_uid = curr_proc_counter
        def terminate_proc(sender, **kwargs):
            if curr_proc.is_alive():
                try:
                    if WINDOWS:
                        curr_proc.terminate()
                    else:
                        os.kill(curr_proc.pid, signal.SIGKILL) # Stronger method
                except:
                    pass

        shutdown.connect(terminate_proc, weak=False, dispatch_uid=curr_proc_counter)

    def main_loop(self):
        while True:
            response_type, data = self.from_component.get()

            if response_type == ComponentDriver.RESPONSE.Callback:
                callback, param = data
                CModule.run_script('Tools.callbacks.tryCall("%s", "%s")' % (callback, param), 'component %s callback' % self.name)
            elif response_type == ComponentDriver.RESPONSE.Error:
                CModule.show_message('Error', 'Component %s: %s' % (self.name, data))

    def keepalive_loop(self):
        while True:
            time.sleep(1.0)

            # Restart
            if not self.proc.is_alive() and (self.keep_alive_always or (self.keep_alive_when_outgoing and not self.to_component.empty())):
                self.kickstart()
                continue

    def receive(self, sender, **kwargs):
        component_id = kwargs['component_id']
        data = kwargs['data']

        try:
            if component_id == self.name:
                parts = data.split('|')
                command = parts[0]
                params = '|'.join(parts[1:])
                self.to_component.put_nowait((command, params))
        except Exception, e:
            log(logging.ERROR, "Error in %s component: %s" + (self.name, str(e)))

        return ''


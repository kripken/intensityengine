
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

"""
Provides a thread-safe queue of actions.

A SafeActionQueue is a queue of actions that multiple threads
can add actions to. The queued actions can then be carried out
by the appropriate thread - either a thread dedicated to this purpose
alone, or a thread that executes all the queued actions as part of
its main loop.
"""

from __future__ import with_statement

import threading, time, thread

class SafeActionQueue:
    def __init__(self):
        self.action_queue = []
        self.lock = threading.Lock()
        self.action_needed = threading.Event() # Either a new action, or to quit
        self.should_quit = False
        self.has_quit = False

    def add_action(self, action):
        with self.lock:
            self.action_queue.append(action)
            self.action_needed.set()

    ## Runs all queued actions, in order, and returns
    def execute_all(self):
        # Under the lock, copy the list, delete it, then execute from the copy
        # Executing from the copy lets actions affect the queue (like adding new actions)
        with self.lock:
            to_do = self.action_queue
            self.action_queue = []
            if not self.should_quit: # If we should quit, then don't clear - leave set
                self.action_needed.clear()

        for i in range(len(to_do)):
            to_do[i]()

    ## Internal main loop function
    def _actual_main_loop(self):
        while not self.should_quit:
            self.action_needed.wait()
            if self.should_quit: # If quitting, quit, don't do the actions
                break
            self.execute_all()

        self.has_quit = True

    ## Runs as a 'main loop': Creates a thread, which
    ## waits for actions to appear
    ## in the queue, at which time it executes them
    def main_loop(self):
        thread = threading.Thread(target=self._actual_main_loop)
        thread.setDaemon(True) # Main program should not stop quitting if only this is left
        thread.start()

    ## Calling this will tell a running main_loop to quit
    def quit(self):
        with self.lock:
            self.should_quit = True
            self.action_needed.set()


## A class that runs CModule.render_progress() every now and then. This is done in the
## main thread (which is allowed to access the OpenGL context), and meanwhile other
## threads can do other stuff. This class continues until it is told by the other
## threads that it can stop
class KeepAliver:
    def __init__(self, message, delay=0.02, cancellable=False): # Poll 50fps by default
        self.message = message
        self.delay = delay
        self.should_quit = False
        self.cancellable = cancellable

    def wait(self):
        start_time = time.time()

        # Done at this late time, because otherwise loops
        import intensity.c_module
        CModule = intensity.c_module.CModule.holder

        while not self.should_quit:
            CModule.render_progress( -((time.time() - start_time)%3.0)/3.0, self.message )
            if Global.CLIENT:
                if not self.cancellable:
                    CModule.intercept_key(0)
                elif CModule.intercept_key(CModule.get_escape()):
                    thread.interrupt_main()
                    break

            time.sleep(self.delay)

    def quit(self):
        self.should_quit = True

    @staticmethod
    def do(func, message):
        '''
        E.g.:
        KeepAliver.do(
            lambda: some_func(),
            "Decompressing JPEG2000 image..."
        )
        '''
        from intensity.base import side_actionqueue
        keep_aliver = KeepAliver(message)
        class Result: pass
        def side_operations():
            Result.output = func()
            keep_aliver.quit()
        side_actionqueue.add_action(side_operations)
        keep_aliver.wait()
        return Result.output


from intensity.base import *


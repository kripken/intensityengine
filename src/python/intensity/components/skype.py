
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

import os, sys, time

from intensity.components.component_driver import *


def skype_main(to_skype, from_skype):
    sys.path.append(os.path.join(os.path.dirname(__file__), 'thirdparty'))
    import Skype4Py

    # Main

    skype = Skype4Py.Skype()

    # start Skype client if it isn't running
    if not skype.Client.IsRunning:
        skype.Client.Start()

    # This variable will get its actual value in OnCall handler
    CallStatus = 0

    # Here we define a set of call statuses that indicate a call has been either aborted or finished
    CallIsFinished = set ([Skype4Py.clsFailed, Skype4Py.clsFinished, Skype4Py.clsMissed, Skype4Py.clsRefused, Skype4Py.clsBusy, Skype4Py.clsCancelled]);

    def AttachmentStatusText(status):
       return skype.Convert.AttachmentStatusToText(status)

    def CallStatusText(status):
        return skype.Convert.CallStatusToText(status)

    class State:
        curr_call = None
        prepared_to_answer = []

    # This handler is fired when status of Call object has changed
    def OnCall(call, status):
        global CallStatus
        CallStatus = status
        print 'Call status: ' + CallStatusText(status)
        if status == Skype4Py.clsRinging and call.PartnerHandle in State.prepared_to_answer:
            call.Answer()

    def attach():
        for i in range(4):
            try:
                skype.Attach()
                print "Skype attachment proceeding"
                return
            except Skype4Py.errors.SkypeAPIError, e:
                print "Skype attachment failed (%d): %s" % (i, str(e))
            time.sleep(2.0)
        from_skype.put((ComponentDriver.RESPONSE.Error, "Failed to attach to Skype. Is Skype working, and did you allow this program to connect to it?"))
        sys.exit(1)

    # This handler is fired when Skype attachment status changes
    def OnAttach(status): 
        print 'API attachment status: ' + AttachmentStatusText(status)
        if status == Skype4Py.apiAttachAvailable:
            attach()

    skype.OnAttachmentStatus = OnAttach
    skype.OnCallStatus = OnCall

    # Attatching to Skype..
    print 'Connecting to Skype..'
    attach()

    # Main loop
    while True:
        command, params = to_skype.get()
        if command == 'whoami':
            print "Who am i?"
            from_skype.put((ComponentDriver.RESPONSE.Callback, (params, skype.CurrentUser.Handle)))
        elif command == 'call':
            if State.curr_call is not None:
                State.curr_call.Finish()
            State.curr_call = skype.PlaceCall(*(params.split(',')))
        elif command == 'preparetoanswer':
            State.prepared_to_answer.append(params)
            

# Setup
# XXX Seems to be problem with creating multiprocessing.Queue objects during __imports__
# which is exactly where we are now (a component being imported). As a workaround,
# queue it
#  * http://bugs.python.org/issue7707
def setup():
    global skype_driver
    skype_driver = ComponentDriver('Skype', skype_main, keep_alive_when_outgoing=True)
main_actionqueue.add_action(setup)


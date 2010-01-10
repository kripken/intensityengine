
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


import os, sys, time

from intensity.components.component_driver import *


def skype_main(to_skype, from_skype):
    from_skype.put((True,'I begin'))

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

    # This handler is fired when status of Call object has changed
    def OnCall(call, status):
        global CallStatus
        CallStatus = status
        print 'Call status: ' + CallStatusText(status)

    def attach():
        try:
            skype.Attach()
        except Skype4Py.errors.SkypeAPIError, e:
            from_skype.put((False, "Failed to attach to Skype. Is Skype working, and did you allow this program to connect to it?"))
            sys.exit(1)

    # This handler is fired when Skype attachment status changes
    def OnAttach(status): 
        print 'API attachment status: ' + AttachmentStatusText(status)
    #    if status == Skype4Py.apiAttachAvailable:
    #        attach()

    skype.OnAttachmentStatus = OnAttach
    skype.OnCallStatus = OnCall

    # Attatching to Skype..
    print 'Connecting to Skype..'
    attach()
    print 'Connected.'

    print "I am:", skype.CurrentUser.Handle

    # Checking if what we got from command line parameter is present in our contact list
    print "Placing call.."
    skype.PlaceCall('nicknamehandle')

    print "Looping..."
    while not CallStatus in CallIsFinished:
        time.sleep(0.25)
    print "Done"

    from_skype.put((False,'all '))


skype_driver = ComponentDriver('Skype', skype_main, keep_alive_when_outgoing=True)



# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

import os, sys, shutil

from intensity.signals import shutdown


GRACEFUL_SHUTDOWN_INDICATOR = "-----====Shutdown was graceful====-----"
MAX_UPLOAD_LOG_SIZE = 80*100

# On normal shutdown, output a marker that it was graceful
def write_shutdown_graceful(sender, **kwargs):
    print "--"
    print GRACEFUL_SHUTDOWN_INDICATOR
    print "--"


shutdown.connect(write_shutdown_graceful, weak=False)


# Check last log file, if there is one, and upload error log if it wasn't graceful

# Test for an ungraceful shutdown last time, and update master if so
def test_last_shutdown_graceful():
    try:
        log_filename = sys.argv[2]
    except IndexError:
        print "NOTE: No previous log file provided, so no way to check for prior crash and to upload log to master"
        return True # Continue as if all is well, oblivious to it all

    print "Checking previous log for crash:", log_filename
    if not os.path.exists(log_filename):
        return True # No log, so assume all ok

    log_size = os.stat(log_filename).st_size

    log_file = open(log_filename, 'r')
    safe_size = min(len(GRACEFUL_SHUTDOWN_INDICATOR)*3 + 50, log_size)
    log_file.seek(-safe_size, 2) # Read a safe margin of data from the end of the log
    data = log_file.read(safe_size)
    log_file.close()

    if GRACEFUL_SHUTDOWN_INDICATOR in data:
        return True # Last shutdown was fine

    # We had a crash last time, upload log
    print "WARNING: Last shutdown was not graceful, presumably due to a crash"
    print "----Uploading error log to master server"

    from intensity.server.auth import upload_error_log, InstanceStatus

    log_file = open(log_filename, 'r')
    if log_size > MAX_UPLOAD_LOG_SIZE:
        log_file.seek(-MAX_UPLOAD_LOG_SIZE, 2) # Limit size of uploaded log
    data = log_file.read()
    log_file.close()

    success = upload_error_log(data)

    backup_filename = log_filename + ".backup"
    try:
        shutil.move(log_filename, backup_filename)
    except:
        print '(old log not found, so not backed up)'

    if success:
        print '----Log upload succeeded.'
    else:
        print "WARNING: Error in uploading log to master server. Log can still be found in secondary backup (%s)" % backup_filename

    print "----Starting server in standby mode - do a manual repurposing to get it to load maps"

    InstanceStatus.in_standby = True

test_last_shutdown_graceful()


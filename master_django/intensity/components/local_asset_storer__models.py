
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

'''
Simple example of an asset storage component.

Store assets in the local file storage, using the uuid as the filename. Uses
a single lock for write access
'''

from __future__ import with_statement

__COMPONENT_PRECEDENCE__ = 100

import os, threading, shutil

from intensity.tracker.signals import asset_delete_trigger, store_asset, retrieve_asset, initialize_asset_storage, destroy_asset_storage


STORAGE_DIR = None

def set_dir(storage_dir=None, testing=False):
    if storage_dir is None:
        from django.conf import settings
        # Place directory in normal place if main DB is not in memory, or else in RAM drive (we are testing)
        if settings.DATABASE_NAME != ':memory:':
            # Normal storage location is under same directory as the DB (which is usually the home dir)
            storage_dir = os.path.join(os.path.dirname(settings.DATABASE_NAME), 'local_asset_storage')
        else:
            testing = True

    if testing:
        assert(os.path.exists('/dev/shm'), 'Testing requires a RAM drive, which is not set up on this OS.')
        storage_dir = '/dev/shm/local_asset_storage'

    global STORAGE_DIR
    STORAGE_DIR = storage_dir

    STORAGE_DIR = storage_dir
    if not os.path.exists(storage_dir):
        os.makedirs(storage_dir)

set_dir()

initialize_called_destroy = False

def destroy(sender, **kwargs):
    global initialize_called_destroy
    initialize_called_destroy = False # next time we can initialize, can call destroy, no harm (and useful in testing)

    if STORAGE_DIR is not None:
        try:
            pass
# Workaround for issue with this being called each syncdb - do not want to lose all this content
#            # Move the current files into .old (adding or replacing existing files there)
#            OLD_STORAGE_DIR = STORAGE_DIR + '.old'
#            if not os.path.exists(OLD_STORAGE_DIR):
#                os.makedirs(OLD_STORAGE_DIR)
#            for name in os.listdir(STORAGE_DIR):
#                if os.path.exists(os.path.join(OLD_STORAGE_DIR, name)):
#                    os.remove(os.path.join(OLD_STORAGE_DIR, name))
#                shutil.move(
#                    os.path.join(STORAGE_DIR, name),
#                    os.path.join(OLD_STORAGE_DIR, name),
#                )
#            os.rmdir(STORAGE_DIR)
        except Exception, e:
            print "Warning during removal of old storage: ", str(e)

destroy_asset_storage.connect(destroy, weak=False)


def initialize(sender, **kwargs):
    global initialize_called_destroy
    if not initialize_called_destroy:
        destroy(sender, **kwargs)
        initialize_called_destroy = True

        set_dir()

    from intensity.tracker.fixtures.create_initial_content import create_initial_asset_content
    return create_initial_asset_content(kwargs['asset'])

initialize_asset_storage.connect(initialize, weak=False)

def get_filename(asset_uuid):
    return os.path.join(STORAGE_DIR, asset_uuid)

## Write lock for all the assets - only one write at a time, for now
write_lock = threading.Lock()

def do_store(sender, **kwargs):
    asset = kwargs['asset']
    asset_uuid = asset.uuid
    asset_file = kwargs['asset_file']
    asset_file.seek(0) # Might have been read by other handlers

    with write_lock:
        destination = open(get_filename(asset_uuid), 'wb')
        for chunk in asset_file.chunks():
            destination.write(chunk)
        destination.close()

    return True

store_asset.connect(do_store)


def do_retrieve(sender, **kwargs):
    asset_uuid = kwargs['asset_uuid']
    source = open(get_filename(asset_uuid), 'rb')
    return source

retrieve_asset.connect(do_retrieve)


def do_delete(sender, **kwargs):
    asset_uuid = kwargs['asset_uuid']

    try:
        os.remove(get_filename(asset_uuid))
    except OSError:
        pass # The asset might not have content, have none uploaded yet, etc.

asset_delete_trigger.connect(do_delete)



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


# Core models of the master server

import os, hashlib, math, urllib, uuid, fnmatch
from string import lower

from django.db import models
from django.contrib.auth.models import User, check_password
from django.db.models.signals import pre_save, post_save, pre_delete

from intensity.tracker.signals import store_asset
import intensity.conf as intensity_conf


## Generic utility that takes an 'enum' (a class with members, e.g.
##    class STATUS:
##        Inactive, Designated, Preparing, Active, Standby = range(5)
## and returns a Django choices list of tuples. Also modifies and
## fixes the original enum, so that the choice codes are the same
## in the enum.
def create_enum_and_choices(enum):
    ret = [ (lower(name[0]), name) for name in filter(lambda x: x[0] != '_', dir(enum)) ]
    for key, val in ret:
        enum.__dict__[val] = key

    ## Translates the full string name to the code
    @staticmethod
    def convert(name):
        assert(name[0] != '_')
        return getattr(enum, name)

    enum.convert = convert

    return ret

## Generic utility that searches for a match among a list of
## separated items (by default using commas)
def filter_item_in_list(_class, field, value, sep=','):
    value = str(value)
    objects = _class.objects
    return objects.filter(**{field + '__contains': sep + value + sep}) | \
           objects.filter(**{field + '__startswith': value + sep}) | \
           objects.filter(**{field + '__endswith': sep + value}) | \
           objects.filter(**{field + '__exact': value})


## Returns a random UUID that is slugfield-ready
def make_uuid():
    return str(uuid.uuid4()).replace('-', '_')

## Base class for all Intensity Engine 'things', which we
## identify using UUIDs
class IntensityUUIDModel(models.Model):
    uuid = models.SlugField(max_length=50, unique=True, default='')

    @staticmethod
    def _create_uuid(sender, **kwargs):
        instance = kwargs['instance']
        if instance.uuid == '': # Only do this for a new instance, which got the default value
            instance.uuid = make_uuid() # _s are alphanumeric as per w+ in regexes

## Function that sets up uuid creation. If this is forgotten
## from a UUID class, the unique=True clause will cause errors
## when trying to add new items (which will have identical, blank
## UUIDs)
def intensity_uuid_model(_class):
    pre_save.connect(_class._create_uuid, sender=_class, weak=False)


## The user account info relevant to intensity. An addition to
## the Django User account, in a sense
## TODO: User a signal to delete this along with a user account, etc.
class UserAccount(IntensityUUIDModel):
    user = models.OneToOneField(User)
    nickname = models.CharField(max_length=30)
    kb_storage_left = models.IntegerField()
    seconds_left = models.IntegerField()
    session_id = models.SlugField(max_length=50, unique=True, default=make_uuid) # Need make_uuid here, to prevent default values
                                                                                 # from breaking uniqueness
#    session_timestamp = models.DateTimeField() # TODO: Time of session start/activity, so know when to expire it

    def __unicode__(self):
        return self.user.username

    @staticmethod
    def parent_saved(sender, **kwargs):
        if kwargs['created']: # Only care about new accounts
            user = kwargs['instance']
            user_account = UserAccount(
                user = user,
                nickname = user.username,
                kb_storage_left = 0,
                seconds_left = 0
            )
            user_account.save()

    @staticmethod
    def parent_deleted(sender, **kwargs):
        user = kwargs['instance']
        user_account = UserAccount.objects.get(user=user)
        user_account.delete()

    def start_session(self, hashed_password):
        success = hashed_password == self.user.password
        if success:
            self.session_id = make_uuid()
            self.save()
        return success

    @staticmethod
    def get_initial_asset_creator():
        return UserAccount.objects.get(user=User.objects.get(username='__initial_data_creator__'))


# Connect signals to auto-create and delete UserAccounts
# in relation to django Users

post_save.connect(UserAccount.parent_saved, sender=User, weak=False)
pre_delete.connect(UserAccount.parent_deleted, sender=User, weak=False)


## A representation of a single server instance
class ServerInstance(IntensityUUIDModel):
    ## Modes:
    ##  Pooled: A server instance that can be repurposed, i.e., part of the
    ##        main pool of instances for requitisioning.
    ##  Independent: A server instance that cannot be repurposed. It might be
    ##               an external server that uses this tracker but decides on
    ##               its own what to run, or it could be an internal server
    ##               that is doing some important task that keeps it out of
    ##               the pool for now.
    class MODE:
        Pooled, Independent = range(2)

    class STATUS:
        Inactive, Designated, Preparing, Active, Standby = range(5)

    ##! The complete URL to this server - resource type://domain:port - used to
    ##! access it for normal purposes - userspace stuff, events etc.
    user_interface = models.CharField(max_length=200, unique=True)

    ##! The complete URL to this server - resource type://domain:port - used to
    ##! access it for admin purposes - start up, shut down, add users, etc.
    ##! Typically this is the same as user_interface, but with a different port
    admin_interface = models.CharField(max_length=200, unique=True)

    ##! See MODE
    mode = models.CharField(max_length=1, choices=create_enum_and_choices(MODE))

    ##! Whether an activity is currently running
    status = models.CharField(max_length=1, choices=create_enum_and_choices(STATUS))

    ##! The current activity running, if one is running
    activity = models.ForeignKey('Activity', blank=True, null=True)

    ##! The last time we got an update from the instance
    last_update = models.DateTimeField()

    ##! The number of players currently active in this instance. This is
    ##! calculated every once in a while and stored here, so it is not
    ##! necessarily real time accurate.
    players = models.IntegerField(blank=True, null=True)

    ##! The maximum amount of players the instance will allow
    max_players = models.IntegerField(blank=True, null=True)

    ##! The user that requisitioned this instance, i.e., whose seconds are
    ##! used to keep it running
    requisitioner = models.ForeignKey(UserAccount, blank=True, null=True)

    session_id = models.SlugField(max_length=50, unique=True, default=make_uuid) # Need make_uuid here, to prevent default values
                                                                                 # from breaking uniqueness

    def __unicode__(self):
        return self.user_interface

    def contact_instance(self, command):
        url = self.admin_interface
        if url[-1] != '/': url += '/'
        url += command
        page = urllib.urlopen('http://' + url + '?' + urllib.urlencode([
            ('instance_id', self.uuid),
        ]))
        data = page.read()
        code = page.getcode()
        page.close()

        return code in [200, 302]

    ## Tells the instance it should be repurposed. This is done by
    ## the instance contacting the master directly to know what to
    ## repurpose to. This is more secure (do not trust incoming
    ## requests).
    ## Semantics: Requisitioning is the process of asking for
    ## a server, and getting one assigned. Repurposing is the
    ## specific act of making that server do what it should.
    def repurpose(self, account, activity):
        # Set data before contacting instance, so when it calls us back we get the info
        self.status = ServerInstance.STATUS.Preparing
        self.requisitioner = account
        self.activity = activity
        self.save()

        try:
            success = self.contact_instance('repurpose')
            if not success:
                raise Exception('Repurposing failed due to internal instance issue')
        except IOError: # Problem in contacting instance
            self.status = ServerInstance.STATUS.Inactive
            self.requisitioner = None
            self.activity = None
            self.save()
            raise

    def unpurpose(self):
        self.status = ServerInstance.STATUS.Inactive
        self.requisitioner = None
        self.activity = None
        self.save()

        try:
            self.contact_instance('unpurpose')
        except IOError: # Problem in contacting instance
            pass

    ## Do not list instances that are just for local work, unless we are
    ## a locally-running master
    def listable(self):
        if self.user_interface.split(':')[0] == 'localhost' and \
            intensity_conf.get('Network', 'address') != 'localhost':
            return False
        return True


class Activity(IntensityUUIDModel):
    ## The user-chosen name of this activity.
    name = models.CharField(max_length=30)

    ## A comment for this activity (useful for versioning info etc.)
    comment = models.TextField(blank=True)

    ## The id of the asset group for this activity - i.e., an asset that has a
    ## dependency on all the assets needed for this activity.
    ## By convention this 'head asset' is also a zipfile asset containing
    ## map.ogz and map.js for this activity
    asset = models.ForeignKey('AssetInfo')

    ## The creator and owner of this activity
    ## Assets can have multiple owners, but activities only have one. One reason
    ## is that assets take storage space, and sharing assets spreads the 'cost'.
    ## Another is that assets are worked on by multiple people. Whereas an
    ## activity is just a placeholder that refers to an asset, which anyone can
    ## create.
    owner = models.ForeignKey(UserAccount)

    def __init__(self, *args, **kwargs):
        asset = kwargs.get('asset')
        if asset is not None and not Activity.is_valid_asset(asset):
            raise Activity.InvalidAsset()

        super(Activity, self).__init__(*args, **kwargs)

    def __unicode__(self):
        return self.name

    class InvalidAsset(Exception):
        pass

    @staticmethod
    def is_valid_asset(asset):
        return fnmatch.fnmatch(asset.location, 'base/*.tar.gz')


class AssetInfo(IntensityUUIDModel):
    location = models.CharField(max_length=200)

    hash_value = models.CharField(max_length=128, blank=True, null=True)
    dependencies = models.ManyToManyField('self', symmetrical=False) # Assets upon which we depend

    class TYPE:
        Server, Client, Both = range(3)

    type_x = models.CharField(max_length=1, choices=create_enum_and_choices(TYPE)) # Convention: _x when need a _

    # Additional fields - master server only

    ##! How much storage this asset needs, in KB.
    kb_size = models.IntegerField(default=0)

    ##! A list of (uuids of) people that 'own' this asset.
    ##! The first 'original' owner is the one who 'pays'
    ##! for the storage, i.e., it comes out of their
    ##! account limit (this lets you add other owners
    ##! freely without it harming them). So changing
    ##! the first owner should not be possible.
    ##!
    ##! Owners can update the asset, so the owner of a map asset
    ##! can enter edit mode to edit that asset, for
    ##! example.
    owners = models.ManyToManyField(UserAccount) # Assets upon which we depend

    comment = models.TextField(blank=True)

#    preview = models.BlobProperty() ##< A small 256x256 or so png preview, or such

    def __unicode__(self):
        return self.location

    ## Updates the metadata (hash value and size) given a file. Typically this is done after
    ## a new asset file is uploaded
    @staticmethod
    def calculate_metadata(sender, **kwargs):
        asset = kwargs['asset']

        asset_file = kwargs['asset_file']
        asset_file.seek(0) # Might have been read by other handlers

        # Size
        asset.kb_size = math.ceil(asset_file.size/1024.)

        # Hash - use Django chunks, so can handle big files decently
        hasher = hashlib.sha256()
        for chunk in asset_file.chunks():
            hasher.update(chunk)
        asset.hash_value = 'SHA256|' + hasher.hexdigest()

        # Save
        asset.save()

        return True

    ## Returns the 'emptymap asset', a basis for new maps
    @staticmethod
    def get_emptymap():
        owner = UserAccount.get_initial_asset_creator()
        ret = AssetInfo.objects.filter(location='base/emptymap.tar.gz')
        ret = filter(lambda r: r.owners.all()[0].uuid == owner.uuid, ret)[0]
        return ret


store_asset.connect(AssetInfo.calculate_metadata, weak=False)

classes = [UserAccount, ServerInstance, Activity, AssetInfo]

for _class in classes:
    intensity_uuid_model(_class)


# Set home dir, to our parent (ignored if already set)
import intensity.conf as intensity_conf
intensity_conf.set_home_dir()


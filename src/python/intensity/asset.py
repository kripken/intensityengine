
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

import os, cgi, urllib, time, tempfile, hashlib, shutil, httplib, pickle, tarfile, urlparse
from functools import partial

from _dispatch import Signal

from intensity.logging import *
from intensity.post_multipart import post_multipart
from intensity.signals import multiple_send
from intensity.errors import IntensityUserCancellation


# Signals

# Called when checking if an asset exists in up-to-date form locally
check_existing = Signal(providing_args=['asset_info'])

# Called when an asset is actually modified, and when a file inside it (if it is
# an archive) is modified. In the latter case 'filename' is given. Filenames
# are relative to the home_dir/packages/
asset_item_updated = Signal(providing_args=['asset_info', 'filename'])

# Errors

class AssetRetrievalError(Exception):
    pass


def calculate_hash(filename, hasher):
    try:
        f = open(filename, "rb")
        data = f.read()
        f.close()
        return hasher(data).hexdigest()
    except IOError:
        return None


## Metadata for an asset stored locally. This class encapsulates
## the functionality to access the metadata of an asset, using its ID.
##
## Usage:
##      To get metadata for an existing asset, use get_by_path,
##      with the path to the asset. This is what you would do if
##      you are traversing all the assets, e.g., looking for which
##      was most recently used
##
##      You can simply read the fields .asset_id, .acquire_time,
##      .use_time of an AssetMetadata instance to get that info
##
##      To create a new metadata, create a new AssetMetadata,
##      giving it the asset_id
##
##      Given an instance of this class, you can call update_acquire_time
##      and update_use_time when those events occur. Then call save()
##      to update the data on disk
class AssetMetadata:
    ## Gets the path to the actual metadata file, given the path to
    ## the asset
    ## For now, we simply use the same filename, but under /metadata instead of /packages
    @staticmethod
    def get_metadata_path(asset_path):
        ASSET_FRAG = 'packages'
        frags = asset_path.split( os.sep )
        assert(list(frags).count(ASSET_FRAG) == 1) # Need just one such thing, and we replace exactly that
        frags[frags.index(ASSET_FRAG)] = 'metadata'
        return os.sep.join(frags)

    ## Gets an asset metadata using the path of an asset. The asset metadata
    ## must already exist, so we can read the asset_id from it
    @staticmethod
    def get_by_path(asset_path):
        data = AssetMetadata.load_raw( AssetMetadata.get_metadata_path(asset_path) )
        if data is not None:
            return AssetMetadata(data['asset_id'])
        else:
            raise Exception("Cannot get metadata for asset %s as an error occured in reading" % asset_path)

    def __init__(self, asset_id):
        self.asset_id = asset_id
        self.asset_info = AssetManager.get_info(asset_id) # Needed?
        self.asset_filename = AssetManager.get_full_location(self.asset_info) # Needed?
        self.metadata_filename = AssetMetadata.get_metadata_path(self.asset_filename)

        # Defaults
        self.acquire_time = None
        self.use_time = None

        self.load() # Try to load existing metadata

    def load(self):
        data = self.load_raw(self.metadata_filename)
        if data is not None:
            try:
                assert(data.asset_id == self.asset_id)
                self.acquire_time = data.acquire_time
                self.use_time = data.use_time
            except: # Errors mean we just ignore the file data
                pass

    @staticmethod
    def load_raw(metadata_filename):
        metadata_filename = os.path.join(get_home_subdir(), metadata_filename)
        try:
            pkl_file = open(metadata_filename, 'rb')
            data = pickle.load(pkl_file)
            pkl_file.close()
        except IOError:
            return None
        except EOFError:
            return None

        if type(data) is not dict:
            return None

        return data            

    ## Saves the (possibly changed) asset metadata
    def save(self):
        try:
            # Ensure the directories exist
            dirname = os.path.dirname(self.metadata_filename)
            if not os.path.exists(dirname):
                os.makedirs(dirname)

            output = open(self.metadata_filename, 'wb')
            pickle.dump({ 'asset_id': self.asset_id, 'acquire_time': self.acquire_time, 'use_time': self.use_time }, output)
            output.close()
        except Exception, e:
            log(logging.ERROR, "Error in saving asset metadata: %s - %s: %s" % (self.asset_id, self.metadata_filename, str(e)))
            assert(0) # XXX

    ## Marks the acquire time to be 'now'. Not saved - call save() for that
    def update_acquire_time(self):
        self.acquire_time = time.time()

    ## Marks the use time to be 'now'. Not saved - call save() for that
    def update_use_time(self):
        self.use_time = time.time()


class AssetInfo:
    def __init__(self, asset_id, location, url, _hash, dependencies, _type):
        self.asset_id = asset_id
        self.location = location
        self.url = url
        self._hash = _hash ##< Form: "TYPE|HASHVAL", e.g.: "SHA256|ABE1723762", sha256 with ABE1723762
        self.dependencies = dependencies
        self._type = _type

    def get_hash_type(self):
        return self._hash.split("|")[0]

    def get_hash_value(self):
        return self._hash.split("|")[1]

    def has_content(self):
        if self.url == "" and self.location == "":
            return False

        if self.location[-1] == "/":
            assert(self.url == "" or self.url is None) # Directories have no content to get
            return False # This is a directory

        assert(self.url != "")
        return True

    def is_valid(self):
        if self.asset_id == "":
            log(logging.ERROR, "Invalid asset info: No ID");
            return False
        elif not (self.has_content() or len(self.dependencies) > 0):
            log(logging.ERROR, "Invalid asset info for %s: No content and no dependencies" % self.asset_id);
            return False
        elif self._type == "":
            log(logging.ERROR, "Invalid asset info for %s: No type" % self.asset_id);
            return False

        return True

    ## Checks if this asset's location is of a zipfile, i.e., ending with .tar.gz, .zip, etc.
    def is_zipfile(self):
        return len(self.location) > 7 and self.location[-7:] == ".tar.gz"

    ## For a zipfile, returns the part of the location that is the basis, i.e., without the
    ## trailing .tar.gz or .zip
    ## @param name If None (not given), then use the location of this instance
    def get_zip_location(self, name=None):
        if name is None:
            name = self.location
        return name[:-7]


class AssetManagerClass:
    def __init__(self):
        self.clear_cache()

    ## Wipes all asset infos, so that we will re-request them from the master.
    ## Important in order to request asset infos when they have been updated (new hash),
    ## until we implement a push system.
    def clear_cache(self):
        self.asset_infos = {}

    ## Primary function: Acquire an asset.
    ## @return the AssetInfo for that asset, or None on failure
    def acquire(self, asset_id):
        log(logging.DEBUG, "Acquiring asset: " + asset_id)

        asset_info = self.get_info(asset_id)

        if not self.is_needed(asset_info):
            return

        self.create_location(asset_info)

        self.get_dependencies(asset_info)

        self.get_content(asset_info) # Done after dependencies, so if they fail, no need to get this

        return asset_info

    ## @param recursive Whether to also get infos for all dependencies, recursively, in a single
    ##                  request to the master
    ## @return The AssetInfo
    def get_info(self, asset_id, recursive=True):
        # Simple caching TODO: Improve
        if asset_id in self.asset_infos:
            return self.asset_infos[asset_id]

        master_server = get_config("Network", "master_server", "MISSING MASTER SERVER")

        master_server = master_server.replace("http://", "") # Allow config to have http://, even though it isn't needed

        # Get asset info

        class Output:
            pass

        keep_aliver = KeepAliver("Retrieving asset info...")

        def side_operations():
            try:
                Output.response = contact_master(
                    "asset/getinfo",
                    { 'asset_id' : asset_id, 'recurse': '1' }
                )
            except MasterNetworkError, e:
                Output.error = str(e)
                log(logging.ERROR, "Error in contacting master: " + Output.error)
                Output.response = None
            finally:
                keep_aliver.quit()

        # Run blocking HTTP in other thread, meanwhile do keepalive in this one, until finished
        side_actionqueue.add_action(side_operations)
        keep_aliver.wait()

        if Output.response is None:
            raise AssetRetrievalError("Error from master server when trying to get AssetInfo:\n %s" % (Output.error))

        asset_info_raw = Output.response

        log(logging.DEBUG, "Parsed response: " + str(asset_info_raw))

        if 'error' in asset_info_raw:
            raise AssetRetrievalError("Error response from master server when trying to get AssetInfo: %s" % (str(asset_info_raw['error'])))

        try:
            SEP = '$'
            num = asset_info_raw['asset_id'].count('$')+1
            empty = ['']*num

            asset_ids = asset_info_raw['asset_id'].split(SEP)
            locations = asset_info_raw['location'].split(SEP) if 'location' in asset_info_raw else empty
            urls = asset_info_raw['url'].split(SEP) if 'url' in asset_info_raw else empty
            hashes = asset_info_raw['hash'].split(SEP) if 'hash' in asset_info_raw else empty
            dependencieses = asset_info_raw['dependencies'].split(SEP) if 'dependencies' in asset_info_raw else empty
            types = asset_info_raw['type'].split(SEP) if 'type' in asset_info_raw else empty

            for i in range(num):
                curr_asset_info = AssetInfo(
                    asset_id = asset_ids[i],
                    location = locations[i],
                    url = urls[i],
                    _hash = hashes[i],
                    dependencies = dependencieses[i].split(',') if dependencieses[i] != '' else [], # '' would be split into ['']; need []
                    _type = types[i]
                )

                if not curr_asset_info.is_valid():
                    raise AssetRetrievalError('Got an invalid AssetInfo for: '+curr_asset_info.asset_id + ' : '+curr_asset_info.location)

                self.asset_infos[curr_asset_info.asset_id] = curr_asset_info
        except:
            log(logging.ERROR, "Error on parsing response of AssetInfo request: " + str(asset_info_raw))
            raise

        if asset_id not in self.asset_infos:
            raise AssetRetrievalError('Response from master did not include the asset we requested')

        return self.asset_infos[asset_id]

    def is_needed(self, asset_info):
        if (asset_info._type == "S" and Global.CLIENT) or (asset_info._type == "C" and Global.SERVER):
            return False
        else:
            return True

    @classmethod
    def get_full_location(self, asset_info):
        location = asset_info.location
        # We will use this as an actual path now, so fix for Windows
        if WINDOWS:
            location = location.replace('/', '\\')
        return os.path.join( get_asset_dir(), location)

    def create_location(self, asset_info):
        dirname = os.path.dirname( self.get_full_location(asset_info) )

        if not os.path.exists(dirname):
            os.makedirs( dirname )

    def get_dependencies(self, asset_info):
        log(logging.DEBUG, "Getting dependencies for " + asset_info.asset_id + ":" + str(len(asset_info.dependencies)))
        for dependency_id in asset_info.dependencies:
            self.acquire(dependency_id)

    def get_content(self, asset_info):
        if not asset_info.has_content():
            return

        metadata = AssetMetadata(asset_info.asset_id) # Asset has content, so prepare to work on its metadata

        CModule.render_progress(0, 'validating... ' + str(asset_info.location))
        if Global.CLIENT: CModule.intercept_key(0)

        if not self.check_existing(asset_info):
            # Actually acquire it
            log(logging.DEBUG, "Getting content for asset %s, from: %s" % (asset_info.asset_id, asset_info.url))

            # Build full URL - bake the session_info into the url
            parsed = urlparse.urlparse(asset_info.url)
            query = cgi.parse_qsl(parsed.query)
            get_master_session().add_info(query)
            new_parsed = list(parsed)
            new_parsed[4] = urllib.urlencode(query)
            full_url = urlparse.urlunparse(new_parsed)

            log(logging.DEBUG, "   Full download URL: " + full_url)

            class DownloadInfo:
                last_time = time.time()
                bytes = 0

            def reporthook(blocknum, bs, size):
                if Global.CLIENT:
                    if CModule.intercept_key(CModule.get_escape()):
                        raise IntensityUserCancellation('Escape pressed')

                if time.time() - DownloadInfo.last_time > 0.1: # Max 10 render_progresses per second. Prevents filling of server logs.
                    DownloadInfo.last_time = time.time()

                    DownloadInfo.bytes = blocknum*bs
                    if size > 0:
                        bar = float(DownloadInfo.bytes)/size
                    else:
                        bar = 0
                    mb = DownloadInfo.bytes/1048576.
                    CModule.render_progress(bar, "downloading... " + str(asset_info.location) + '  (%.2f MB)' % mb)

            # Some NATs require more than 1 attempt to succeed, web browsers therefore do 1-4 attempts,
            # so we do something similar.
            # Also, we try both with and without the proxies that Python auto-detects - they may be
            # necessary, but they may also be misleading

            temp_filename = ''
            def cleanup():
                if WINDOWS: return # For some reason Windows may crash here, even *with* the try-except...
                try:
                    os.remove(temp_filename)
                except:
                    pass

            errors = []

            for attempt_type in [False, False, False, False, True, True, True, True]:
                try:
                    temp_filename = tempfile.mkstemp()[1] # Use a temporary file, so partials do not appear in full_location
                    DownloadInfo.bytes = 0

                    if attempt_type:
                        log(logging.DEBUG, "   normal download attempt");
                        dummy, headers = urllib.urlretrieve(full_url, temp_filename, reporthook)
                    else:
                        log(logging.DEBUG, "   proxy-less download attempt");
                        dummy, headers = urllib.FancyURLopener(proxies={}).retrieve(full_url, temp_filename, reporthook)

                    # If got here, success
                    errors = []
                    break

                except IOError, e:
                    errors += [str(e)]
                    cleanup()
                    if DownloadInfo.bytes > 5120:
                        break # We downloaded some data, then ran into an error - so the issue isn't proxies. Give up.
                    else:
                        time.sleep(0.1) # Might be a proxies issue that prevented connecting. Wait a bit, then continue to next attempt
                except IntensityUserCancellation:
                    errors = ['Cancelled by user.']
                    break

            if errors != []:
                log(logging.WARNING, 'urlretrieve errors: ' + str(errors))
                raise AssetRetrievalError(errors[0]) # Just one, to not fill screen. Rest are logged.

            if 'Content-Type: text/html' in str(headers):
                # We received an html/text response, containing an error, not a binary asset file. Raise the error
                ret = AssetRetrievalError(open(temp_filename, 'r').read())
                cleanup()
                raise ret

            # Original is a tempfile, and will be deleted by the OS in due time (doing 'move' here would fail on Windows, but not Linux)
            shutil.copyfile(temp_filename, self.get_full_location(asset_info))

            cleanup()

            asset_item_updated.send(None, asset_info=asset_info)

    #        except IOError, e: # Connection refused, etc. etc.
     #           log(logging.WARNING, "Cannot acquire AssetInfo for %s (%s), due to %s" % (asset_id, url, str(e)))
      #          return None

            if not self.check_existing(asset_info):
                log(logging.ERROR, "Failed to retrieve asset " + asset_info.asset_id)
                raise AssetRetrievalError("Failed to retrieve asset")

            # We get the asset, mark the asset's metadata
            metadata.update_acquire_time()

        # Mark the asset's metadata that we are attempting to use it now
        metadata.update_use_time()

        # Save the metadata info
        metadata.save()

    ## Checks if we have existing data that is up to date for the asset
    def check_existing(self, asset_info):
        checks = multiple_send(check_existing, None, asset_info=asset_info)
        return True in checks and False not in checks

    ## Uploads this asset to the server. The asset is uploaded from its natural position,
    ## i.e., the assumption is that the asset has been modified locally.
    ## Raises Exception on error
    def upload_asset(self, asset_info):
        # Primary URL will redirect us to the correct location
        url = contact_master(
            'tracker/asset/upload/%s/' % asset_info.asset_id,
            { },
            accept_redirect=True
        )
        parsed = urlparse.urlparse(url)

        protocol = httplib.HTTPConnection if parsed.scheme == 'http' else httplib.HTTPSConnection
        asset_server = parsed.netloc
        path = parsed.path

        datafile = open(AssetManager.get_full_location(asset_info), 'rb')
        data = datafile.read()
        datafile.close()

        class Output:
            pass

        keep_aliver = KeepAliver("Uploading...", cancellable=True)

        def side_operations():
            params = [
                ('asset_id', asset_info.asset_id),
                ('user_id', get_master_session().my_id),
                ('return_to', 'http://' + get_master_server()) # asset server detects master from this
            ]

            get_master_session().add_info(params) # Add session info

            try:
                Output.response = post_multipart(
                    protocol,
                    asset_server,
                    path,
                    params,
                    [ ('file', "Xx.xX", data) ]
                )
            except Exception, e:
                Output.response = None
                Output.error = "Error in uploading asset: %s" % (str(e))
            finally:
                keep_aliver.quit()

        # Run blocking HTTP in other thread, meanwhile do keepalive in this one, until finished
        side_actionqueue.add_action(side_operations)
        try:
            keep_aliver.wait()
        except KeyboardInterrupt:
            Output.response = None
            Output.error = 'Cancelled by user.'

        if Output.response is None:
            log(logging.ERROR, Output.error)
            raise Exception(Output.error)
        elif Output.response.status not in [200, 302]: # Accept redirect as success, as we redirect only on success
            error = "Error in uploading asset: %s - %s" % (Output.response.status, Output.response.reason)
            log(logging.ERROR, error)
            #print str(Output.response.getheaders())
            #print Output.response.read()
            raise Exception(error)


# Standard components

## TODO: Do this directly on disk, without loading into memory - would be more efficient perhaps
## @param hash_type The type of hash to use. If none is given, use the hash implied in
##                  the asset info
## @return The complete hash, including type and value, as appearing in AssetInfo._hash
##         Returns None if no hash could be calculated.
def get_existing_file_hash(asset_info, hash_type=None):
    if hash_type is None:
        hash_type = asset_info.get_hash_type()

    if hash_type == "MD5":
        hasher = hashlib.md5
    elif hash_type == "SHA1":
        hasher = hashlib.sha1
    elif hash_type == "SHA256":
        hasher = hashlib.sha256
    else:
        raise Exception("Unknown hash type: " + str(hash_type) + ":" + str(type(hash_type)))

    value = calculate_hash(AssetManager.get_full_location(asset_info), hasher)
    if value is None: # No such file or other error
        return None

    return hash_type + "|" + value

def check_hash(sender, **kwargs):
    asset_info = kwargs['asset_info']
    if asset_info.get_hash_type() == "NONE":
        # A None hash implies nothing to check - use local value blindly. Only return
        # False if nothing is there to use
        return os.path.exists( AssetManager.get_full_location(asset_info) )

    desired = asset_info._hash
    existing = KeepAliver.do(partial(get_existing_file_hash, asset_info), "Checking content for %s..." % asset_info.location)
    log(logging.DEBUG, "Comparing hashes: %s ? %s" % (desired, existing))
    return desired == existing

check_existing.connect(check_hash, weak=False)

# Extract existing zipfiles during check phase. This is useful for the case
# where the zipfile was part of the download binary, or we just pasted it
# in, and all we need is for it to be expanded. Extraction is only done
# if the directory doesn't exist (so we don't override local changes
# during editing).

def check_zipfiles_need_extraction(sender, **kwargs):
    asset_info = kwargs['asset_info']

    full_location = AssetManager.get_full_location(asset_info)

    if not asset_info.has_content() or not os.path.exists(full_location) or not asset_info.is_zipfile():
        return True # Not relevant to us

    if not os.path.exists(asset_info.get_zip_location(full_location)):
        asset_item_updated.send(None, asset_info=asset_info) # Treat this like it just arrived - process it, extraction etc.  

    return True

check_existing.connect(check_zipfiles_need_extraction, weak=False)

def extract_zipfiles(sender, **kwargs):
    '''
    This handles special assets like tarfile assets, which are extracted
    into a directory with their name.
    '''
    if 'filename' in kwargs:
        return True # We don't care about internal files - no recursive archives

    asset_info = kwargs['asset_info']

    full_location = AssetManager.get_full_location(asset_info)

    if not asset_info.has_content() or not os.path.exists(full_location) or not asset_info.is_zipfile():
        return True # Not relevant to us

    path = asset_info.get_zip_location(full_location) + os.sep # This is a directory, with the name of the tarfile

    if not os.path.exists(path): # Create our directory, if needed
        os.makedirs(path)

    the_zip = tarfile.open(full_location, 'r')
    names = the_zip.getnames()
    the_zip.extractall(path)
    the_zip.close()

    # Send updates for all files just created
    for name in names:
        asset_item_updated.send(
            None,
            asset_info=asset_info,
            filename=os.path.join(asset_info.get_zip_location(), name)
        )

    return True

# Extract newly retrieved zipfiles, overriding old contents if any.
asset_item_updated.connect(extract_zipfiles, weak=False)


# Singleton
AssetManager = AssetManagerClass()


# Prevent loops
from intensity.base import *
from intensity.master import *


# (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
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


from datetime import datetime
import urllib, logging, fnmatch

from django.http import HttpResponse, HttpResponseRedirect
from django.views.generic.simple import direct_to_template
from django.contrib.auth.decorators import login_required
from django.contrib.auth.models import User
from django.core.files import File
from django.db import transaction

from intensity.models import UserAccount, ServerInstance, Activity, AssetInfo, filter_item_in_list, make_uuid
from intensity.tracker.forms import ServerInstanceUpdateForm, AssetForm, ActivityForm
from intensity.signals import singleton_send, multiple_send, send_email

from intensity.tracker.signals import asset_upload_redirect, asset_download_redirect, asset_delete_trigger, requisition_instance, unrequisition_instance, validate_instance, list_instances, post_instance_update, pre_instance_update, receive_error_log, account_tools
from intensity.components.asset_storer__views import store_asset, retrieve_asset
import intensity.conf as intensity_conf
from intensity.register.decorators import login_or_session_required
from intensity.version import check_version
from intensity.templatetags.intensity_tags import filtration, check_only_mine


def account(request, uuid=None, message=None):
    if request.user.is_authenticated():
        my_account = UserAccount.objects.get(user=request.user)
        my_uuid = my_account.uuid
    else:
        my_uuid = None

    if uuid is not None:
        if uuid != my_uuid:
            other_account = UserAccount.objects.get(uuid=uuid)
        else:
            other_account = my_account
    else:
        if not request.user.is_authenticated():
            return login_required(account)(request)

        uuid = my_uuid
        other_account = my_account

    if uuid == my_uuid:
        instances = ServerInstance.objects.filter(requisitioner=my_account)
    else:
        instances = []

    multiple_send(list_instances, None, instances = instances, request = request)

    return direct_to_template(request, template="tracker/account.html", extra_context={
        'account': other_account,
        'is_my_account': uuid == my_uuid,
        'instances': instances,
        'message': message,
        'tools': multiple_send(account_tools, None, account = my_account) if uuid == my_uuid else [],
    })


def account_startsession(request):
    if not check_version(request.GET.get('version', '')):
        return HttpResponse(urllib.urlencode([('error', 'Version mismatch. Please download the latest release.')]))

    username = request.GET['identifier']
    hashed_password = request.GET.get('hashed_password', None)

    error = False

    try:
        user = User.objects.get(username=username)
        account = UserAccount.objects.get(user=user)

        # No password - asking for hash+salt
        if hashed_password is None:
            algorithm, salt, hash_ = user.password.split('$')
            # The real password is longer than 6 characters
            assert(len(algorithm) < 6)
            assert(len(salt) < 6)
            assert(len(hash_) > 6)
            return HttpResponse(urllib.urlencode([
                ('algorithm', algorithm),
                ('salt', salt),
            ]))

        if account.start_session(hashed_password):
            ret = [('your_id', account.uuid), ('session_id', account.session_id)]

            logging.info('Client login: ' + username)
        else:
            error = True
    except User.DoesNotExist:
        error = True

    if error:
        ret = [('error', 'The supplied username and password do not match a valid account.')]

    return HttpResponse(urllib.urlencode(ret))


@login_or_session_required
def account_instancelogin(request):
    instance_id = request.GET['instance_id']
    code = request.GET['code']

    instance = ServerInstance.objects.get(uuid=instance_id)

    error = False
    try:
        account_id, session_id = code.split(',')
        try:
            account = UserAccount.objects.get(uuid=account_id)
            # TODO: Validate session_id
        except:
            error = True
    except:
        error = True

    if error:
        return HttpResponse(urllib.urlencode([('error', 'Invalid validation code')]))

    # Immediately note the new # of players. This may be buggy sometimes, but good enough.
    instance.players += 1
    pre_instance_update.send(None, instance=instance) # As if the instance is updated, with new stats
    post_instance_update.send(None, instance=instance) # As if the instance is updated, with new stats
    instance.save()

    # TODO: As components
    activity = instance.activity
    if activity is not None:
        is_owner = account.uuid in [owner.uuid for owner in activity.asset.owners.all()]
    else:
        is_owner = False

    can_edit = bool(int(intensity_conf.get('Instances', 'let_anyone_edit'))) or is_owner

    return HttpResponse(urllib.urlencode([
        ('success', '1'),
        ('user_id', account.uuid),
        ('username', account.nickname),
        ('can_edit', str(int(can_edit))),
    ]))


## Utility

def sorted_objects(request, default, sortables, objects):
    sorts = request.GET.get('sorts', default).split(',')
    for sort in sorts:
        if sort[0] == '-': sort = sort[1:]
        assert(sort in sortables or '-' + sort in sortables)

    return objects.order_by(*sorts)


## Instances

def instances(request):
    SORTABLES = {
        'user_interface': 'Location',
        'admin_interface': 'Admin',
        'status': 'Status',
        'activity': 'Current Activity',
        'last_update': 'Last Update',
        'requisitioner': 'Requisitioner',
        'players': 'Players',
        'max_players': 'Max. Players'
    }

    objects = sorted_objects(
        request,
        '-last_update',
        SORTABLES,
        ServerInstance.objects
    )

    # Only display listable instances
    objects = filter(lambda instance: instance.listable(), objects)

    multiple_send(list_instances, None, instances = objects, request = request)

    return direct_to_template(request, template="tracker/instances.html", extra_context={
        'instances': objects, 'sortables': SORTABLES,
    })


def instance_update(request):
    data = request.GET.copy()

    if not check_version(data.get('version', '')):
        return HttpResponse(urllib.urlencode([('error', 'Engine version mismatch.')]))

    # If instance didn't send us interfaces, use 'unknown' values
    # (useful for running a master with a single server that doesn't
    # know it's IP)
    if data.get('user_interface') is None:
        data['user_interface'] = '127.0.0.1'
    if data.get('admin_interface') is None:
        data['admin_interface'] = '127.0.0.1'

    try:
        instance = ServerInstance.objects.get(user_interface=data.get('user_interface'))
        form = ServerInstanceUpdateForm(data, instance=instance)
    except ServerInstance.DoesNotExist:
        form = ServerInstanceUpdateForm(data)

    instance = form.instance

    assert(form.is_valid())

    instance.last_update = datetime.now()
    form.save() # Generate UUIDs, etc.

    pre_instance_update.send(None, instance=instance)

    true_activity = instance.activity

    response = []

    response.append( ('instance_id', instance.uuid) )

    # If the instance is not running the right map, tell it so

    if true_activity is not None:
        true_activity_id = true_activity.uuid
        true_map_id = true_activity.asset.uuid
    else:
        true_activity_id = ''
        true_map_id = ''

    # Decide if to notify of new map, and update the status

    curr_activity_id = data.get('activity_id', '')
    if curr_activity_id != '':
        try:
            curr_activity = Activity.objects.get(uuid=curr_activity_id)
        except:
            print "Warning: Invalid activity reported by instance"
            curr_activity = None
    else:
        curr_activity = None

    curr_asset_id = data.get('map_asset_id', '')
    if curr_asset_id != '':
        curr_asset = AssetInfo.objects.get(uuid=curr_asset_id)
    else:
        curr_asset = None

    force_asset_location = intensity_conf.get('Instances', 'force_asset_location')
    if force_asset_location != '':
        # We tell all instances to run a particular map asset, if not already running it
        force_asset = AssetInfo.objects.get(location=force_asset_location)
        if not (curr_asset is not None and curr_asset.uuid == force_asset.uuid):
            response.append( ('activity_id', 'forced_location') )
            response.append( ('map_asset_id', force_asset.uuid) )

        instance.status = ServerInstance.STATUS.Active
    else:
        # Normal processing
        if curr_activity != true_activity:
            response.append( ('activity_id', true_activity_id) )
            response.append( ('map_asset_id', true_map_id) )

            if true_activity_id != '':
                instance.status = ServerInstance.STATUS.Preparing
            else:
                instance.status = ServerInstance.STATUS.Inactive
        else:
            if true_activity_id != '':
                instance.status = ServerInstance.STATUS.Active
            else:
                instance.status = ServerInstance.STATUS.Inactive

    # Mode

    validations = validate_instance.send(None, instance=instance, validation=data.get('validation'))
    # If at least one positive validation, then validated
    if len(validations) > 0 and reduce(
        lambda x,y: x or y,
        map(lambda result: result[1], validations)
    ):
        instance.mode = ServerInstance.MODE.Pooled
        instance.session_id = make_uuid()
    else:
        instance.mode = ServerInstance.MODE.Independent
        instance.activity = curr_activity

    response.append( ('session_id', instance.session_id) ) # Maybe the old one, or a new one

    # Post-update trigger

    post_instance_update.send(None, instance=instance)
    
    # Write

    form.save()

    return HttpResponse(urllib.urlencode(response))


@login_or_session_required
def instance_uploadlog(request):
    uuid = request.POST['instance_id']
    error_log = request.POST['error_log']

    instance = ServerInstance.objects.get(uuid=uuid)
    requisitioner = instance.requisitioner

    # Stop running the activity. The instance must be manually repurposed.
    instance.unpurpose()

    content = '''
Crash log for server instance %s:
=================================

Requisitioner: %s

''' % (instance.user_interface, requisitioner.user.username if requisitioner is not None else '(None)') + error_log

    receive_error_log.send(None, instance=instance, requisitioner=requisitioner, content=content)

    send_email.send(None, subject='Crash log', body=content) # TODO: do via connection

    return HttpResponse()


@transaction.autocommit # Do a commit whenever we do a save, so the instance calling us back when we repurpose it will see new data
@login_required
def instance_unrequisition(request, uuid):
    instance = ServerInstance.objects.get(uuid=uuid)
    message = singleton_send(unrequisition_instance, request.account, instance=instance)
    request.session['message'] = message
    return HttpResponseRedirect('/tracker/account/')


## Activities

def activities(request):
    SORTABLES = {
        'name': 'Name',
    }

    glob = request.GET.get('glob', '*')

    only_mine = check_only_mine(request)

    objects = sorted_objects(
        request,
        'name',
        SORTABLES,
        Activity.objects if not only_mine else Activity.objects.filter(owner=request.account)
    )

    if glob != '*' and glob != '':
        objects = filter(lambda activity: fnmatch.fnmatch(activity.name, glob), objects)

    return direct_to_template(request, template="tracker/activities.html", extra_context={
        'activities': objects, 'sortables': SORTABLES,
        'filtration': filtration(glob, only_mine)
    })

def show_item_page(request, _class, class_name, item_or_uuid, form_class, template_name, item_name='item', extra_content={}, ownership_check=(lambda item, account: item.owner==account)):
    if type(item_or_uuid) in (str, unicode):
        item = _class.objects.get(uuid=item_or_uuid)
    else:
        item = item_or_uuid

    if request.user.is_authenticated():
        is_mine = ownership_check(item, request.account)
    else:
        is_mine = False

    if is_mine and request.method == 'POST':
        form = form_class(request.POST, instance=item)
        if form.is_valid():
            form.save()
            request.session['message'] = '%s successfully updated.' % class_name
            return HttpResponseRedirect('/tracker/%s/view/%s/' % (item_name, item.uuid))
    else:
        form = form_class(initial=dict(
            [(key, getattr(item, key)) for key in form_class.Meta.fields]
        ), instance=item)

    final_extra_content = {
        item_name: item,
        'form': form,
        'is_mine': is_mine,
    }
    final_extra_content.update(extra_content)
    return direct_to_template(request, template=template_name, extra_context=final_extra_content)

def activity(request, uuid):
    activity = Activity.objects.get(uuid=uuid)
    instances = ServerInstance.objects.filter(activity=activity)
    total_players = sum(map(lambda instance: instance.players, instances))
    return show_item_page(
        request,
        Activity,
        'Activity',
        activity,
        ActivityForm,
        'tracker/activity.html',
        'activity',
        extra_content = {
            'instances': instances,
            'total_players': total_players,
        },
    )

@login_required
def activity_delete(request, uuid):
    activity = Activity.objects.get(uuid=uuid)
    assert request.account.uuid == activity.owner.uuid, 'You cannot delete an activity you do not own.'
    activity.delete()

    request.session['message'] = 'Activity successfully deleted.'
    return HttpResponseRedirect('/tracker/activities/')

@transaction.autocommit # Do a commit whenever we do a save, so the instance calling us back when we repurpose it will see new data
@login_required
def activity_requisition(request, uuid):
    activity = Activity.objects.get(uuid=uuid)
    message = singleton_send(requisition_instance, request.account, activity=activity)
    request.session['message'] = message
    return HttpResponseRedirect('/tracker/account/')


## Assets

def assets(request):
    SORTABLES = {
        'location': 'Location',
        '_type': 'Type',
    }

    glob = request.GET.get('glob', '*')

    only_mine = check_only_mine(request)

    objects = sorted_objects(
        request,
        'location',
        SORTABLES,
        AssetInfo.objects if not only_mine else request.account.assetinfo_set
    )

    if glob != '*' and glob != '':
        objects = filter(lambda asset: fnmatch.fnmatch(asset.location, glob), objects)

    return direct_to_template(request, template="tracker/assets.html", extra_context={
        'assets': objects, 'sortables': SORTABLES,
        'filtration': filtration(glob, only_mine)
    })

def asset(request, uuid):
    asset = AssetInfo.objects.get(uuid=uuid)

    return show_item_page(
        request,
        AssetInfo,
        'Asset metadata',
        asset,
        AssetForm,
        'tracker/asset.html',
        'asset',
        ownership_check = lambda item, account: account.uuid in [owner.uuid for owner in asset.owners.all()],
    )

@login_required
def asset_new(request):
    asset = AssetInfo.objects.create(
        location = '?',
        hash_value = '',
        type_x = AssetInfo.TYPE.Both,
        kb_size = 0,
        comment = '',
    )
    asset.owners.add(request.account)
    asset.save()

    request.session['message'] = 'Asset successfully created.'
    return HttpResponseRedirect('/tracker/asset/view/%s/' % asset.uuid)

@login_or_session_required
def asset_upload(request, uuid):
    return HttpResponseRedirect(singleton_send(asset_upload_redirect, None, uuid=uuid))

@login_or_session_required 
def asset_download(request, uuid):
    return HttpResponseRedirect(singleton_send(asset_download_redirect, None, uuid=uuid))

@login_required
def asset_delete(request, uuid):
    asset = AssetInfo.objects.get(uuid=uuid)
    assert request.account.uuid in [owner.uuid for owner in asset.owners.all()], 'You cannot delete an asset you do not own.'
    asset.delete()

    asset_delete_trigger.send(None, asset_uuid=uuid)

    request.session['message'] = 'Asset successfully deleted.'
    return HttpResponseRedirect('/tracker/assets/')

@login_required
def asset_clone(request, uuid):
    asset = AssetInfo.objects.get(uuid=uuid)

## Old method:
##    # Use old owners, but make sure cloner is first in the list
##    old_owners = asset.owners.split(',')
##    new_owners = [request.account.uuid] + filter(lambda owner: owner != request.account.uuid, old_owners)
    new_owners = [request.account.uuid]

    clone_asset = AssetInfo.objects.create(
        location = asset.location,
        hash_value = asset.hash_value,
        type_x = asset.type_x,
        kb_size = asset.kb_size,
        comment = 'Clone of asset "%s"' % asset.comment,
    )
    clone_asset.owners.add(request.account)
    clone_asset.save()

    # Clone dependencies
    for dep in asset.dependencies.all():
        clone_asset.dependencies.add(dep)
    clone_asset.save()

    # Clone in the storage
    data = singleton_send(retrieve_asset, None, asset_uuid = asset.uuid)
    multiple_send(store_asset, None, asset = clone_asset, asset_file = File(data))

    request.session['message'] = 'Asset successfully cloned.'
    return HttpResponseRedirect('/tracker/asset/view/%s/' % clone_asset.uuid)

## If 'recurse': Gets all the dependency asset infos as well, recursively
@login_or_session_required
def asset_getinfo(request):
    uuid = request.GET['asset_id']
    base_asset = AssetInfo.objects.get(uuid=uuid)
    recurse = (request.GET.get('recurse') == '1')

    assets = { base_asset.uuid: base_asset }

    SEP = '$'

    # TODO: faster SQLs, and cache these - everyone playing the same map runs the same stuff
    if recurse:
        # Recurse all dependencies
        pool = [base_asset.uuid]
        while len(pool) > 0:
            temp_pool = pool
            pool = []
            for asset_uuid in temp_pool:
                asset = assets[asset_uuid]
                for dep in asset.dependencies.all():
                    old_size = len(assets)
                    assets[dep.uuid] = dep
                    if len(assets) > old_size:
                        pool.append(dep.uuid)

    asset_list = assets.values() # Rely on the order from now

    ret = [
        ('asset_id', SEP.join([asset.uuid for asset in asset_list])),
        ('location', SEP.join([asset.location for asset in asset_list])),
        ('url', SEP.join([
            (singleton_send(asset_download_redirect, None, uuid=asset.uuid) if asset.location[-1] != '/' else '') for asset in asset_list
        ])), # Directory locations do not have content, so no url for download
        ('hash', SEP.join([asset.hash_value for asset in asset_list])),
        ('type', SEP.join([asset.type_x for asset in asset_list])),
        ('dependencies', SEP.join([
            ','.join([dep.uuid for dep in asset.dependencies.all()]) for asset in asset_list
        ])),
    ]
    return HttpResponse(urllib.urlencode(ret))

## Activities

@login_required
def activity_new(request):
    asset = AssetInfo.objects.get(uuid=request.POST['asset_id'])
    try:
        activity = Activity.objects.create(
            asset = asset,
            name = ('Activity for "%s"' % asset.comment)[0:25], # Really can go up to 30 in the database, but keep it safe
            comment = '',
            owner=request.account,
        )
        request.session['message'] = 'Activity successfully created.'
        return HttpResponseRedirect('/tracker/activity/view/%s/' % activity.uuid)
    except Activity.InvalidAsset:
        request.session['message'] = 'Activity could not be created, as the asset is invalid (as a map asset).'
        return HttpResponseRedirect('/tracker/asset/view/%s/' % asset.uuid)


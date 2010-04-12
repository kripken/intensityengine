
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from django import template
from django.utils.safestring import SafeString
from django.utils.html import escape


register = template.Library()

def enum_key(enum, value):
    for key in enum.__dict__:
        if value == getattr(enum, key):
            return key
    return '?'

@register.filter
def instance_status(value):
    return enum_key(ServerInstance.STATUS, value)

@register.filter
def instance_mode(value):
    return enum_key(ServerInstance.MODE, value)

@register.filter
def hl_activity(value):
    """Renders a small 'headline' version of an activity, including brief summary, link, etc."""
    if value is None:
        return ''
    return SafeString('<a href="/tracker/activity/view/' + value.uuid + '/">' + value.name + '</a>')

@register.filter
def hl_account(value, my_account=None):
    if value is None or value == '':
        return ''
    nickname = escape(value.nickname) if my_account is None or value.uuid != my_account.uuid else "<b>Me (%s)</b>" % value.nickname
    return SafeString('<a href="/tracker/account/view/' + escape(value.uuid) + '/">' + nickname + '</a>')

@register.filter
def hl_instance(value):
    if value is None or value == '':
        return ''
    def zero_if_none(value): return value if value is not None else 0
    return SafeString('<a href="/tracker/instance/view/%s/">%s</a> [%s/%s : %s]' % tuple(map(
        lambda val: escape(str(val)),
        (value.uuid, value.user_interface, zero_if_none(value.players), zero_if_none(value.max_players), value.activity)
    )))

@register.filter
def hl_account_list(accounts, my_account=None):
    if accounts is None or accounts == '' or len(accounts) == 0:
        return ''
    return reduce(lambda x,y: x + SafeString(', ') + y, map(lambda account: hl_account(account, my_account), accounts))

@register.filter
def hl_asset(value):
    if value is None or value == '':
        return ''
    return SafeString('<a href="/tracker/asset/view/' + escape(value.uuid) + '/">' + value.location + '</a>')

@register.filter
def mul_1000(value):
    if value is None or value == '':
        return ''
    return value*1000


# Filtrations

def check_only_mine(request):
    if not request.user.is_authenticated(): return False
    
    if 'filtration' in request.GET:
        return request.GET.get('only_mine') == 'on'
    else:
        return True # By default, only mine

def filtration(glob, only_mine):
    return glob + '|' + str(int(only_mine))

@register.filter
def filtrationer(filtration, examples=None):
    glob, only_mine = filtration.split('|')
    if examples is not None:
        examples = ' (e.g., %s)' % examples
    else:
        examples = ''

    return SafeString('''
<form method="GET" action="">
    Pattern%s: <input type="text" name="glob" value="%s">
    <input type="checkbox" name="only_mine" %s>Show only mine
    <input type="hidden" name="filtration" value="1">
    <input type="submit" value="Apply">
</form>
''' % (examples, glob, 'checked' if only_mine == '1' else ''))


# Other

@register.filter
def timesizeformat(total_seconds):
    '''
    >>> timesizeformat(0)
    '0 seconds'
    >>> timesizeformat(1)
    '1 seconds'
    >>> timesizeformat(9)
    '9 seconds'
    >>> timesizeformat(11)
    '11 seconds'
    >>> timesizeformat(59)
    '59 seconds'
    >>> timesizeformat(60)
    '1:00 minutes'
    >>> timesizeformat(70) # Show seconds, not proportion! (Not seconds/60)
    '1:10 minutes'
    >>> timesizeformat(119)
    '1:59 minutes'
    >>> timesizeformat(60*3+42)
    '3:42 minutes'
    >>> timesizeformat(60*59+57)
    '59:57 minutes'
    >>> timesizeformat(60*60)
    '1:00:00 hours'
    >>> timesizeformat(60*60+53)
    '1:00:53 hours'
    >>> timesizeformat(60*60+42*60)
    '1:42:00 hours'
    >>> timesizeformat(60*60+51*60+37)
    '1:51:37 hours'
    >>> timesizeformat(60*60*85+24*60+16) # Don't do days - just hours
    '85:24:16 hours'
    '''

    try:
        total_seconds = int(total_seconds)
    except TypeError:
        return u"0 seconds"

    seconds = total_seconds % 60
    total_seconds = total_seconds/60
    minutes = total_seconds % 60
    hours = total_seconds/60

    time = '%02d' % seconds
    text = 'seconds'
    if minutes > 0 or hours > 0:
        time = ('%02d:' % minutes) + time
        text = 'minutes'
    if hours > 0:
        time = str(hours) + ':' + time
        text = 'hours'

    if time[0] == '0' and len(time) > 0: time = time[1:]

    return time + ' ' + text

if __name__ == '__main__':
    import doctest
    doctest.testmod()
else:
    from intensity.models import ServerInstance, UserAccount



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

from __future__ import with_statement

__COMPONENT_PRECEDENCE__ = 100

import os, threading

from intensity.tracker.signals import receive_error_log
import intensity.conf as intensity_conf


def get_account_error_log_dir(account):
    uuid = account.uuid
    _dir = os.path.join(intensity_conf.get_home_dir(), 'error_logs', uuid)
    if not os.path.exists(_dir):
        os.makedirs(_dir)
    return _dir

def get_last_error_log_index(account):
    _dir = get_account_error_log_dir(account)
    _max = -1
    for name in os.listdir(_dir):
        index = int(name.replace('.txt', ''))
        _max = max(_max, index)
    return _max

def get_error_log_path(account, index):
    return os.path.join(get_account_error_log_dir(account), str(index) + '.txt')

def get_new_error_log_path(account):
    index = get_last_error_log_index(account) + 1
    return get_error_log_path(account, index)

def get_error_log(account, index):
    try:
        return open(get_error_log_path(account, index), 'r').read()
    except:
        return 'Invalid error log index'


lock = threading.Lock()

def save_error_log(sender, **kwargs):
    instance = kwargs['instance']
    requisitioner = kwargs['requisitioner']
    content = kwargs['content']

    if requisitioner is None: return

    with lock:
        path = get_new_error_log_path(requisitioner)
        f = open(path, 'w')
        f.write(content)
        f.close()

receive_error_log.connect(save_error_log)


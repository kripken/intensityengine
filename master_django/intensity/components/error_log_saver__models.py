
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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


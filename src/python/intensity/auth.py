
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


def get_hexdigest(algorithm, salt, raw_password):
    '''
    Based on the BSD-licensed Django contrib.auth.models code
    '''

    raw_password, salt = str(raw_password), str(salt)
    if algorithm == 'crypt':
        try:
            import crypt
        except ImportError:
            raise ValueError('"crypt" password algorithm not supported in this environment')
        return crypt.crypt(raw_password, salt)
    # The rest of the supported algorithms are supported by hashlib, but
    # hashlib is only available in Python 2.5.
    try:
        import hashlib
    except ImportError:
        if algorithm == 'md5':
            import md5
            return md5.new(salt + raw_password).hexdigest()
        elif algorithm == 'sha1':
            import sha
            return sha.new(salt + raw_password).hexdigest()
    else:
        if algorithm == 'md5':
            return hashlib.md5(salt + raw_password).hexdigest()
        elif algorithm == 'sha1':
            return hashlib.sha1(salt + raw_password).hexdigest()
    raise ValueError("Got unknown password algorithm type in password.")

def get_full_password(algorithm, salt, raw_password):
    return '%s$%s$%s' % (algorithm, salt, get_hexdigest(algorithm, salt, raw_password))


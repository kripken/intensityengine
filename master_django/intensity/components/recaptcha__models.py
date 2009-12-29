
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


__COMPONENT_PRECEDENCE__ = 100

import httplib, urllib

from intensity.signals import prepare_security_check, verify_security_check
import intensity.conf as intensity_conf


IDENTIFIER = 'recaptcha'

def prepare(sender, **kwargs):
    public_key = intensity_conf.get('ReCaptcha', 'public_key')
    if public_key == '': return ''

    errors = kwargs.get('errors', [])
    errors = filter(lambda pair: pair[0] == IDENTIFIER, errors)
    error = errors[0][1] if len(errors) == 1 else ''

    return '''
<p>Please prove you are human:
<script>
var RecaptchaOptions = {
   theme : 'white',
   tabindex : 2
};
</script>
<script type="text/javascript"
   src="http://api.recaptcha.net/challenge?k=%(key)s&error=%(error)s">
</script>

<noscript>
   <iframe src="http://api.recaptcha.net/noscript?k=%(key)s&error=%(error)s">
       height="300" width="500" frameborder="0"></iframe><br>
   <textarea name="recaptcha_challenge_field" rows="3" cols="40">
   </textarea>
   <input type="hidden" name="recaptcha_response_field" 
       value="manual_challenge">
</noscript>
</p>
''' % { 'key': public_key, 'error': error }

prepare_security_check.connect(prepare, weak=False)

def verify(sender, **kwargs):
    private_key = intensity_conf.get('ReCaptcha', 'private_key')
    if private_key == '': return True

    request = kwargs['request']
    params = request.POST
    challenge = params.get('recaptcha_challenge_field')
    response = params.get('recaptcha_response_field')

    if challenge is None or response is None:
        return (IDENTIFIER, '')

    ip = request.META['REMOTE_ADDR']

    params_vec = [
        ('privatekey', private_key),
        ('remoteip', ip),
        ('challenge', challenge),
        ('response', response),
    ]

    conn = httplib.HTTPConnection('api-verify.recaptcha.net')
    headers = {'Content-type': 'application/x-www-form-urlencoded', 'Accept': 'text/plain'}
    conn.request('POST', '/verify', urllib.urlencode(params_vec), headers)
    response = conn.getresponse()
    assert(response.status == 200)
    data = response.read()
    conn.close()

    lines = data.split('\n')
    if lines[0] == 'true':
        return True
    else:
        return (IDENTIFIER, lines[1])

verify_security_check.connect(verify, weak=False)


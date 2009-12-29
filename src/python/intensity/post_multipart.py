# Adapted from http://code.activestate.com/recipes/146306/
# Thanks to Wade Leftwich

# See http://bugs.python.org/issue3244 for progress on an internal Python implementation


import httplib, mimetypes

from logging import *


## @reporthook: A function to be called every reporthook_blocksize bytes sent. This probably
##              makes the sending slower, as each blocksize is sent separately, and we wait
##              on its arrival.
def post_multipart(protocol, host, selector, fields, files, reporthook=None, reporthook_blocksize=None):
    """
    Post fields and files to an http host as multipart/form-data.
    fields is a sequence of (name, value) elements for regular form fields.
    files is a sequence of (name, filename, value) elements for data to be uploaded as files
    Return the Response instance
    """
    content_type, body = encode_multipart_formdata(fields, files)
    h = protocol(host)
    h.putrequest('POST', selector)
    h.putheader('content-type', content_type)
    h.putheader('content-length', str(len(body)))
    h.endheaders()

    if reporthook is None:
        h.send(body)
    else:
        i = 0
        while i < len(body):
            h.send(body[i:i+reporthook_blocksize])
            i += reporthook_blocksize
            reporthook(i)

    response = h.getresponse()
    h.close()
    return response

def encode_multipart_formdata(fields, files):
    """
    fields is a sequence of (name, value) elements for regular form fields.
    files is a sequence of (name, filename, value) elements for data to be uploaded as files
    Return (content_type, body) ready for httplib.HTTP instance
    """
    BOUNDARY = 'xxX14ThIs_Is_tHe_bouNdaRYX976xx'
    CRLF = '\r\n'
    L = []
    for (key, value) in fields:
        L.append('--' + BOUNDARY)
        L.append('Content-Disposition: form-data; name="%s"' % key)
        L.append('')
        assert(BOUNDARY not in value)
        L.append(value)
    for (key, filename, value) in files:
        L.append('--' + BOUNDARY)
        L.append('Content-Disposition: form-data; name="%s"; filename="%s"' % (key, filename))
        L.append('Content-Type: %s' % get_content_type(filename))
        L.append('')
        assert(BOUNDARY not in value)
        L.append(value)
    L.append('--' + BOUNDARY + '--')
    L.append('')
    body = CRLF.join(L)
    content_type = 'multipart/form-data; boundary=%s' % BOUNDARY
    return content_type, body

def get_content_type(filename):
    return mimetypes.guess_type(filename)[0] or 'application/octet-stream'


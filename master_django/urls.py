import os
ROOT_PATH = os.path.dirname(__file__)

from django.conf import settings
from django.conf.urls.defaults import *

from django.contrib import admin
admin.autodiscover()

import intensity.urls

urlpatterns = intensity.urls.urlpatterns

urlpatterns += patterns('',
    (r'^admin/(.*)', admin.site.root),
# In Django 1.1:    (r'^admin/', include(admin.site.urls)),
)

if settings.DEBUG:
    urlpatterns += patterns('django.views.static',
    (r'^static/(?P<path>.*)$', 
        'serve', {
        'document_root': os.path.join(ROOT_PATH, 'intensity', 'static'),
        'show_indexes': True }),)


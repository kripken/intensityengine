# Django settings for Intensity Engine - Django project.

import os
ROOT_PATH = os.path.dirname(__file__)

import intensity.conf as intensity_conf


DEBUG = True
TEMPLATE_DEBUG = DEBUG

ADMINS = (
    # ('Your Name', 'your_email@domain.com'),
)


MANAGERS = ADMINS

DATABASE_ENGINE = 'sqlite3'           # 'postgresql_psycopg2', 'postgresql', 'mysql', 'sqlite3' or 'oracle'.
DATABASE_NAME = os.path.join(intensity_conf.get_home_dir(), 'sqlite.db') # Or path to database file if using sqlite3.
DATABASE_USER = ''             # Not used with sqlite3.
DATABASE_PASSWORD = ''         # Not used with sqlite3.
DATABASE_HOST = ''             # Set to empty string for localhost. Not used with sqlite3.
DATABASE_PORT = ''             # Set to empty string for default. Not used with sqlite3.
DATABASE_OPTIONS = { 'timeout': 15 } # Wait more than the default 5 seconds for 'database is locked' errors

# Local time zone for this installation. Choices can be found here:
# http://en.wikipedia.org/wiki/List_of_tz_zones_by_name
# although not all choices may be available on all operating systems.
# If running in a Windows environment this must be set to the same as your
# system time zone.
TIME_ZONE = 'America/Chicago'

# Language code for this installation. All choices can be found here:
# http://www.i18nguy.com/unicode/language-identifiers.html
LANGUAGE_CODE = 'en-us'

SITE_ID = 1

# If you set this to False, Django will make some optimizations so as not
# to load the internationalization machinery.
USE_I18N = True

# Absolute path to the directory that holds media.
# Example: "/home/media/media.lawrence.com/"
MEDIA_ROOT = os.path.join(ROOT_PATH, 'intensity', 'static')

# URL that handles the media served from MEDIA_ROOT. Make sure to use a
# trailing slash if there is a path component (optional in other cases).
# Examples: "http://media.lawrence.com", "http://example.com/media/"
MEDIA_URL = ''

# URL prefix for admin media -- CSS, JavaScript and images. Make sure to use a
# trailing slash.
# Examples: "http://foo.com/media/", "/media/".
ADMIN_MEDIA_PREFIX = '/media/'

# Make this unique, and don't share it with anybody.
SECRET_KEY = '$a8i3m2=rni_aq&oah^8^+**+fm6eu8&4xne%&z_pg2gz@8659'

# List of callables that know how to import templates from various sources.
TEMPLATE_LOADERS = (
    'django.template.loaders.filesystem.load_template_source',
    'django.template.loaders.app_directories.load_template_source',
#     'django.template.loaders.eggs.load_template_source',
)

MIDDLEWARE_CLASSES = (
#    'django.middleware.cache.UpdateCacheMiddleware',
    'django.middleware.common.CommonMiddleware',
#    'django.middleware.cache.FetchFromCacheMiddleware',
    'django.contrib.sessions.middleware.SessionMiddleware',
    'django.contrib.auth.middleware.AuthenticationMiddleware',
    'django.middleware.transaction.TransactionMiddleware',
    'intensity.register.middleware.AccountMiddleware',
    'intensity.middleware.request_logger.RequestLoggerMiddleware',
)

ROOT_URLCONF = 'master_django.urls' # Should have the name of this directory

TEMPLATE_DIRS = (
    os.path.join(ROOT_PATH, 'intensity', 'templates'),
    os.path.join(ROOT_PATH, 'intensity', 'tracker', 'templates'),
    os.path.join(ROOT_PATH, 'intensity', 'components', 'templates'),
    os.path.join(intensity_conf.get_home_dir(), 'templates'),
)

INSTALLED_APPS = (
    'django.contrib.auth',
    'django.contrib.admin',
    'django.contrib.contenttypes',
    'django.contrib.humanize',
    'django.contrib.sessions',
    'django.contrib.sites',
    'intensity',
    'intensity.register',
    'intensity.tracker',
    'intensity.components',
)


# Use file-based sessions for speed
SESSION_ENGINE = "django.contrib.sessions.backends.file"
SESSION_FILE_PATH = os.path.join(intensity_conf.get_home_dir(), 'sessions')
if not os.path.exists(SESSION_FILE_PATH):
    os.mkdir(SESSION_FILE_PATH)


from django.conf.global_settings import TEMPLATE_CONTEXT_PROCESSORS

TEMPLATE_CONTEXT_PROCESSORS += (
    'django.core.context_processors.debug',
    'intensity.register.context_processors.account',
    'intensity.register.context_processors.toplevel',
)

if DEBUG:
    INTERNAL_IPS = ('127.0.0.1',)


# Intensity Engine specific settings

INTENSITY_PRODUCTION = False
INTENSITY_AUTH = False


# Caching

#CACHE_BACKEND = 'file://' + os.path.abspath( os.path.join(intensity_conf.get_home_dir(), 'django_cache') )
CACHE_BACKEND = 'locmem:///'

#CACHE_MIDDLEWARE_SECONDS = 5
#CACHE_MIDDLEWARE_KEY_PREFIX = ''


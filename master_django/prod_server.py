# This runs a CherryPy server, which is pure Python WSGI server that is
# multithreaded and production-ready. This setup will server both the
# dynamic Django content and the static content in intensity/static
# in an efficient manner (with expiration, etc.).
#
# When running in development mode, you can have Django serve the
# static content. This is done when DEBUG is set to True in the settings.
#
# Thanks to
#   http://www.eflorenzano.com/blog/post/hosting-django-site-pure-python/
#   http://www.arteme.fi/blog/2009/02/26/django-cherrypy-dev-server-and-static-files
#
# Tips: In production, you might want to server port 80 without being
#       in superuser, e.g., with authbind:
#           http://www.debian-administration.org/article/Running_network_services_as_a_non-root_user
#
# Some benchmarks of this vs. the dev server:
#
#   $ ab -n 1000 -c 10 http://localhost:8000/faq/ (dynamic content)
#   Dev: 100-165 reqs/sec
#   CherryPy: 170-200 reqs/sec
#
#   $ ab -n 1000 -c 10 http://localhost:8000/static/robots.txt (static content)
#   Dev: 333 reqs/sec
#   CherryPy: 333 reqs/sec


import os, sys

import cherrypy

import intensity.conf as intensity_conf

if len(sys.argv) >= 2:
    intensity_conf.set_home_dir(sys.argv[1])
else:
    intensity_conf.set_home_dir() # Use environment


# Auth servers serve using HTTPS, with a certificate in a subdirectory
# called ssl/, and use a different settings.py file
# (settings_production_auth.py).
auth = bool(int(intensity_conf.get('Network', 'auth')))

# Start and stop the CherryPy engine
#######cherrypy.config.update({'environment': 'embedded'})
if cherrypy.__version__ < '3.1':
    print "Starting engine..."
    cherrypy.engine.start(blocking=False)

# Dummy root for static dir
class StaticRoot:
    pass

if __name__ == '__main__':
    # General setup

    current_dir = os.path.dirname(os.path.abspath(__file__))

    cherrypy.config.update({
        'environment': 'production',
#        'log.error_file': 'log.cherrypy',
        'log.screen': True,
    })

    # Static dir serving

    static_conf = {
        '/': {
            'tools.staticdir.root': os.path.join(current_dir, 'intensity')
        },
        '/static': {
            'tools.staticdir.on': True,
            'tools.staticdir.dir': 'static',
            'tools.expires.secs': 60*60*24*4, # 4 days
        },
        '/favicon.ico': {
            'tools.staticfile.on': True,
            'tools.staticfile.filename': os.path.join(current_dir, 'intensity', 'static', 'favicon.ico'),
            'tools.expires.secs': 60*60*24*4, # 4 days
        },
        '/robots.txt': {
            'tools.staticfile.on': True,
            'tools.staticfile.filename': os.path.join(current_dir, 'intensity', 'static', 'robots.txt'),
            'tools.expires.secs': 60*60*24*4, # 4 days
        },
    }
#####################    cherrypy.quickstart(StaticRoot(), '/', config=conf) # Simple way to run just static dir

    static_app = cherrypy.tree.mount(StaticRoot(), '/', config=static_conf)

    # Dynamic Django serving

    sys.path += os.path.split(os.getcwd())[:-1] # Add parent, so Django can import the current directory as if a module
    os.environ['DJANGO_SETTINGS_MODULE'] = intensity_conf.get('Django', 'settings')
    import django.core.handlers.wsgi

    django_app = django.core.handlers.wsgi.WSGIHandler()

    from django.conf import settings
    assert(not settings.DEBUG) # Never use debug mode in production

    # Run main server - using CherryPy WSGIServer 3.1, copied locally

    import cherrypy.wsgiserver as wsgiserver

    dispatcher = wsgiserver.WSGIPathInfoDispatcher({
        '/': django_app,
        '/static': static_app,
        '/favicon.ico': static_app,
        '/robots.txt': static_app,
    })

    server = wsgiserver.CherryPyWSGIServer(
        ('0.0.0.0', int(intensity_conf.get('Network', 'port'))),
        dispatcher,
        server_name = intensity_conf.get('Network', 'address', 'localhost'),
        numthreads = int(intensity_conf.get('Network', 'num_threads')),
        request_queue_size = int(intensity_conf.get('Network', 'request_queue_size')),
        timeout = int(intensity_conf.get('Network', 'timeout', 10)),
    )

    if auth:
        server.ssl_certificate = os.path.join(intensity_conf.get_home_dir(), 'ssl', 'cert.crt')
        server.ssl_private_key = os.path.join(intensity_conf.get_home_dir(), 'ssl', 'cert.key')

## CherryPy trunk
##        import ssl_builtin
##        server.ssl_adapter = ssl_builtin.BuiltinSSLAdapter(
#        import ssl_pyopenssl
#        server.ssl_adapter = ssl_pyopenssl.pyOpenSSLAdapter(
#            os.path.join(intensity_conf.get_home_dir(), 'ssl', 'cert.crt'),
#            os.path.join(intensity_conf.get_home_dir(), 'ssl', 'cert.key'),
#        )

    try:
        print "    Starting server..."
        server.start()
    except KeyboardInterrupt:
        print "    ...stopping server"
        server.stop()
        if cherrypy.__version__ < '3.1':
            print "...stopping engine"
            cherrypy.engine.stop()


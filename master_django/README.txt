Intensity Engine - Master Server
================================


Production
----------

For production you will probably want to use
another database instead of Sqlite, which is used
by default. See the Django docs for how to do that.



Notes
-----

Django 1.0 is required; Django 1.1 currently does *not* work,
but hopefully soon will.


Troubleshooting
---------------

* If you log into the master on the web interface, but do not
    appear as logged in - yet you pass the login page - then
    the issue might be with Django sessions. This seems to
    occur in some cases on Windows systems, and you will see in
    home_dir/sessions/ some files of size 0. A workaround for
    this problem is to change

        SESSION_ENGINE = "django.contrib.sessions.backends.file"

    to

        SESSION_ENGINE = "django.contrib.sessions.backends.db"

    in settings.cfg, which will store sessions in the db
    (the default) rather than files, which are usually faster.


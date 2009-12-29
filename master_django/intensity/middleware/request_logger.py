# http://www.djangosnippets.org/snippets/1672/


import logging

from django.db import connection
from django.template import Template, Context

import intensity.conf as intensity_conf


#
# Log all SQL statements direct to the console (when running in DEBUG)
# Intended for use with the django development server.
#

class RequestLoggerMiddleware:
    def process_response(self, request, response):
        if intensity_conf.get('Django', 'logging', '0') == '1':
            if hasattr(request, 'user'):
                user_info = request.user.username if request.user.is_authenticated() else '<<anonymous>>'
            else:
                user_info = '<<anonymous>>'

            user_info += ' ' + request.META['REMOTE_ADDR']

            logging.info('%s - %s - %s' % (user_info, request.method, request.path))

            if len(connection.queries) > 0:
                time = sum([float(q['time']) for q in connection.queries])
                t = Template('''
    {{count}} quer{{count|pluralize:\"y,ies\"}} in {{time}} seconds:
        {% for sql in sqllog %}[{{forloop.counter}}] {{sql.time}}s: {{sql.sql|safe}}{% if not forloop.last %}\n        \n{% endif %}{% endfor %}
''')
                logging.info(t.render(Context({'sqllog':connection.queries,'count':len(connection.queries),'time':time})))
            else:
                logging.info('    (no queries)')

        return response


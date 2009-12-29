
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


"""
Simple component system for the Intensity Engine master server

Will try to import all .py files in intensity.components. Import
order will be done based on a variable called __COMPONENT_PRECEDENCE__
which should be used as follows:

__COMPONENT_PRECEDENCE__ = X

where X is a number. Components will be imported in order of decreasing
precedence, i.e., the highest precedence is imported first. See
load_components and the other functions in this module for more
details.

Components should communicate using the Django signals mechanism,
and no other way.
"""

import os, sys

MANAGER_DIR = os.path.dirname(__file__)

def load_components(postfix=''):
    postfix += '.py'
    components = []

    component_dir = os.path.join(MANAGER_DIR, 'components')

    for filename in filter(
        lambda filename: filename[-len(postfix):] == postfix and filename != '__init__.py',
        os.listdir(component_dir)
    ):
        content = open(os.path.join(component_dir, filename), 'r').read() + '\n'
        filename = filename[:-3]
        try:
            start = content.index('__COMPONENT_PRECEDENCE__')
        except ValueError:
            print 'Component "%s" does not define __COMPONENT_PRECEDENCE__, not loading' % filename
            continue

        end = content.index('\n', start)

        exec content[start:end] in globals(), locals()#p_globals, p_locals

        components.append( (__COMPONENT_PRECEDENCE__, filename) )

    components.sort(lambda x, y: cmp(y[0],x[0]))

    # Import each component, and return that actual module
    return map(lambda component: getattr(getattr(__import__('intensity.components.' + component[1], level=1), 'components'), component[1]), components)


def filter_components(components, _class):
    if len(components) == 0:
        return []
    else:
        ret = filter(
            lambda x: type(x) == type(_class) and issubclass(x, _class) and x is not _class,
            reduce(lambda x, y: x+y, [component.__dict__.values() for component in components])
        )
        return set(ret)

def load_models():
    from django.db.models import Model
    return filter_components(load_components('__models'), Model)

from django.conf.urls.defaults import *

def load_urls():
    components = load_components('__urls')
    urlpatternses = map(lambda component: component.urlpatterns, components)
    if len(urlpatternses) > 0:
        return reduce(lambda x,y: x+y, urlpatternses)
    else:
        return urlpatternses('',)

def load_tests():
    from django.test import TestCase
    return filter_components(load_components('__tests'), TestCase)


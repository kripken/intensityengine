
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from django.contrib import admin
from intensity.models import classes

for class_ in classes:
    admin.site.register(class_)


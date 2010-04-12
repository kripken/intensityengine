
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

'''
Load the models from all components, so syncdb will create them as necessary.
'''

import intensity.component_manager as component_manager
component_models = component_manager.load_models()
for model in component_models:
    # Make the syncdb etc. system aware of these
    if model.__name__ in globals().keys(): continue
    print "Exposing model", model.__name__
    exec "%s = model" % model.__name__


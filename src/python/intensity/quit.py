
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

## Quitting operations shared by both client and server

log(logging.DEBUG, "Python system quitting")

#save_config() # XXX Disable this for now, no clear use case requires it, and it annoys
#log(logging.DEBUG, "Saved config")

# Send shutdown signal
shutdown.send(None)


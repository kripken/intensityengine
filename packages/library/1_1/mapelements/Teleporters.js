
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Teleporter = AreaTrigger.extend({
    _class: "Teleporter",

    target: new StateArrayFloat(),

    clientOnCollision: function(collider) {
        collider.position = this.target; // Simply place the collider at the target
    }
});

registerEntityClass(Teleporter);


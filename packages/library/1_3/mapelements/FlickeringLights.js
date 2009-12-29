
/*
 *=============================================================================
 * Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
 *
 * This file is part of the Intensity Engine project,
 *    http://www.intensityengine.com
 *
 * The Intensity Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * The Intensity Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Intensity Engine.  If not, see
 *     http://www.gnu.org/licenses/
 *     http://www.gnu.org/licenses/agpl-3.0.html
 *=============================================================================
 */


//=============================================================================
// Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
//
// This file is part of the Intensity Engine project,
//    http://www.intensityengine.com
//
// The Intensity Engine is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, version 3.
//
// The Intensity Engine is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with the Intensity Engine.  If not, see
//     http://www.gnu.org/licenses/
//     http://www.gnu.org/licenses/agpl-3.0.html
//=============================================================================


registerEntityClass(bakePlugins(WorldMarker, [{
    _class: 'FlickeringLight',

    shouldAct: { client: true },

    radius: new StateFloat(),
    color: new StateInteger(),
    lightProbability: new StateFloat(),
    minDelay: new StateFloat(),
    maxDelay: new StateFloat(),

    init: function() {
        this.radius = 20;
        this.color = 0xFFFFFF;
        this.lightProbability = 0.5;
        this.minDelay = 0.1; // Do not cause epileptic seizures
        this.maxDelay = 0.333;
    },

    clientActivate: function() {
        this.delay = 0;
    },

    clientAct: function(seconds) {
        this.delay -= seconds;
        if (this.delay <= 0) {
            this.delay = Math.max(Math.random()*this.maxDelay, this.minDelay)*2;
            if (Math.random() < this.lightProbability) {
                Effect.addDynamicLight(this.position, this.radius, this.color, this.delay, 0, 1<<2, this.radius);
            }
        }
    },
}]));


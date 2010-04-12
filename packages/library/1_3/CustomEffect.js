
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

CustomEffect = {
    Fireworks: Action.extend({
        create: function(kwargs) {
            this._super(kwargs);

            this.kwargs = kwargs;
        },

        doExecute: function(seconds) {
            var newShots = [];

            this.kwargs.shots = filter(function(shot) {
                shot.position.add(shot.velocity.mulNew(seconds));
                shot.velocity.z -= seconds * World.gravity/2;
                if (!shot.color) {
                    shot.color = shot.getColor();
                }
                Effect.splash(PARTICLE.SPARK, 15*shot.size, Math.max(0.05, seconds*2), shot.position, shot.color, 1.0, 70*shot.size, 1);
                Effect.addDynamicLight(shot.position, shot.size*60, shot.color);
                shot.secondsLeft -= seconds;
                if (shot.secondsLeft > 0 &&
                    (!shot.minZ || shot.velocity.z > 0 || shot.position.z > shot.minZ) &&
                    (!shot.minVelocityZ || shot.velocity.z > shot.minVelocityZ)) {
                    return true;
                } else {
                    var num = Math.floor(Math.random()*15*shot.size);
                    for (var i = 0; i < num; i++) {
                        Effect.fireball(PARTICLE.EXPLOSION_NO_GLARE, shot.position, shot.size*5, 0.25, shot.color, shot.size*5);
                        Effect.addDynamicLight(shot.position, shot.size*90, shot.color, 0.25, 0.1, 0, 10);
//                        Sound.play("yo_frankie/DeathFlash.wav", shot.position.addNew(new Vector3(0, 0, 10/shot.size)));
                        if (shot.size >= 0.6) {
                            Sound.play('olpc/MichaelBierylo/sfx_DoorSlam.wav', shot.position);
                        }

                        newShots.push({
                            position: shot.position.copy(),
                            velocity: shot.velocity.addNew(
                                new Vector3(50*(Math.random()-0.5), 50*(Math.random()-0.5), 50*(Math.random()-0.5))
                            ),
                            minZ: shot.childMinZ,
                            childMinZ: shot.childMinZ,
                            secondsLeft: shot.childSecondsLeft*Math.random()*2,
                            childSecondsLeft: shot.childSecondsLeft*Math.random(),
                            getColor: shot.getColor,
                            size: shot.size/2.5,
                        });
                    }
                }
            }, this.kwargs.shots);

            this.kwargs.shots.push.apply(this.kwargs.shots, newShots);

            return this.kwargs.shots.length === 0;
        },
    }),

    explosion: function(center, position, radius, color, debrises, entity) {
        Effect.fireball(PARTICLE.EXPLOSION_NO_GLARE, center, radius, 1.0, color, 5);
        Sound.play("yo_frankie/DeathFlash.wav", center);
        Effect.addDecal(DECAL.SCORCH, position, new Vector3(0,0,1), 7, 0x000000);

        var num = debrises/2 + Math.ceil(Math.random()*debrises*2);
        for (var i = 0; i < num; i++) {
            GameManager.getSingleton().projectileManager.add(new Projectiles.debris(
                center.addNew(Random.normalizedVector3().mul(radius/3)),
                Random.normalizedVector3().mul(Math.random()*radius*2).add(new Vector3(0, 0, radius*1.333)),
                entity
            ));
        }
    },

    Rain: {
        start: function(params) { with(params) {
            this.drops = [];
            var worldSize = Editing.getWorldSize();
            this.addDropEvent = GameManager.getSingleton().eventManager.add({
                secondsBefore: 0,
                secondsBetween: frequency,
                func: bind(function() {
                    var camera = getPlayerEntity().position.copy();
                    var lx = Math.max(0, camera.x - radius);
                    var ly = Math.max(0, camera.y - radius);
                    var hx = Math.min(camera.x + radius, worldSize);
                    var hy = Math.min(camera.y + radius, worldSize);
                    var dx = hx-lx;
                    var dy = hy-ly;
                    var chance = dx*dy/Math.pow(worldSize, 2);
                    var amount = spawnAtOnce*chance;
                    if (this.drops.length + amount > maxAmount) {
                        amount = maxAmount - this.drops.length;
                    }
                    for (var i = 0; i < amount; i++) {
                        var origin = new Vector3(lx + Math.random()*dx, ly + Math.random()*dy, worldSize);
                        var floorDist = floorDistance(origin, worldSize*2);
                        if (floorDist < 0) floorDist = worldSize;
                        this.drops.push({
                            position: origin,
                            finalZ: origin.z - floorDist,
                        });
                    }
                }, this),
            }, this.addDropEvent);
            this.visualEffectEvent = GameManager.getSingleton().eventManager.add({
                secondsBefore: 0,
                secondsBetween: 0,
                func: bind(function() {
                    var delta = Global.currTimeDelta;
                    this.drops = filter(function(drop) {
                        var bottom = drop.position.copy();
                        bottom.z -= size;
                        Effect.flare(PARTICLE.STREAK, drop.position, bottom, 0, dropColor, 0.3);
                        drop.position.z -= speed*delta;
                        if (drop.position.z > drop.finalZ) {
                            /* // Experimental water splash code
                            if (World.getMaterial(drop.position) === MATERIAL.WATER) {
								drop.position.z = drop.position.z + 5;
                                Effect.addDecal(DECAL.BLOOD, drop.position, new Vector3(0,0,1), 7, 0x0000ff);
                                return false;
                            }
                            */
                            return true;
                        } else {
                            drop.position.z = drop.finalZ - 5;
                            Effect.splash(PARTICLE.SPARK, 15, 0.1, drop.position, splashColor, 1.0, 70, -1);
                            return false;
                        }
                    }, this.drops);
                }, this),
            }, this.visualEffectEvent);
        } },
    },
};


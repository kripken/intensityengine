
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
};


// (C) 2009 Alon 'Kripken' Zakai


Library.include('library/1_2/Events');
Library.include('library/1_2/CustomEffect');

KarmicKoala = registerEntityClass(bakePlugins(Mapmodel, [
    ClientParallelActionsPlugin,
    {
        _class: 'KarmicKoala',
        shouldAct: true,

        init: function() {
            this.modelName = 'koala';
        },

        clientActivate: function() {
            this.fireworkTimers = [new RepeatingTimer(2.0), new RepeatingTimer(2.0)];
            forEach(this.fireworkTimers, function(timer) {
                timer.tick(1.0 + Math.random()); // Speed up first shot
            });
        },

        clientAct: function(seconds) {
            forEach(this.fireworkTimers, function(timer) {
                if (timer.tick(seconds)) {
                    timer.tick(Math.random()); // Add about 0.5 seconds, so we actually fire each 1-2 seconds
                    Sound.play('olpc/MichaelBierylo/sfx_DoorSlam.wav', this.position.copy());

                    this.addParallelAction(new CustomEffect.Fireworks({
                        shots: [{
                            position: this.position.addNew(new Vector3(0, 0, 16)),
                            velocity: new Vector3(30*(Math.random()-0.5), 30*(Math.random()-0.5), 160 + Math.random()*80),
                            secondsLeft: 3.0,
                            childMinZ: this.position.z + 10,
                            minVelocityZ: -10,
                            childSecondsLeft: 0.6,
                            getColor: function() {
                                if (this.size === 1.0) return 0xFFEECC;
                                return Math.floor(Math.random()*255) + (Math.floor(Math.random()*255) << 8) + (Math.floor(Math.random()*255) << 16);
                            },
                            size: 1.0,
                        }],
                    }));
                }
            }, this);
        },
    },
]));


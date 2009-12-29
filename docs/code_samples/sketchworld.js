// (C) 2009 Alon 'Kripken' Zakai

Library.include('library/1_2/Plugins');
Library.include('library/1_2/GameManager');
Library.include('library/1_2/Health');
Library.include('library/1_2/Chat');
Library.include('library/1_2/mapelements/DetectionAreas');

// Default materials, etc.

Library.include('library/MapDefaults');

// Textures

Library.include('yo_frankie/');

// Map settings

Map.fogColor(0, 0, 0);
Map.fog(9999);
Map.loadSky("skyboxes/philo/sky3");
Map.skylight(100, 100, 100);
Map.ambient(20);
Map.shadowmapAmbient("0x101010");
Map.shadowmapAngle(300);

//// Player class

SketchPlugin = {
};

registerEntityClass(
    bakePlugins(
        Player,
        [
            GameManager.playerPlugin,
            Health.plugin,
            Chat.playerPlugin,
            SketchPlugin,
            {
                _class: "GamePlayer",

                markColor: new StateInteger({ clientSet: true }),
                newMark: new StateArrayFloat({ clientSet: true, hasHistory: false }), //, reliable: false }),
                clearMarks: new StateBoolean({ clientSet: true, hasHistory: false }),

                init: function() {
                    this.movementSpeed = 75;
                    this.clearMarks = true;
                },

                clientActivate: function() {
                    this.marks = [];

                    this.connect('client_onModify_newMark', this.onNewMark);
                    this.connect('client_onModify_clearMarks', this.resetMarks);
                },

                resetMarks: function() {
                    this.marks = [];

                    if (this === getPlayerEntity()) {
                        function randomChannel() { return integer(Math.random()*256); }
                        this.markColor = randomChannel() + (randomChannel() << 8) + (randomChannel() << 16);
                        Effect.splash(PARTICLE.SPARK, 15, 1.0, this.position.addNew(new Vector3(0,0,20)), this.markColor, 1.0, 70, 1);
                    }
                },

                clientAct: function(seconds) {
                    // Show marks
                    var color = this.markColor;
                    var last;
                    forEach(this.marks, function(mark) {
                        if (last && mark) {
                            Effect.flare(PARTICLE.STREAK, last, mark, 0, color, 1.0);
                            Effect.flare(PARTICLE.STREAK, mark, last, 0, color, 1.0);
                        }
                        last = mark;
                    });

                    // Create new mark, possibly
                    if (this === getPlayerEntity()) {
                        var newBatch = this.marks.length === 0 || !this.marks[this.marks.length-1];
                        var continuingBatch = this.marks.length > 0 && this.marks[this.marks.length-1];

                        if (continuingBatch) {
                            Effect.splash(PARTICLE.SPARK, 10, 0.15, this.marks[this.marks.length-1], color, 1.0, 25, 1);
                        }

                        if (this.pressing) {
                            var newPosition = CAPI.getTargetPosition();
                            var toPlayer = this.position.subNew(newPosition);
                            newPosition.add(toPlayer.normalize().mul(1.0)); // Bring a little out of the scenery
                            // Add if a new batch, or otherwise only if not too close
                            if (newBatch || !this.marks[this.marks.length-1].isCloseTo(newPosition, 5.0)) {
                                this.newMark = newPosition.asArray();
                            }
                        }
                    }
                },

                onNewMark: function(mark) {
                    if (mark.length === 3) {
                        mark = new Vector3(mark);
                    } else {
                        mark = null;
                    }
                    this.marks.push(mark);
                    // TODO: Expire old marks beyond a certain number of total marks
                },
            }
        ]
    )
);

//// Application

ApplicationManager.setApplicationClass(Application.extend({
    _class: "SketchWorldApp",

    getPcClass: function() {
        return "GamePlayer";
    },

    getScoreboardText: GameManager.getScoreboardText,

    // Replace this with appropriate behaviour for when a player falls of the map
    clientOnEntityOffMap: function(entity) {
        entity.position = [600,600,600];
    },

    clientClick: function(button, down, position, entity) {
        if (down && DetectionAreas.check(position, 'ignore')) return; // Ignore clicks in ignore areas

        var player = getPlayerEntity();
        if (!entity) {
            if (button === 1) {
                player.pressing = down;
            } else if (button === 2 && down) {
                player.newMark = []; // Separator
            }

            var anim = down ? ANIM_ATTACK1|ANIM_LOOP : ANIM_IDLE|ANIM_LOOP;
            if (player.animation !== anim) {
                player.animation = anim; // Do not send unnecessary messages
            }
        } else {
            // There are much better ways to code this sort of thing, but as a hack it will work
            if (down && entity._class === 'Mapmodel') {
                player.clearMarks = true;
            }
        }

        return true; // Handled
    },
}));

// Manager

GameManager.setup([
    GameManager.managerPlugins.messages,
    {
        clientActivate: function() {
            this.addHUDMessage({
                text: "Hints: Click, rightclick, click on tree",
                color: 0xDDEEFF,
                duration: 4.0,
                size: 0.8,
                y: 0.25,
            });
        },
    },
]);

//// Load permanent entities

if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile("./entities.json");
    loadEntities(entities);

    GameManager.getSingleton().registerTeams([
        {
            _name: 'sketchers',
            setup: function(player) {
                player.defaultModelName = 'frankie';
            },
        },
    ]);
}

// TODO: See pre-existing marks from before you logged in


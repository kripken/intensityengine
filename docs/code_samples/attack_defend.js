// Game mode where attackers score by capturing an item, and defenders score by keeping it safe over time
// Also has roles (classes)
//
// Used in Psycho 1.0 map.

Library.include('library/1_2/__CorePatches');
Library.include('library/1_2/Plugins');
Library.include('library/1_2/Firing');
Library.include('library/1_2/Health');
Library.include('library/1_2/Events');
Library.include('library/1_2/GameManager');
Library.include('library/1_2/modes/CTF');
Library.include('library/1_2/modes/Time');
Library.include('library/1_2/AutoTargeting');
Library.include('library/1_2/MultipartRendering');
Library.include('library/1_2/guns/Insta');
Library.include('library/1_2/guns/Stunball');
Library.include('library/1_2/guns/Rocket');
Library.include('library/1_2/guns/Shotgun');
Library.include('library/1_2/mapelements/JumpPads');
Library.include('library/1_2/mapelements/Cannons');
Library.include('library/1_2/Roles');

// Default materials, etc.

Library.include('library/MapDefaults');

// Textures

/*
Library.include('yo_frankie/');

Library.include('textures/gk/brick2/');
Library.include('textures/gk/concrete2/');
Library.include('textures/gk/deco2/');
Library.include('textures/gk/ground2/');
Library.include('textures/gk/maya2/');
Library.include('textures/gk/metal2/');
Library.include('textures/gk/morter2/');
Library.include('textures/gk/rock2/');
Library.include('textures/gk/roof2/');
Library.include('textures/gk/stone2/');
Library.include('textures/gk/wall2/');
Library.include('textures/gk/walls2/');
Library.include('textures/gk/wood2/');
*/


Library.include('textures/gk/brick3/');
Library.include('textures/gk/concrete3/');
Library.include('textures/gk/deco3/');
Library.include('textures/gk/ground3/');
Library.include('textures/gk/metal3/');
Library.include('textures/gk/morter3/');
Library.include('textures/gk/rock3/');
Library.include('textures/gk/walls3/');
Library.include('textures/gk/wood3/');

Library.include('yo_frankie/'); // TODO: Remove

// Map settings

Map.fogColor(0, 0, 0);
Map.fog(2500);
//Map.loadSky("skyboxes/blue_orange/1");
Map.loadSky("skyboxes/philo/sky3");
Map.skylight(50, 50, 50);
Map.ambient(45);
Map.shadowmapAmbient("0x101010");
Map.shadowmapAngle(300);

//// Player class

playerSniperGun = Firing.registerGun(new InstaGun(), 'Sniper Rifle', 'packages/hud/gui_gk_Icon_w03.png');
playerShotgun = Firing.registerGun(new Shotgun(), 'Shotgun', 'packages/hud/gui_gk_Icon_w04.png');
playerRocketLauncher = Firing.registerGun(new RocketGun(), 'Rocket Launcher', 'packages/hud/gui_gk_Icon_w02.png');
playerSeekingRocketLauncher = Firing.registerGun(new SeekingRocketGun(), 'Seeking Rocket Launcher', 'packages/hud/gui_gk_Icon_w01.png');

registerEntityClass(
    bakePlugins(
        Player,
        [
            Firing.plugins.protocol,
            Firing.plugins.player,
            Health.plugin,
            GameManager.playerPlugin,
            CTFMode.playerPlugin,
            Projectiles.plugin,
            StunballVictimPlugin,
            TargetLockingPlugin,
            Roles.plugin,
            {
                _class: "GamePlayer",

                requestTeam: new StateString({ hasHistory: false }),

                init: function() {
                    this.modelName = ''; // Start invisible, until spawned
                    this.canMove = false; // Start immobile, until spawned
                },

                activate: function() {
                    this.movementSpeed = 95; // Almost the same as sauer (100)

                    this.connect('onModify_requestTeam', function(team) {
                        GameManager.getSingleton().setPlayerTeam(this, team);
                    });
                },

                clientActivate: function() {
                    // XXX - remove when hudmodelname is a state var
                    this.connect('client_onModify_team', function(team) {
                        if (team === 'attackers') {
                            this.HUDModelName = 'stromar_1_1/hud/red';
                        } else if (team === 'defenders') {
                            this.HUDModelName = 'stromar_1_1/hud/blue';
                        } else if (team === 'spectators') {
                            this.HUDModelName = ''; // No hud for spectators
                        }
                    });

                    this.connect('client_onModify_role', function(role) {
                        if (role) {
                            Roles.set(this, role);
                        }
                    });

                    this.connect('clientRespawn', function() {
                        this.maxHealth = 100; // So when we set the true value, go up
                    });
                },

                clientAct: function() {
                    if (this === getPlayerEntity() && !this.doneSpectatorAction && this.team === 'spectators') {
                        this.doneSpectatorAction = true;
                        this.clearActions();
                        this.queueAction(new SpectatorAction());
                    }
                },
            }
        ]
    )
);

Roles.getPossible = function(entity) {
    return ['Assault', 'Defense', 'Trooper', 'Sniper'];
};

Roles.set = function(entity, role) {
    if (role === 'Assault') {
        entity.gunIndexes = [playerShotgun, playerRocketLauncher];
        entity.maxHealth = 250;
    } else if (role === 'Defense') {
        entity.gunIndexes = [playerSniperGun, playerSeekingRocketLauncher];
        entity.maxHealth = 250;
    } else if (role === 'Trooper') {
        entity.gunIndexes = [playerSniperGun, playerShotgun, playerRocketLauncher, playerSeekingRocketLauncher];
        entity.maxHealth = 150;
    } else if (role === 'Sniper') {
        entity.gunIndexes = [playerSniperGun];
        entity.maxHealth = 200;
    }

    entity.health = entity.maxHealth; // Update health
};


// Autocannons

function makeCannon(_name, gunClass, additionalPlugins) {
    var CannonGun = bakePlugins(gunClass, [CannonGunPlugin]);
    var cannonGun = new CannonGun();
    Firing.registerGun(cannonGun);

    var plugins = [AutoTargetingPlugin, MultipartRenderingPlugin, MultipartRenderingAutotargetingPlugin, CannonPlugin, Firing.plugins.protocol, BotFiringPlugin, CannonHealthPlugin, { _class: _name, gun: cannonGun }];
    additionalPlugins = defaultValue(additionalPlugins, []);
    return registerEntityClass( bakePlugins(Mapmodel, plugins.concat(additionalPlugins)) );

/*
    var plugins = [AutoTargetingPlugin, MultipartRenderingPlugin, MultipartRenderingAutotargetingPlugin, CannonPlugin, Firing.plugins.protocol, BotFiringPlugin, CannonHealthPlugin, {
        _class: (additionalPlugins ? 'middle__' : '' ) + _name, // middle name, if using a middle class
        gun: cannonGun,
    }];
    var ret = bakePlugins(Mapmodel, plugins);
    if (additionalPlugins) {
        additionalPlugins.push({ _class: _name });
        ret = bakePlugins(ret, additionalPlugins); // Additional inheritance step
    }
    return registerEntityClass(ret);
*/
}

makeCannon('DefensiveCannon', InstaGun, [{
    init: function() {
        this.autoTargetParams = {
            rotateSpeed: 90,
            minPitch: -45,
            maxPitch: 45,
            eyeHeight: 10, // Where we shoot from
            searchRadius: 150,
            seekDelay: 0.5, // How often to search for an entity to target
            fullSyncRate: 1.0/15,
        };
    },

    isValidTarget: function(target) {
        return Health.isValidTarget.call(this, target) && target.team === 'attackers' && !GameManager.getSingleton().disableAutoTargeting && hasLineOfSight(this.getTargetingOrigin(), target.getCenter());
    },
}]);


//// Application

ApplicationManager.setApplicationClass(Application.extend({
    _class: "GameApplication",

    getPcClass: function() {
        return "GamePlayer";
    },

    // Replace this with appropriate behaviour for when a player falls of the map
    clientOnEntityOffMap: Health.dieIfOffMap,

    getScoreboardText: GameManager.getScoreboardText,

    clientClick: Firing.clientClick,

    actionKey: function(index, down) {
        if (!down) return;
        if (index === 0) { // action key 0: Help
            UserInterface.showMessage("1-4: Select weapon. Middle mouse: Cycle weapons");
        } else if (index >= 1 && index <= 4) { // Or: bind MOUSE2 actionkey1
            var gunIndexes = getPlayerEntity().gunIndexes.asArray();
            if (findIdentical(gunIndexes, index-1) >= 0) {
                getPlayerEntity().currGunIndex = index-1;
            }
        } else if (index === 7) { // die
            getPlayerEntity().health = 0;
        }
//            GameManager.getSingleton().dropFlags(getPlayerEntity()); // TODO - buggy
    },
}));

//// Load permanent entities

GameManager.setup([
    merge(CTFMode.managerPlugin, { CTFMode: {
        score: 10.0, // 10 points per scoring
    }}),
    merge(TimeMode.managerPlugin, { timeMode: {
        interval: 6.0,
        score: 1.0, // 1 point per 6 seconds - 10 points per minute, same as 1 score by other team
        teams: ['defenders'],
        condition: function() {
            return ( (this.getFlagState(1) === CTFMode.FlagState.Based) && // If the item is safely at home, defenders get points
                     (this.teams['defenders'].playerList.length > 0 && this.teams['attackers'].playerList.length > 0) );
        }
    }}),
    merge(GameManager.managerPlugins.limitGameTime, { MAX_TIME: 10*60 }),
    merge(GameManager.managerPlugins.limitGameScore, { MAX_SCORE: 100 }),
    GameManager.managerPlugins.intermission,
    GameManager.managerPlugins.messages,
    GameManager.managerPlugins.balancer,
    AutoTargeting.managerPlugin,
    {
        activate: function() {
            this.oldPickTeam = this.pickTeam;
            this.pickTeam = this.customPickTeam;
        },

        clientActivate: function() {
            if (!this.shownWelcome) {
                this.addHUDMessage({
                    text: "Press 'H' for help.",
                    color: 0xCCFFAA,
                    duration: 2.5,
                    y: 0.333,
                    size: 0.5,
                });
                this.shownWelcome = true;
            }
        },

        //! Start in 'spec' team by default
        customPickTeam: function(player, sync) {
            var sync = defaultValue(sync, true);

            if (player.team === '') {
                this.setPlayerTeam(player, 'spectators', sync); // First team you get is spec
            } else {
                this.oldPickTeam(player, sync);
            }
        },
    },
]);

SpectatorAction = InputCaptureAction.extend({
    _name: 'SpectatorAction',

    canBeCancelled: false,
    canMultiplyQueue: false,

    done: false,

    clientClick: function() {
        return true;
    },

    actionKey: function(index, down) {
        if (!down) return;
        if (index === 0) {
            UserInterface.showMessage("Pick a team by pressing 1 or 2, or have fun spectating");
        } else if (index === 1) {
            Sound.play('gk/imp_01');
            this.actor.requestTeam = 'attackers';
            this.done = true;
        } else if (index === 2) {
            Sound.play('gk/imp_01');
            this.actor.requestTeam = 'defenders';
            this.done = true;
        }
    },

    doExecute: function() {
        CAPI.showHUDText('Press 1 for Attackers, 2 for Defenders.', 0.5, 0.8, 0.6, 0xFFCC55);

        return this.done;
    },
});


if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile("./entities.json");
    loadEntities(entities);

    GameManager.getSingleton().registerTeams([
        {
            _name: 'attackers',
            setup: function(player) {
                player.defaultModelName = 'stromar_1_1/red';
//XXX                player.HUDModelName = 'stromar_1_1/hud/red'; // apply when this is a state var, and remove the above XXX
            },
            flagModelName: null, // No flag, other team can't take anything
        },
        {
            _name: 'defenders',
            setup: function(player) {
                player.defaultModelName = 'stromar_1_1/blue';
//XXX                player.HUDModelName = 'stromar_1_1/hud/blue';
            },
            flagModelName: 'nut',
        },
        {
            _name: 'spectators',
            setup: function(player) {
                player.defaultModelName = 'stromar_1_1/yellow';
//XXX                player.HUDModelName = 'stromar_1_1/hud/blue';
            },
            kwargs: {
                ignoreForBalancing: true,
            },
        },
    ]);
}

Map.preloadModel('stromar/yellow');
Map.preloadModel('stromar/red');
Map.preloadModel('stromar/blue');
//Map.preloadModel('stromar/hud/red');  // Don't load, because (1) not everyone uses first person, 
//Map.preloadModel('stromar/hud/blue'); // (2) you may need only one of these, not both
Map.preloadModel('nut');

Map.preloadSound('gk/imp_01');


// (C) 2009 Alon 'Kripken' Zakai

// Default materials, etc.

Library.include('library/1_2/__CorePatches');
Library.include('library/1_2/Plugins');
Library.include('library/1_2/MapDefaults');
Library.include('library/1_2/Projectiles');
Library.include('library/1_2/Physics');

// Textures

Library.include('yo_frankie/');

// Map settings

Map.fogColor(0, 0, 0);
Map.fog(9999);
Map.loadSky("skyboxes/philo/sky3");
Map.skylight(100, 100, 100);
Map.ambient(20);
Map.shadowmapAmbient("0x505050");
Map.shadowmapAngle(300);

//// Player class

PendulumBall = Projectiles.Projectile.extend(Physics.plugins.ConstrainedProjectile).extend({
    gravity: 1.0,
    timeLeft: 10000,
    //render
});

registerEntityClass(bakePlugins(Player, [Projectiles.plugin, {
    _class: "GamePlayer",

    clientAct: function() {
        if (!this.pendulum) {
            this.pendulum = {
                top: getEntityByTag('top').position,
                bottom: getEntityByTag('bottom').position,
            };
            this.pendulum.radius = this.pendulum.top.subNew(this.pendulum.bottom).magnitude();

            this.pendulum.ball = new PendulumBall(this.pendulum.bottom.copy(), new Vector3(0,20,0), this);
            this.pendulum.ball.constraints.push(new Physics.constraints.Distance(this.pendulum.top.copy(), this.pendulum.radius));

            this.projectileManager.add(this.pendulum.ball);
        }

        Effect.flare(PARTICLE.STREAK, this.pendulum.top, this.pendulum.ball.position, 0, 0xFFFFFF, 1.0);

        if (this.pendulum.ball.velocity.magnitude() < 5 && this.pendulum.ball.position.subNew(this.pendulum.bottom).magnitude() < 5) {
            this.pendulum.ball.velocity = new Vector3(300*(Math.random()-0.5), 300*(Math.random()-0.5), 0);
            Sound.play("yo_frankie/DeathFlash.wav", this.pendulum.ball.position);
            Effect.fireball(PARTICLE.EXPLOSION, this.pendulum.ball.position, 10, 0.4, 0xFF0000, 10);

        }
    },
}]));

//// Application

ApplicationManager.setApplicationClass(Application.extend({
    _class: "GameApplication",

    getPcClass: function() {
        return "GamePlayer";
    },

    // Replace this with appropriate behaviour for when a player falls of the map
    clientOnEntityOffMap: function(entity) {
        entity.position = [600,600,600];
    }
}));

//// Load permanent entities

if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile("./entities.json");
    loadEntities(entities);
}


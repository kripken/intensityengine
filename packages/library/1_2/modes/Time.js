
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Add points every interval

TimeMode = {
    managerPlugin: {
        timeMode: {}, // Override

        activate: function() {
            this.connect('startGame', function() {
                this.timeMode.timer = new RepeatingTimer(this.timeMode.interval);
            });
        },

        act: function(seconds) {
            if (!this.gameRunning) return;

            if (this.timeMode.timer.tick(seconds)) {
                var changed = false;

                forEach(values(this.teams), function(team) {
                    if (findIdentical(this.timeMode.teams, team._name) >= 0) {
                        if (!this.timeMode.condition || this.timeMode.condition.apply(this)) {
                            team.score += this.timeMode.score;
                            changed = true;
                        }
                    }
                }, this);

                if (changed) {
                    this.syncTeamData();
                }
            }
        },
    },
};


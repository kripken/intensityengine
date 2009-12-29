
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


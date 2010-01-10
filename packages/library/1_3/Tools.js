
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


Tools = {
    //! Useful tool to replace globals. For example:
    //!     Tools.replaceFunction('Map.texture', function() { ... },);
    replaceFunction: function(originalName, replacement, callOriginal, _this) {
        callOriginal = defaultValue(callOriginal, true);
        var original = eval(originalName);
        function wrapper() {
            _this = defaultValue(_this, this); // Use global if not otherwise specified
            if (callOriginal) {
                original.apply(_this, arguments);
            }
            replacement.apply(_this, arguments);
        }
        eval(originalName + ' = wrapper');
    },
};


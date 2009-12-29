
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

eval(assert(' typeof Global === "object" '));
eval(assert(' typeof Global.initAsClient === "function" '));
eval(assert(' typeof Global.initAsServer === "function" '));

eval(assert(' !Global.CLIENT '));
eval(assert(' !Global.SERVER '));

Global.initAsClient();
eval(assert(' Global.CLIENT '));
eval(assert(' !Global.SERVER '));

Global.initAsServer();
eval(assert(' !Global.CLIENT '));
eval(assert(' Global.SERVER '));

// Restore initial state
Global.CLIENT = false;
Global.SERVER = false;


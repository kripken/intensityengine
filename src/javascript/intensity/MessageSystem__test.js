
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


if (Global.SERVER) {
    // SERVER

    var fb1 = [];
    var f1 = function(clientNumber, arg1, arg2) {
        fb1.push([clientNumber, arg1, arg2]);
    };

    MessageSystem.send(MessageSystem.ALL_CLIENTS, f1, 'this?', 'there');

    eval(assert(' serializeJSON([[-1, "this?", "there"]]) === serializeJSON(fb1) '));;

    var fb2 = [];
    var f2 = function(clientNumber, arg1, arg2, arg3) {
        fb2.push([clientNumber, arg1, arg2, arg3]);
    };

    var le = new LogicEntity();
    le.clientNumber = 1449;

    MessageSystem.send(le, f2, 'a1', 'b223', 'c4');

    log(DEBUG, serializeJSON(fb2));

    eval(assert(' serializeJSON([[1449, "a1", "b223", "c4"]]) === serializeJSON(fb2) '));;

    // Protocol stuff

    MessageSystem.generateProtocolData("__testclass", ['cat', 'dog', 'aarvark', 'peach', 'beachedwhale']);

    eval(assert(' typeof MessageSystem.toProtocolId("__testclass", "cat") === "number" '));
    eval(assert(' MessageSystem.fromProtocolId("__testclass",  MessageSystem.toProtocolId("__testclass", "cat") ) === "cat" '));

    eval(assert(' typeof MessageSystem.toProtocolId("__testclass", "dog") === "number" '));
    eval(assert(' MessageSystem.fromProtocolId("__testclass",  MessageSystem.toProtocolId("__testclass", "dog") ) === "dog" '));

    eval(assert(' typeof MessageSystem.toProtocolId("__testclass", "aarvark") === "number" '));
    eval(assert(' MessageSystem.fromProtocolId("__testclass",  MessageSystem.toProtocolId("__testclass", "aarvark") ) === "aarvark" '));

    eval(assert(' typeof MessageSystem.toProtocolId("__testclass", "peach") === "number" '));
    eval(assert(' MessageSystem.fromProtocolId("__testclass",  MessageSystem.toProtocolId("__testclass", "peach") ) === "peach" '));

    eval(assert(' typeof MessageSystem.toProtocolId("__testclass", "beachedwhale") === "number" '));
    eval(assert('MessageSystem.fromProtocolId("__testclass",MessageSystem.toProtocolId("__testclass","beachedwhale"))==="beachedwhale" '));

    MessageSystem.generateProtocolData("__testclass2", ['peach', 'cat', 'aarvark', 'beachedwhale', 'dog']); // different order

    eval(assert(' MessageSystem.toProtocolId("__testclass", "cat") === MessageSystem.toProtocolId("__testclass2", "cat") '));
    eval(assert(' MessageSystem.toProtocolId("__testclass", "dog") === MessageSystem.toProtocolId("__testclass2", "dog") '));
    eval(assert(' MessageSystem.toProtocolId("__testclass", "aarvark") === MessageSystem.toProtocolId("__testclass2", "aarvark") '));
    eval(assert(' MessageSystem.toProtocolId("__testclass", "peach") === MessageSystem.toProtocolId("__testclass2", "peach") '));
    eval(assert(' MessageSystem.toProtocolId("__testclass", "beachedwhale") === MessageSystem.toProtocolId("__testclass2", "beachedwhale") '));

    MessageSystem.removeClassInfo('__testclass');
    MessageSystem.removeClassInfo('__testclass2');

} else {
    // CLIENT
}


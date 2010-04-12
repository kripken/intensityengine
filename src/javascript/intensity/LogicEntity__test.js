
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Testing.replaceEnvironment();

// General

if (Global.CLIENT) {
    eval(assert(' LogicEntity === ClientLogicEntity '));
} else {
    // SERVER
    eval(assert(' LogicEntity === ServerLogicEntity '));
    eval(assert(' typeof LogicEntity.prototype.canCallCFuncs === "function" '));
}

// Server

if (Global.SERVER) {
    log(DEBUG, "Server tests");

    var ServerClass = ServerLogicEntity.extend({
        _class: "ServerClass",
        health: new StateInteger()
    });

    var test = new ServerClass();

    test.init(15);
    test.activate();

    eval(assert(" test.initialized "));
    eval(assert(" !test.deactivated "));

    log(DEBUG, "M1:" + MessageSystem.feedback);
    eval(assert(' arrayEqual(MessageSystem.feedback, ["MSG: 9 CAPI.LogicEntityCompleteNotification -1,15,ServerClass"]) '));


    eval(assert(' CAPI.feedback["setupNonSauer"] === 15 ' ));

    test.health = 18;
    eval(assert(" test.health === 18 "));

    log(DEBUG, "M2:" + MessageSystem.feedback);
    eval(assert(' arrayEqual(MessageSystem.feedback, ["MSG: 9 CAPI.LogicEntityCompleteNotification -1,15,ServerClass","MSG: 9 CAPI.StateDataUpdate 15,ServerClass|health,18"]) '));

    test.deactivate();

    eval(assert(" test.deactivated "));

    // Server testing of state data parsing

    // Wipe clean

    Testing.restoreEnvironment();
    Testing.replaceEnvironment();

    test = new ServerClass();

    var sd = serializeJSON( { health: 70172 } );

    test.init(15, { stateData: sd } );
    test.activate( { stateData: sd } );

    eval(assert(" test.initialized "));
    eval(assert(" !test.deactivated "));

    log(DEBUG, "M3:" + MessageSystem.feedback);

    eval(assert(" test.health === 70172 "));

    test.deactivate();

    eval(assert(" test.deactivated "));

    // Wipe clean

    Testing.restoreEnvironment();
    Testing.replaceEnvironment();

    // Tags

    log(DEBUG, "Tags tests - with init")

    test = new ServerClass();

    test.init(11);
    test.activate();

    test.initialized = true;
    eval(assert(' test.tags.length === 0 '));
    eval(assert(' !test.hasTag("TAG-ONE") '));
    test.addTag('TAG-ONE');
    eval(assert(' test.tags.length === 1 '));
    eval(assert(' test.hasTag("TAG-ONE") '));
    test.addTag('TAG-2');
    eval(assert(' test.tags.length === 2 '));
    eval(assert(' test.hasTag("TAG-ONE") '));
    eval(assert(' test.hasTag("TAG-2") '));
    log(DEBUG, "Now removing a tag");
    test.removeTag('TAG-ONE');
    log(DEBUG, "Removing done");
    eval(assert(' test.tags.length === 1 '));
    eval(assert(' !test.hasTag("TAG-ONE") '));
    eval(assert(' test.hasTag("TAG-2") '));

    log(DEBUG, "Tags tests - with tags from serialization")

    test = new ServerClass();

    sd = serializeJSON( { tags: "[]" } );

    test.init(15, { stateData: sd } );
    test.activate( { stateData: sd } );

    test.initialized = true;
    eval(assert(' test.tags.length === 0 '));
    eval(assert(' !test.hasTag("TAG-ONE") '));
    test.addTag('TAG-ONE');
    eval(assert(' test.tags.length === 1 '));
    eval(assert(' test.hasTag("TAG-ONE") '));
    test.addTag('TAG-2');
    eval(assert(' test.tags.length === 2 '));
    eval(assert(' test.hasTag("TAG-ONE") '));
    eval(assert(' test.hasTag("TAG-2") '));
    test.removeTag('TAG-ONE');
    eval(assert(' test.tags.length === 1 '));
    eval(assert(' !test.hasTag("TAG-ONE") '));
    eval(assert(' test.hasTag("TAG-2") '));

    log(DEBUG, "Final tags tests");
    test.tags = ["waka", "amazing", "three"];
    eval(assert(' test.tags.length === 3 '));
    eval(assert(' test.hasTag("waka") '));
    eval(assert(' test.hasTag("amazing") '));
    eval(assert(' test.hasTag("three") '));
    eval(assert(' !test.hasTag("TAG-ONE") '));
    eval(assert(' !test.hasTag("TAG-2") '));

    log(DEBUG, "Final tags tests (B)");
    test.tags = test.tags;
    eval(assert(' test.tags.length === 3 '));
    eval(assert(' test.hasTag("waka") '));
    eval(assert(' test.hasTag("amazing") '));
    eval(assert(' test.hasTag("three") '));
    eval(assert(' !test.hasTag("TAG-ONE") '));
    eval(assert(' !test.hasTag("TAG-2") '));


    // Events

    log(DEBUG, "Events tests")

    test = new ServerClass();

    test.init(11);
    test.activate();

    test.initialized = true;

    test.connect("onModify_health", function(value) {
        this.feedback = value + 11;
    });

    eval(assert(' test.feedback === undefined '));
    test.health = 50;
    eval(assert(' test.feedback === 61 '));

} else {
    // Client

    log(DEBUG, "Client tests");

    var ClientClass = ClientLogicEntity.extend({
        _class: "ClientClass",
        health: new StateInteger()
    });

    test = new ClientClass();

    test.uniqueId = 11;
    test.clientActivate();

    test.initialized = true;

    eval(assert(" test.initialized "));
    eval(assert(" !test.deactivated "));

    eval(assert(' CAPI.feedback["setupNonSauer"] === 11 '));
    eval(assert(" arrayEqual(MessageSystem.feedback, []) "));

    test.health = 1155;
    eval(assert(' arrayEqual(MessageSystem.feedback, ["MSG: CAPI.StateDataChangeRequest 11 ClientClass|health,1155,null"]) '));
    eval(assert(" test.health === undefined || isNaN(test.health) ")); // The change shouldn't go through

    test.clientDeactivate();

    eval(assert(" test.deactivated "));
}

// Clean up

Testing.restoreEnvironment();



// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

TAG_HORNS           = "tag_horns";
TAG_ARMOR           = "tag_armor";
TAG_WEAPON          = "tag_weapon";
TAG_LEFTWEAPON      = "tag_leftweapon";
TAG_WEAPON_PARTICLE = "tag_weapon_particle";

MODEL_HORNS = "mrfixit/horns";

MODEL_ARMOR_BLUE   = "mrfixit/armor/blue";
MODEL_ARMOR_GREEN  = "mrfixit/armor/green";
MODEL_ARMOR_YELLOW = "mrfixit/armor/yellow";

MODEL_WEAPON_PISTOL = "vwep/pistol";
MODEL_WEAPON_SHOTG  = "vwep/shotg";
MODEL_WEAPON_CHAING = "vwep/chaing";
MODEL_WEAPON_ROCKET = "vwep/rocket";
MODEL_WEAPON_RIFLE  = "vwep/rifle";
MODEL_WEAPON_GL     = "vwep/gl";

MODEL_SHIELD = "vwep/fist";

MODEL_SPARKLY = "particle/sparkly";

//! Generate a standard-form attachment string from a tag and a name, that the client knows how to parse for viewing
//! @param tag The tag for this attachment, i.e., where to attach this attachment.
//! @param name The model name for the attachment itself.
//! @return A string with a standard form of an attachment for the specified tag and name.
function modelAttachment(tag, _name) {
    eval(assert(' tag.indexOf(",") '));
    eval(assert(' _name.indexOf(",") '));

    return (tag + "," + _name);
}


// An invisible model that can be used to fill space and cause collisions there.
// This is useful if rendering is done in a dynamic manner, but we still want collisions.
//
// See comments in areatrigger

Model.shadow(0);
Model.collide(1);
Model.perEntityCollisionBoxes(1);
Model.ellipseCollide(1);


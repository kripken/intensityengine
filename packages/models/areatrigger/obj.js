// An inivisible model that can be used to fill space and detect when something passes through that space,
// i.e., as a trigger area
//

//mdlbb 10 5 0
Model.shadow(0);
Model.collide(1);
Model.collisionsOnlyForTriggering(1);
Model.perEntityCollisionBoxes(1);


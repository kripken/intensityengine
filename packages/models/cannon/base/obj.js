Model.objLoad("tris.obj");

Model.scale(20);

Model.objSkin("mesh", "../cc.jpg", "../sc.jpg");
Model.objBumpmap("mesh", "../nm.jpg");

Model.spec(50);
Model.glow(500);
Model.glare(0.5, 2);
Model.ellipseCollide(1);
Model.shader("bumpshader");
Model.ambient(2);


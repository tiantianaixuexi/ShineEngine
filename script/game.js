var posX = 0.0;
var posY = 0.0;

function update(dt) {
    posX += 1 * dt;
    posY += 0.5 * dt;
    if (typeof MoveActor === "function") {
        MoveActor(1, posX, posY);
    }
    if (typeof Log === "function") {
        Log("Actor position updated to (" + posX.toFixed(2) + ", " + posY.toFixed(2) + ")");
    }
}

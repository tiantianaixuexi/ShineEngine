declare function MoveActor(id: number, x: number, y: number): void;
declare function Log(msg: string): void;

// 脚本内存储数据（建议状态放C++中）
var posX = 0.0;
var posY = 0.0;

function update(dt: number) {
    posX += 1 * dt;
    posY += 0.5 * dt;
    MoveActor(1, posX, posY);
    Log(`Actor position updated to (${posX.toFixed(2)}, ${posY.toFixed(2)})`);
}


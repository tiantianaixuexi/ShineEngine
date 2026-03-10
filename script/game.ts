declare function Log(msg: string): void;
declare function SetTimeout(cb: () => void, delayMs: number): number;
declare function SetInterval(cb: () => void, intervalMs: number): number;
declare function ClearTimer(id: number): boolean;
declare function SetScriptEnabled(enabled: boolean): boolean;
declare function SetScriptTickInterval(intervalSeconds: number): boolean;

@SCLASS("DemoScriptActor")
class DemoScriptActor {
    @SPROPERTY({ type: "float", access: "ReadWrite", group: "移动" })
    motionScale = 1.0;

    @SPROPERTY({ type: "bool", access: "ReadWrite", group: "开关" })
    pulseEnabled = true;

    @SPROPERTY({ type: "float", access: "ReadWrite", group: "属性" })
    hp = true;

    @SPROPERTY({ type: "float", access: "ReadWrite", group: "属性", visible: true })
    max_hp = true;

    @SPROPERTY({ type: "array", access: "ReadWrite", group: "属性" })
    items: number[] = [1, 2, 3];

    @SPROPERTY({ type: "map", access: "ReadWrite", group: "属性" })
    properties: Record<string, string> = { "speed": "fast" };

    @SPROPERTY({ type: "string", access: "ReadOnly", group: "基础", visible: false })
    displayName = "DemoScriptActor";

    @SPROPERTY({ type: "array", access: "ReadWrite", group: "属性" })
    skills: string[] = ["slash", "kick"];

    private timeAccum = 0.0;
    private pulseTimerId = 0;

    @SFUNCTION()
    Init() {
        Log("Init called");
    }

    @SFUNCTION()
    Start() { 
        if (typeof SetScriptTickInterval === "function") {
            SetScriptTickInterval(0.033);
        }
        Log(`SCLASS=${this.displayName}`);
        if (typeof SetInterval === "function") {
            this.pulseTimerId = SetInterval(() => {
                //Log(`timer tick t=${this.timeAccum.toFixed(2)} scale=${this.motionScale}`);
            }, 1000);
        }

        SetTimeout(() => {
            Log("timeout fired");
            SetScriptEnabled(false);
            SetTimeout(() => {
                SetScriptEnabled(true);
            }, 1500);
        }, 1800);
    }

    @SFUNCTION()
    Update(dt: number) {
        this.timeAccum += dt * this.motionScale;
        if (!this.pulseEnabled) {
            return;
        }
    }

    @SFUNCTION()
    Destroy() {
        ClearTimer(this.pulseTimerId);
        this.pulseTimerId = 0;
        Log("Destroy called");
    }

    @SFUNCTION()
    OnEnable() {
        Log("OnEnable called");
    }

    @SFUNCTION()
    OnDisable() {
        Log("OnDisable called");
    }
}

const __script = new DemoScriptActor();
(globalThis as { __script?: DemoScriptActor }).__script = __script;

function Init() { __script.Init(); }
function Start() { __script.Start(); }
function update(dt: number) { __script.Update(dt); }
function Destroy() { __script.Destroy(); }
function OnEnable() { __script.OnEnable(); }
function OnDisable() { __script.OnDisable(); }


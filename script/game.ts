declare function Log(msg: string): void;
declare function SetTimeout(cb: () => void, delayMs: number): number;
declare function SetInterval(cb: () => void, intervalMs: number): number;
declare function ClearTimer(id: number): boolean;
declare function SetScriptEnabled(enabled: boolean): boolean;
declare function SetScriptTickInterval(intervalSeconds: number): boolean;
type LocalFieldType = "float" | "int" | "bool" | "string";
type LocalMetaRoot = { classes: Array<{ name: string; properties: Array<{ name: string; type: LocalFieldType }>; functions: Array<{ name: string }> }> };
type LocalClassConfig = string | { name: string };
type LocalPropertyConfig = { type: LocalFieldType; name?: string };
type LocalFunctionConfig = { name?: string };
const GMeta = (globalThis as Record<string, unknown>).__shine_meta as LocalMetaRoot;
const GClass = (globalThis as Record<string, unknown>).SCLASS as (config: LocalClassConfig) => ClassDecorator;
const GProperty = (globalThis as Record<string, unknown>).SPROPERTY as (config: LocalPropertyConfig) => PropertyDecorator;
const GProeprty = (globalThis as Record<string, unknown>).SPROEPRTY as (config: LocalPropertyConfig) => PropertyDecorator;
const GFunction = (globalThis as Record<string, unknown>).SFUNCTION as (config?: LocalFunctionConfig) => MethodDecorator;

@GClass("DemoScriptActor")
class DemoScriptActor {
    @GProperty({ type: "float" })
    motionScale = 1.0;

    @GProperty({ type: "bool" })
    pulseEnabled = true;

    @GProeprty({ type: "string" })
    displayName = "DemoScriptActor";

    private timeAccum = 0.0;
    private pulseTimerId = 0;

    @GFunction()
    Init() {
        Log("Init called");
    }

    @GFunction()
    Start() {
        if (typeof SetScriptTickInterval === "function") {
            SetScriptTickInterval(0.033);
        }
        Log(`SCLASS=${this.displayName} properties=${GMeta.classes[0]?.properties.length ?? 0} functions=${GMeta.classes[0]?.functions.length ?? 0}`);
        if (typeof SetInterval === "function") {
            this.pulseTimerId = SetInterval(() => {
                Log(`timer tick t=${this.timeAccum.toFixed(2)} scale=${this.motionScale}`);
            }, 1000);
        }
        if (typeof SetTimeout === "function") {
            SetTimeout(() => {
                Log("timeout fired");
                SetScriptEnabled(false);
                SetTimeout(() => {
                    SetScriptEnabled(true);
                }, 1500);
            }, 1800);
        }
    }

    @GFunction()
    Update(dt: number) {
        this.timeAccum += dt * this.motionScale;
        if (!this.pulseEnabled) {
            return;
        }
    }

    @GFunction()
    Destroy() {
        ClearTimer(this.pulseTimerId);
        this.pulseTimerId = 0;
        Log("Destroy called");
    }

    @GFunction()
    OnEnable() {
        Log("OnEnable called");
    }

    @GFunction()
    OnDisable() {
        Log("OnDisable called");
    }
}

const __script = new DemoScriptActor();

function Init() { __script.Init(); }
function Start() { __script.Start(); }
function update(dt: number) { __script.Update(dt); }
function Destroy() { __script.Destroy(); }
function OnEnable() { __script.OnEnable(); }
function OnDisable() { __script.OnDisable(); }


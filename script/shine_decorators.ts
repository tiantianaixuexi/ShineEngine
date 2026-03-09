declare function ReflectGetField(actorId: number, typeName: string, fieldName: string): unknown;
declare function ReflectSetField(actorId: number, typeName: string, fieldName: string, value: number | string | boolean | any[] | Record<string, any>): boolean;
declare function ReflectCallMethod(actorId: number, typeName: string, methodName: string, ...args: (number | string | boolean | any[] | Record<string, any>)[]): unknown;

type SFieldType = "float" | "int" | "bool" | "string" | "array" | "map" | "set" | string;
type SPropertyAccess = "ReadOnly" | "ReadWrite";
type SClassConfig = string | { name: string };
type SPropertyConfig = { type: SFieldType; name?: string; access?: SPropertyAccess; group?: string; visible?: boolean };
type SFunctionConfig = { name?: string };
type PropValue = number | string | boolean | any[] | Record<string, any>;

type ScriptPropertyMeta = { name: string; type: SFieldType; access: SPropertyAccess; group?: string; visible?: boolean };
type ScriptFunctionMeta = { name: string };
type ScriptClassMeta = { name: string; properties: ScriptPropertyMeta[]; functions: ScriptFunctionMeta[] };
type ScriptMetaRoot = { classes: ScriptClassMeta[] };

type ScriptReflectApi = {
    getField<T extends PropValue>(actorId: number, typeName: string, fieldName: string): T | undefined;
    setField(actorId: number, typeName: string, fieldName: string, value: PropValue): boolean;
    callMethod<T extends PropValue>(actorId: number, typeName: string, methodName: string, ...args: PropValue[]): T | undefined;
    getMeta(): ScriptMetaRoot;
};

const __shineMetaByCtor = new WeakMap<Function, ScriptClassMeta>();
const __shineMetaRoot: ScriptMetaRoot = { classes: [] };

function ensureClassMeta(ctor: Function): ScriptClassMeta {
    let meta = __shineMetaByCtor.get(ctor);
    if (meta) {
        return meta;
    }
    meta = { name: ctor.name, properties: [], functions: [] };
    __shineMetaByCtor.set(ctor, meta);
    return meta;
}

function ensureClassExport(meta: ScriptClassMeta): void {
    if (!__shineMetaRoot.classes.some((item) => item === meta)) {
        __shineMetaRoot.classes.push(meta);
    }
}

function SCLASS(config: SClassConfig): ClassDecorator {
    return (target) => {
        const meta = ensureClassMeta(target);
        meta.name = typeof config === "string" ? config : config.name;
        ensureClassExport(meta);
    };
}

type SPropertyArg = SFieldType | SPropertyAccess | boolean | Partial<SPropertyConfig> | string;

function SPROPERTY(...args: SPropertyArg[]): PropertyDecorator {
    return (target, propertyKey) => {
        const ctor = (target as { constructor: Function }).constructor;
        const meta = ensureClassMeta(ctor);
        
        let type: SFieldType = "float";
        let name = String(propertyKey);
        let access: SPropertyAccess = "ReadWrite";
        let group: string | undefined;
        let visible = true;

        for (const arg of args) {
            if (typeof arg === "string") {
                if (arg === "float" || arg === "int" || arg === "bool" || arg === "string" || 
                    arg.startsWith("array") || arg.startsWith("map") || arg.startsWith("set")) {
                    type = arg as SFieldType;
                } else if (arg === "ReadOnly" || arg === "ReadWrite") {
                    access = arg as SPropertyAccess;
                } else {
                    group = arg;
                }
            } else if (typeof arg === "boolean") {
                visible = arg;
            } else if (typeof arg === "object" && arg !== null) {
                const config = arg as SPropertyConfig;
                if (config.type) type = config.type;
                if (config.name) name = config.name;
                if (config.access) access = config.access;
                if (config.group) group = config.group;
                if (config.visible !== undefined) visible = config.visible;
            }
        }

        const existingIdx = meta.properties.findIndex((item) => item.name === name);
        const propMeta: ScriptPropertyMeta = { name, type, access, group, visible };
        if (existingIdx !== -1) {
            meta.properties[existingIdx] = propMeta;
        } else {
            meta.properties.push(propMeta);
        }
        ensureClassExport(meta);
    };
}

function SFUNCTION(config: SFunctionConfig | string = {}): MethodDecorator {
    return (target, propertyKey) => {
        const ctor = (target as { constructor: Function }).constructor;
        const meta = ensureClassMeta(ctor);
        const name = typeof config === "string" ? config : (config.name ?? String(propertyKey));
        if (!meta.functions.some((item) => item.name === name)) {
            meta.functions.push({ name });
        }
        ensureClassExport(meta);
    };
}

const SReflect: ScriptReflectApi = {
    getField<T extends PropValue>(actorId: number, typeName: string, fieldName: string): T | undefined {
        if (typeof ReflectGetField !== "function") {
            return undefined;
        }
        return ReflectGetField(actorId, typeName, fieldName) as T | undefined;
    },
    setField(actorId: number, typeName: string, fieldName: string, value: PropValue): boolean {
        if (typeof ReflectSetField !== "function") {
            return false;
        }
        return ReflectSetField(actorId, typeName, fieldName, value);
    },
    callMethod<T extends PropValue>(actorId: number, typeName: string, methodName: string, ...args: PropValue[]): T | undefined {
        if (typeof ReflectCallMethod !== "function") {
            return undefined;
        }
        return ReflectCallMethod(actorId, typeName, methodName, ...args) as T | undefined;
    },
    getMeta(): ScriptMetaRoot {
        return __shineMetaRoot;
    }
};

(globalThis as Record<string, unknown>).__shine_meta = __shineMetaRoot;
(globalThis as Record<string, unknown>).SCLASS = SCLASS;
(globalThis as Record<string, unknown>).SPROPERTY = SPROPERTY;
(globalThis as Record<string, unknown>).SFUNCTION = SFUNCTION;
(globalThis as Record<string, unknown>).SReflect = SReflect;

declare function ReflectGetField(actorId: number, typeName: string, fieldName: string): unknown;
declare function ReflectSetField(actorId: number, typeName: string, fieldName: string, value: number | string | boolean): boolean;
declare function ReflectCallMethod(actorId: number, typeName: string, methodName: string, ...args: (number | string | boolean)[]): unknown;

type SFieldType = "float" | "int" | "bool" | "string";
type SPropertyAccess = "ReadOnly" | "ReadWrite";
type SClassConfig = string | { name: string };
type SPropertyConfig = { type: SFieldType; name?: string; access?: SPropertyAccess; group?: string; visible?: boolean };
type SFunctionConfig = { name?: string };
type PropValue = number | string | boolean;

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

function SPROPERTY(config: SPropertyConfig): PropertyDecorator {
    return (target, propertyKey) => {
        const ctor = (target as { constructor: Function }).constructor;
        const meta = ensureClassMeta(ctor);
        const name = config.name ?? String(propertyKey);
        const access = config.access ?? "ReadWrite";
        const visible = config.visible ?? true;
        if (!meta.properties.some((item) => item.name === name)) {
            meta.properties.push({ name, type: config.type, access, group: config.group, visible });
        }
        ensureClassExport(meta);
    };
}

function SFUNCTION(config: SFunctionConfig = {}): MethodDecorator {
    return (target, propertyKey) => {
        const ctor = (target as { constructor: Function }).constructor;
        const meta = ensureClassMeta(ctor);
        const name = config.name ?? String(propertyKey);
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
(globalThis as Record<string, unknown>).SPROEPRTY = SPROPERTY;
(globalThis as Record<string, unknown>).SFUNCTION = SFUNCTION;
(globalThis as Record<string, unknown>).SReflect = SReflect;

(module $output.gl.raw.wasm
 (type $i32_=>_none (func (param i32)))
 (type $i32_=>_i32 (func (param i32) (result i32)))
 (type $i32_i32_=>_none (func (param i32 i32)))
 (type $i32_i32_=>_i32 (func (param i32 i32) (result i32)))
 (type $i32_i32_i32_=>_none (func (param i32 i32 i32)))
 (type $i32_f32_f32_i32_=>_none (func (param i32 f32 f32 i32)))
 (type $i32_i32_f32_=>_none (func (param i32 i32 f32)))
 (type $i32_f32_=>_none (func (param i32 f32)))
 (type $i32_i32_i32_i32_i32_=>_none (func (param i32 i32 i32 i32 i32)))
 (type $none_=>_i32 (func (result i32)))
 (type $i32_i32_i32_i32_=>_none (func (param i32 i32 i32 i32)))
 (type $i32_i32_i32_=>_i32 (func (param i32 i32 i32) (result i32)))
 (type $none_=>_none (func))
 (type $i32_i32_f32_f32_f32_f32_=>_none (func (param i32 i32 f32 f32 f32 f32)))
 (type $i32_f32_f32_f32_f32_f32_f32_f32_=>_none (func (param i32 f32 f32 f32 f32 f32 f32 f32)))
 (type $i32_f32_f32_=>_i32 (func (param i32 f32 f32) (result i32)))
 (type $f32_=>_f32 (func (param f32) (result f32)))
 (type $i32_i32_f32_f32_i32_=>_none (func (param i32 i32 f32 f32 i32)))
 (type $i32_f32_f32_f32_f32_f32_f32_=>_none (func (param i32 f32 f32 f32 f32 f32 f32)))
 (type $f32_=>_none (func (param f32)))
 (type $f32_f32_i32_=>_none (func (param f32 f32 i32)))
 (type $i32_i32_i32_i32_=>_i32 (func (param i32 i32 i32 i32) (result i32)))
 (type $i32_i32_i32_i32_i32_i32_=>_none (func (param i32 i32 i32 i32 i32 i32)))
 (type $i32_i32_i32_i32_i32_i32_i32_i32_i32_=>_none (func (param i32 i32 i32 i32 i32 i32 i32 i32 i32)))
 (type $i32_f32_f32_f32_f32_i32_=>_none (func (param i32 f32 f32 f32 f32 i32)))
 (type $i32_f32_f32_f32_f32_f32_i32_i32_i32_f32_i32_f32_f32_f32_f32_i32_=>_none (func (param i32 f32 f32 f32 f32 f32 i32 i32 i32 f32 i32 f32 f32 f32 f32 i32)))
 (type $i32_i32_i32_i32_i32_i32_=>_i32 (func (param i32 i32 i32 i32 i32 i32) (result i32)))
 (type $i32_i32_i32_i32_i32_i32_i32_=>_i32 (func (param i32 i32 i32 i32 i32 i32 i32) (result i32)))
 (type $f32_=>_i32 (func (param f32) (result i32)))
 (import "env" "gl_create_buffer" (func $gl_create_buffer (param i32) (result i32)))
 (import "env" "gl_create_vertex_array" (func $gl_create_vertex_array (param i32) (result i32)))
 (import "env" "gl_bind_vertex_array" (func $gl_bind_vertex_array (param i32 i32)))
 (import "env" "gl_bind_buffer" (func $gl_bind_buffer (param i32 i32 i32)))
 (import "env" "gl_enable_attribs" (func $gl_enable_attribs (param i32)))
 (import "env" "_Z29gl_create_program_from_sourceiPKcS0_" (func $gl_create_program_from_source\28int\2c\20char\20const*\2c\20char\20const*\29 (param i32 i32 i32) (result i32)))
 (import "env" "gl_get_uniform_location" (func $gl_get_uniform_location (param i32 i32 i32 i32) (result i32)))
 (import "env" "gl_buffer_data_f32" (func $gl_buffer_data_f32 (param i32 i32 i32 i32 i32)))
 (import "env" "memcmp" (func $memcmp (param i32 i32 i32) (result i32)))
 (import "env" "js_tex_load_url_sync" (func $js_tex_load_url_sync (param i32 i32 i32) (result i32)))
 (import "env" "js_tex_load_url" (func $js_tex_load_url (param i32 i32 i32 i32)))
 (import "env" "js_tex_get_wh" (func $js_tex_get_wh (param i32 i32) (result i32)))
 (import "env" "js_create_context" (func $js_create_context (param i32 i32) (result i32)))
 (import "env" "gl_submit" (func $gl_submit (param i32 i32 i32)))
 (import "env" "gl_setup_attribs_basic" (func $gl_setup_attribs_basic (param i32 i32)))
 (import "env" "gl_create_shader" (func $gl_create_shader (param i32 i32 i32 i32) (result i32)))
 (import "env" "gl_create_program_instanced" (func $gl_create_program_instanced (param i32 i32 i32) (result i32)))
 (import "env" "gl_setup_attribs_instanced" (func $gl_setup_attribs_instanced (param i32 i32 i32)))
 (import "env" "js_create_texture_checker" (func $js_create_texture_checker (param i32 i32) (result i32)))
 (memory $0 2)
 (data (i32.const 1024) "#version 300 es\nprecision mediump float;in vec2 aPos;in vec3 aCol;out vec2 vUV;uniform vec2 uViewSize;void main(){vUV=aCol.xy;vec2 nPos=(aPos/uViewSize)*2.0-1.0;gl_Position=vec4(nPos.x,-nPos.y,0.0,1.0);}\00\00\00\00\00#version 300 es\nprecision mediump float;in vec2 vUV;uniform sampler2D uTex;out vec4 outColor;void main(){outColor=texture(uTex,vUV);}\00\00\00\00\00\00\00\00\00\00\00#version 300 es\nprecision mediump float;in vec2 aPos;in vec3 aCol;out vec3 vCol;uniform vec2 uViewSize;void main(){vCol=aCol;vec2 nPos=(aPos/uViewSize)*2.0-1.0;gl_Position=vec4(nPos.x,-nPos.y,0.0,1.0);}\00\00\00\00\00\00#version 300 es\nprecision mediump float;in vec3 vCol;out vec4 outColor;void main(){outColor=vec4(vCol,1.0);}\00\00\00\00#version 300 es\nprecision mediump float;in vec2 vUV;uniform vec4 uColor;uniform vec4 uTexTint;uniform vec4 uBorderColor;uniform float uBorder;uniform vec4 uShadowColor;uniform vec2 uShadowOff;uniform float uShadowBlur;uniform float uShadowSpread;uniform vec2 uRad;uniform int uUseTex;uniform sampler2D uTex;out vec4 outColor;float sdfRoundRect(vec2 uv,vec2 rad){vec2 p=uv-vec2(0.5);vec2 q=abs(p)-(vec2(0.5)-rad);return length(max(q,0.0))+min(max(q.x,q.y),0.0)-min(rad.x,rad.y);}void main(){vec2 rad=clamp(uRad,vec2(0.0),vec2(0.5));float d=sdfRoundRect(vUV,rad);float aa=max(fwidth(d),0.0039);float fill=1.0-smoothstep(0.0,aa,d);float t=max(0.0,uBorder);float inner=1.0-smoothstep(-t,-t+aa,d);float border=clamp(fill-inner,0.0,1.0);vec4 base=uColor;if(uUseTex!=0)base*=texture(uTex,vUV)*uTexTint;vec4 cFill=vec4(base.rgb,base.a*fill);vec4 cBorder=vec4(uBorderColor.rgb,uBorderColor.a*border);float ds=sdfRoundRect(vUV-uShadowOff,rad)-uShadowSpread;float shadow=1.0-smoothstep(0.0,max(0.0,uShadowBlur)+aa,ds);vec4 cShadow=vec4(uShadowColor.rgb,uShadowColor.a*shadow);vec4 outc=cShadow;outc=outc+cBorder*(1.0-outc.a);outc=outc+cFill*(1.0-outc.a);outColor=outc;}\00\00\00\00\00\00\00\00\00\00\10\00\00\00\a8\0b\00\00\14\00\00\00\ad\0b\00\00\1c\00\01\00\ad\0b\00\00$\00\02\00\ad\0b\00\00(\00\02\00\a8\0b\00\00,\00\02\00\b7\0b\00\000\00\02\00\bf\0b\00\004\00\02\00\c6\0b\00\008\00\02\00\cb\0b\00\00<\00\02\00\d4\0b\00\00@\00\02\00\e1\0b\00\00D\00\02\00\e9\0b\00\00H\00\02\00\f6\0b\00\00L\00\02\00\01\0c\00\00P\00\02\00\0d\0c\00\00uTex\00uViewSize\00uUseTex\00uColor\00uRad\00uTexTint\00uBorderColor\00uBorder\00uShadowColor\00uShadowOff\00uShadowBlur\00uShadowSpread\00c\00\00\00\00#version 300 es\nprecision mediump float;in vec2 aPos;in vec3 aCol;out vec3 vCol;void main(){vCol=aCol;gl_Position=vec4(aPos,0.0,1.0);}\00\00\00\00\00\00\00\00\00\00#version 300 es\nprecision mediump float;in vec3 vCol;out vec4 outColor;void main(){outColor=vec4(vCol,1.0);}\00\00\00\00#version 300 es\nprecision mediump float;in vec2 aPos;in vec3 aCol;in vec3 aOffsetScale;in vec3 aICol;out vec3 vCol;void main(){vec2 pos=aOffsetScale.xy+aPos*aOffsetScale.z;gl_Position=vec4(pos,0.0,1.0);vCol=aICol;}\00\00\00\00\00\00\00\00\00\00\00\00\80\bf\00\00\80\bf\00\00\00\00\00\00\00\00\00\00\00\00\00\00\80?\00\00\80\bf\00\00\00\00\00\00\00\00\00\00\00\00\00\00\80?\00\00\80?\00\00\00\00\00\00\00\00\00\00\00\00\00\00\80\bf\00\00\80\bf\00\00\00\00\00\00\00\00\00\00\00\00\00\00\80?\00\00\80?\00\00\00\00\00\00\00\00\00\00\00\00\00\00\80\bf\00\00\80?\00\00\00\00\00\00\00\00\00\00\00\00Root\00Player\00Weapon\00asset/\e9\87\91\e5\b8\81.png\00\00\00\00\00\00\00\00\00\t\00\00\00\n\00\00\00\0b\00\00\00\0c\00\00\00\0d\00\00\00\0e\00\00\00\0f\00\00\00\00\00\00\00\00\00\00\00\10\00\00\00\11\00\00\00\12\00\00\00\13\00\00\00\14\00\00\00\15\00\00\00\16\00\00\00\17\00\00\00\18\00\00\00\00\00\00\00\00\00\00\00\10\00\00\00\19\00\00\00\12\00\00\00\13\00\00\00\14\00\00\00\15\00\00\00\1a\00\00\00\17\00\00\00\18\00\00\00\00\00\00\00\00\00\00\00\1b\00\00\00\1c\00\00\00\1d\00\00\00\1e\00\00\00\00\00\00\00\00\00\00\00\1f\00\00\00 \00\00\00!\00\00\00\"\00\00\00#\00\00\00$\00\00\00%\00\00\00\00\00\00\00\00\00\00\00\1f\00\00\00&\00\00\00\'\00\00\00\"\00\00\00(\00\00\00)\00\00\00*\00\00\00\00\00\00\00\00\00\00\00\1f\00\00\00+\00\00\00\'\00\00\00\"\00\00\00(\00\00\00,\00\00\00-\00\00\00\00\00\00\00\00\00\00\00.\00\00\00/\00\00\000\00\00\001\00\00\00\00\00\00\00\00\00\00\00\10\00\00\002\00\00\00\12\00\00\00\13\00\00\00\14\00\00\00\15\00\00\00\1a\00\00\00\17\00\00\00\18\00\00\00\00\00\00\00\00\00\00\00\10\00\00\003\00\00\00\12\00\00\00\13\00\00\00\14\00\00\00\15\00\00\00\1a\00\00\004\00\00\00\18\00\00\00\00\00\00\00\00\00\00\00\10\00\00\005\00\00\00\12\00\00\00\13\00\00\00\14\00\00\00\15\00\00\00\1a\00\00\00\17\00\00\006\00\00\00")
 (data (i32.const 4152) "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\01\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\01\00\00\00\00\00\00\00\00\00\00\00\01\00\00\00\00\00\80?\00\00\80?\00\00\80?\00\00\80?\cd\ccL?\cd\ccL?\cd\ccL?\00\00\80?fff?fff?fff?\00\00\80?\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\80?\00\00\80?\00\00\80?\00\00\80?")
 (table $0 55 55 funcref)
 (elem (i32.const 1) $__cxx_global_array_dtor $__cxx_global_array_dtor.1 $demo_rc_draw_rect_tex\28void*\2c\20int\2c\20float\2c\20float\2c\20float\2c\20float\29 $demo_rc_draw_rect_col\28void*\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\29 $DemoGame::onInit\28shine::engine::Engine&\29::$_2::__invoke\28shine::ui::Button*\29 $DemoGame::onInit\28shine::engine::Engine&\29::$_1::__invoke\28shine::ui::Button*\29 $DemoGame::onInit\28shine::engine::Engine&\29::$_0::__invoke\28shine::ui::Button*\29 $demo_on_mode_click\28shine::ui::Button*\29 $DemoGame::~DemoGame\28\29 $DemoGame::~DemoGame\28\29.1 $DemoGame::onInit\28shine::engine::Engine&\29 $DemoGame::onResize\28shine::engine::Engine&\2c\20int\2c\20int\29 $DemoGame::onUpdate\28shine::engine::Engine&\2c\20float\29 $DemoGame::onRender\28shine::engine::Engine&\2c\20float\29 $DemoGame::onPointer\28shine::engine::Engine&\2c\20float\2c\20float\2c\20int\29 $shine::game::Component::~Component\28\29 $PulseColor::~PulseColor\28\29 $shine::game::Component::kind\28\29\20const $shine::game::Component::isOwnedByDead\28\29\20const $shine::game::Component::onAttach\28\29 $shine::game::Component::onDetach\28\29 $PulseColor::onUpdate\28float\29 $shine::game::Component::onRender\28shine::game::RenderContext&\2c\20float\29 $shine::game::Component::onPointer\28float\2c\20float\2c\20int\29 $shine::game::Component::~Component\28\29.1 $shine::game::Component::onUpdate\28float\29 $shine::game::Object::~Object\28\29 $shine::game::Object::~Object\28\29.1 $__cxa_pure_virtual $shine::game::Object::isOwnedByDead\28\29\20const $shine::ui::Element::~Element\28\29.1 $shine::ui::Button::~Button\28\29 $shine::ui::Button::init\28\29 $shine::ui::Element::hit\28float\2c\20float\29\20const $shine::ui::Button::pointer\28float\2c\20float\2c\20int\29 $shine::ui::Button::onResize\28int\2c\20int\29 $shine::ui::Button::render\28int\29 $shine::ui::Element::~Element\28\29 $shine::ui::Element::init\28\29 $shine::ui::Element::pointer\28float\2c\20float\2c\20int\29 $shine::ui::Element::onResize\28int\2c\20int\29 $shine::ui::Element::render\28int\29 $shine::ui::Image::~Image\28\29 $shine::ui::Image::onResize\28int\2c\20int\29 $shine::ui::Image::render\28int\29 $shine::game::Node::~Node\28\29 $shine::game::Node::~Node\28\29.1 $shine::game::Node::kind\28\29\20const $shine::game::Node::isOwnedByDead\28\29\20const $shine::game::Transform::~Transform\28\29 $shine::game::SpriteRenderer::~SpriteRenderer\28\29 $shine::game::SpriteRenderer::onRender\28shine::game::RenderContext&\2c\20float\29 $KillOnClick::~KillOnClick\28\29 $KillOnClick::onPointer\28float\2c\20float\2c\20int\29)
 (global $__stack_pointer (mut i32) (i32.const 103152))
 (export "memory" (memory $0))
 (export "init" (func $init.command_export))
 (export "resize" (func $resize.command_export))
 (export "frame" (func $frame.command_export))
 (export "pointer" (func $pointer.command_export))
 (export "malloc" (func $malloc.command_export))
 (export "free" (func $free.command_export))
 (export "on_tex_loaded" (func $on_tex_loaded.command_export))
 (export "on_tex_failed" (func $on_tex_failed.command_export))
 (func $__wasm_call_ctors
  (call $_GLOBAL__sub_I_renderer_2d.cpp)
  (call $_GLOBAL__sub_I_ui_manager.cpp)
 )
 (func $init (param $0 i32)
  (call $shine::engine::Engine::init\28int\29
   (call $shine::engine::Engine::instance\28\29)
   (local.get $0)
  )
 )
 (func $resize (param $0 i32) (param $1 i32)
  (call $shine::engine::Engine::onResize\28int\2c\20int\29
   (call $shine::engine::Engine::instance\28\29)
   (local.get $0)
   (local.get $1)
  )
 )
 (func $frame (param $0 f32)
  (call $shine::engine::Engine::frame\28float\29
   (call $shine::engine::Engine::instance\28\29)
   (local.get $0)
  )
 )
 (func $pointer (param $0 f32) (param $1 f32) (param $2 i32)
  (call $shine::engine::Engine::pointer\28float\2c\20float\2c\20int\29
   (call $shine::engine::Engine::instance\28\29)
   (local.get $0)
   (local.get $1)
   (local.get $2)
  )
 )
 (func $malloc (param $0 i32) (result i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (block $label$1
   (br_if $label$1
    (local.get $0)
   )
   (return
    (i32.const 0)
   )
  )
  (block $label$2
   (br_if $label$2
    (local.tee $1
     (i32.load offset=4648
      (i32.const 0)
     )
    )
   )
   (i32.store offset=4648
    (i32.const 0)
    (local.tee $1
     (i32.and
      (i32.add
       (i32.const 103152)
       (i32.const 15)
      )
      (i32.const -16)
     )
    )
   )
  )
  (local.set $3
   (i32.and
    (local.tee $2
     (i32.add
      (local.get $0)
      (i32.const 15)
     )
    )
    (i32.const -16)
   )
  )
  (local.set $4
   (i32.const 4640)
  )
  (block $label$3
   (loop $label$4
    (br_if $label$3
     (i32.eqz
      (local.tee $0
       (i32.load
        (local.tee $5
         (local.get $4)
        )
       )
      )
     )
    )
    (local.set $4
     (i32.add
      (local.get $0)
      (i32.const 4)
     )
    )
    (br_if $label$4
     (i32.lt_u
      (local.tee $6
       (i32.load
        (local.get $0)
       )
      )
      (local.get $3)
     )
    )
   )
   (i32.store
    (local.get $5)
    (i32.load offset=4
     (local.get $0)
    )
   )
   (block $label$5
    (br_if $label$5
     (i32.gt_u
      (i32.add
       (local.tee $4
        (i32.and
         (i32.add
          (i32.or
           (local.get $2)
           (i32.const 15)
          )
          (local.tee $5
           (i32.add
            (local.get $0)
            (i32.const 8)
           )
          )
         )
         (i32.const -16)
        )
       )
       (i32.const 24)
      )
      (local.tee $6
       (i32.add
        (local.get $6)
        (local.get $5)
       )
      )
     )
    )
    (i32.store
     (local.get $4)
     (i32.add
      (i32.sub
       (local.get $6)
       (local.get $4)
      )
      (i32.const -8)
     )
    )
    (local.set $6
     (i32.load offset=4640
      (i32.const 0)
     )
    )
    (i32.store offset=4640
     (i32.const 0)
     (local.get $4)
    )
    (i32.store offset=4
     (local.get $4)
     (local.get $6)
    )
    (i32.store
     (local.get $0)
     (local.get $3)
    )
   )
   (i32.store offset=4644
    (i32.const 0)
    (i32.add
     (i32.load offset=4644
      (i32.const 0)
     )
     (i32.const 1)
    )
   )
   (return
    (local.get $5)
   )
  )
  (block $label$6
   (br_if $label$6
    (i32.le_u
     (local.tee $4
      (i32.add
       (local.tee $5
        (i32.or
         (local.tee $0
          (i32.and
           (i32.add
            (local.get $1)
            (i32.const 15)
           )
           (i32.const -16)
          )
         )
         (i32.const 8)
        )
       )
       (local.get $3)
      )
     )
     (local.tee $6
      (i32.shl
       (memory.size)
       (i32.const 16)
      )
     )
    )
   )
   (br_if $label$6
    (i32.ne
     (memory.grow
      (i32.shr_u
       (i32.add
        (i32.sub
         (local.get $4)
         (local.get $6)
        )
        (i32.const 65535)
       )
       (i32.const 16)
      )
     )
     (i32.const -1)
    )
   )
   (i32.store offset=4652
    (i32.const 0)
    (i32.add
     (i32.load offset=4652
      (i32.const 0)
     )
     (i32.const 1)
    )
   )
   (return
    (i32.const 0)
   )
  )
  (i32.store
   (local.get $0)
   (local.get $3)
  )
  (i32.store offset=4
   (local.get $0)
   (i32.const 0)
  )
  (i32.store offset=4648
   (i32.const 0)
   (local.get $4)
  )
  (i32.store offset=4644
   (i32.const 0)
   (i32.add
    (i32.load offset=4644
     (i32.const 0)
    )
    (i32.const 1)
   )
  )
  (local.get $5)
 )
 (func $free (param $0 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $0)
    )
   )
   (i32.store offset=4
    (local.tee $0
     (i32.add
      (local.get $0)
      (i32.const -8)
     )
    )
    (i32.load offset=4640
     (i32.const 0)
    )
   )
   (i32.store offset=4640
    (i32.const 0)
    (local.get $0)
   )
   (i32.store offset=4656
    (i32.const 0)
    (i32.add
     (i32.load offset=4656
      (i32.const 0)
     )
     (i32.const 1)
    )
   )
  )
 )
 (func $operator\20new\28unsigned\20long\29 (param $0 i32) (result i32)
  (block $label$1
   (br_if $label$1
    (local.tee $0
     (call $malloc
      (local.get $0)
     )
    )
   )
   (unreachable)
  )
  (local.get $0)
 )
 (func $operator\20delete\28void*\2c\20unsigned\20long\29 (param $0 i32) (param $1 i32)
  (call $free
   (local.get $0)
  )
 )
 (func $__cxa_atexit (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  (i32.const 0)
 )
 (func $__cxa_pure_virtual
 )
 (func $shine::wasm::svector_reserve_impl\28void**\2c\20unsigned\20int*\2c\20unsigned\20int\2c\20unsigned\20int\2c\20unsigned\20int\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $4)
    )
   )
   (br_if $label$1
    (i32.le_u
     (local.get $3)
     (i32.load
      (local.get $1)
     )
    )
   )
   (br_if $label$1
    (i32.eqz
     (local.tee $5
      (call $malloc
       (i32.mul
        (local.get $4)
        (local.get $3)
       )
      )
     )
    )
   )
   (local.set $6
    (i32.load
     (local.get $0)
    )
   )
   (block $label$2
    (block $label$3
     (block $label$4
      (br_if $label$4
       (i32.eqz
        (local.get $2)
       )
      )
      (br_if $label$4
       (i32.eqz
        (local.get $6)
       )
      )
      (local.set $4
       (i32.mul
        (local.get $4)
        (local.get $2)
       )
      )
      (local.set $2
       (local.get $6)
      )
      (local.set $7
       (local.get $5)
      )
      (loop $label$5
       (br_if $label$3
        (i32.eqz
         (local.get $4)
        )
       )
       (i32.store8
        (local.get $7)
        (i32.load8_u
         (local.get $2)
        )
       )
       (local.set $4
        (i32.add
         (local.get $4)
         (i32.const -1)
        )
       )
       (local.set $2
        (i32.add
         (local.get $2)
         (i32.const 1)
        )
       )
       (local.set $7
        (i32.add
         (local.get $7)
         (i32.const 1)
        )
       )
       (br $label$5)
      )
     )
     (br_if $label$2
      (i32.eqz
       (local.get $6)
      )
     )
    )
    (i32.store offset=4
     (local.tee $4
      (i32.add
       (local.get $6)
       (i32.const -8)
      )
     )
     (i32.load offset=4640
      (i32.const 0)
     )
    )
    (i32.store offset=4640
     (i32.const 0)
     (local.get $4)
    )
    (i32.store offset=4656
     (i32.const 0)
     (i32.add
      (i32.load offset=4656
       (i32.const 0)
      )
      (i32.const 1)
     )
    )
   )
   (i32.store
    (local.get $1)
    (local.get $3)
   )
   (i32.store
    (local.get $0)
    (local.get $5)
   )
  )
 )
 (func $shine::graphics::CommandBuffer::instance\28\29 (result i32)
  (i32.const 4668)
 )
 (func $shine::graphics::CommandBuffer::reset\28\29 (param $0 i32)
  (i64.store offset=32776 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=32768 align=4
   (local.get $0)
   (i64.const 0)
  )
 )
 (func $shine::graphics::CommandBuffer::push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (param $5 i32) (param $6 i32) (param $7 i32) (param $8 i32)
  (local $9 i32)
  (block $label$1
   (br_if $label$1
    (i32.gt_s
     (local.tee $9
      (i32.load offset=32768
       (local.get $0)
      )
     )
     (i32.const 1023)
    )
   )
   (i32.store
    (local.tee $9
     (i32.add
      (local.get $0)
      (i32.shl
       (local.get $9)
       (i32.const 5)
      )
     )
    )
    (local.get $1)
   )
   (i32.store offset=28
    (local.get $9)
    (local.get $8)
   )
   (i32.store offset=24
    (local.get $9)
    (local.get $7)
   )
   (i32.store offset=20
    (local.get $9)
    (local.get $6)
   )
   (i32.store offset=16
    (local.get $9)
    (local.get $5)
   )
   (i32.store offset=12
    (local.get $9)
    (local.get $4)
   )
   (i32.store offset=8
    (local.get $9)
    (local.get $3)
   )
   (i32.store offset=4
    (local.get $9)
    (local.get $2)
   )
   (i32.store offset=32768
    (local.get $0)
    (i32.add
     (i32.load offset=32768
      (local.get $0)
     )
     (i32.const 1)
    )
   )
  )
 )
 (func $shine::graphics::CommandBuffer::getData\28\29 (param $0 i32) (result i32)
  (local.get $0)
 )
 (func $shine::graphics::CommandBuffer::getCount\28\29\20const (param $0 i32) (result i32)
  (i32.load offset=32768
   (local.get $0)
  )
 )
 (func $__cxx_global_array_dtor (param $0 i32)
  (drop
   (call $shine::graphics::Renderer2D::~Renderer2D\28\29
    (i32.const 37452)
   )
  )
 )
 (func $shine::graphics::Renderer2D::~Renderer2D\28\29 (param $0 i32) (result i32)
  (drop
   (call $shine::wasm::SVector<shine::graphics::Renderer2D::Batch>::~SVector\28\29
    (i32.add
     (local.get $0)
     (i32.const 104)
    )
   )
  )
  (drop
   (call $shine::wasm::SVector<float>::~SVector\28\29
    (i32.add
     (local.get $0)
     (i32.const 92)
    )
   )
  )
  (local.get $0)
 )
 (func $shine::wasm::SVector<shine::graphics::Renderer2D::Batch>::~SVector\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $1
      (i32.load offset=8
       (local.get $0)
      )
     )
    )
   )
   (call $free
    (local.get $1)
   )
   (i32.store offset=8
    (local.get $0)
    (i32.const 0)
   )
  )
  (i64.store align=4
   (local.get $0)
   (i64.const 0)
  )
  (local.get $0)
 )
 (func $shine::wasm::SVector<float>::~SVector\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $1
      (i32.load offset=8
       (local.get $0)
      )
     )
    )
   )
   (call $free
    (local.get $1)
   )
   (i32.store offset=8
    (local.get $0)
    (i32.const 0)
   )
  )
  (i64.store align=4
   (local.get $0)
   (i64.const 0)
  )
  (local.get $0)
 )
 (func $shine::graphics::Renderer2D::instance\28\29 (result i32)
  (i32.const 37452)
 )
 (func $shine::graphics::Renderer2D::init\28int\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (local $8 i32)
  (local $9 i32)
  (local $10 i32)
  (global.set $__stack_pointer
   (local.tee $2
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store
   (local.get $0)
   (local.get $1)
  )
  (i32.store offset=4
   (local.get $0)
   (call $gl_create_buffer
    (local.get $1)
   )
  )
  (i32.store offset=8
   (local.get $0)
   (local.tee $3
    (call $gl_create_vertex_array
     (local.get $1)
    )
   )
  )
  (call $gl_bind_vertex_array
   (local.get $1)
   (local.get $3)
  )
  (call $gl_bind_buffer
   (local.get $1)
   (i32.const 34962)
   (i32.load offset=4
    (local.get $0)
   )
  )
  (call $gl_enable_attribs
   (local.get $1)
  )
  (local.set $4
   (i32.const 0)
  )
  (call $gl_bind_vertex_array
   (local.get $1)
   (i32.const 0)
  )
  (call $shine::wasm::SVector<float>::reserve\28unsigned\20int\29
   (i32.add
    (local.get $0)
    (i32.const 92)
   )
   (i32.const 65536)
  )
  (i32.store offset=12
   (local.get $0)
   (call $gl_create_program_from_source\28int\2c\20char\20const*\2c\20char\20const*\29
    (local.get $1)
    (i32.const 1024)
    (i32.const 1232)
   )
  )
  (i32.store offset=24
   (local.get $0)
   (call $gl_create_program_from_source\28int\2c\20char\20const*\2c\20char\20const*\29
    (local.get $1)
    (i32.const 1376)
    (i32.const 1584)
   )
  )
  (i32.store offset=32
   (local.get $0)
   (local.tee $3
    (call $gl_create_program_from_source\28int\2c\20char\20const*\2c\20char\20const*\29
     (local.get $1)
     (i32.const 1024)
     (i32.const 1696)
    )
   )
  )
  (i32.store offset=12
   (local.get $2)
   (local.get $3)
  )
  (i32.store offset=4
   (local.get $2)
   (i32.load offset=12
    (local.get $0)
   )
  )
  (i32.store offset=8
   (local.get $2)
   (i32.load offset=24
    (local.get $0)
   )
  )
  (block $label$1
   (loop $label$2
    (br_if $label$1
     (i32.eq
      (local.get $4)
      (i32.const 120)
     )
    )
    (local.set $5
     (i32.add
      (local.get $0)
      (i32.load16_u offset=2864
       (local.get $4)
      )
     )
    )
    (local.set $6
     (i32.load
      (i32.add
       (i32.add
        (local.get $2)
        (i32.const 4)
       )
       (i32.shl
        (i32.load8_u offset=2866
         (local.get $4)
        )
        (i32.const 2)
       )
      )
     )
    )
    (local.set $8
     (call $shine::wasm::ptr_i32\28void\20const*\29
      (local.tee $7
       (i32.load offset=2868
        (local.get $4)
       )
      )
     )
    )
    (block $label$3
     (block $label$4
      (br_if $label$4
       (local.get $7)
      )
      (local.set $3
       (i32.const 0)
      )
      (br $label$3)
     )
     (local.set $3
      (i32.const 0)
     )
     (loop $label$5
      (local.set $9
       (i32.add
        (local.get $7)
        (local.get $3)
       )
      )
      (local.set $3
       (local.tee $10
        (i32.add
         (local.get $3)
         (i32.const 1)
        )
       )
      )
      (br_if $label$5
       (i32.load8_u
        (local.get $9)
       )
      )
     )
     (local.set $3
      (i32.add
       (local.get $10)
       (i32.const -1)
      )
     )
    )
    (i32.store
     (local.get $5)
     (call $gl_get_uniform_location
      (local.get $1)
      (local.get $6)
      (local.get $8)
      (local.get $3)
     )
    )
    (local.set $4
     (i32.add
      (local.get $4)
      (i32.const 8)
     )
    )
    (br $label$2)
   )
  )
  (call $gl_bind_buffer
   (local.get $1)
   (i32.const 34962)
   (i32.load offset=4
    (local.get $0)
   )
  )
  (call $gl_buffer_data_f32
   (local.get $1)
   (i32.const 34962)
   (i32.const 0)
   (i32.const 262144)
   (i32.const 35048)
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $2)
    (i32.const 16)
   )
  )
 )
 (func $shine::wasm::SVector<float>::reserve\28unsigned\20int\29 (param $0 i32) (param $1 i32)
  (call $shine::wasm::svector_reserve_impl\28void**\2c\20unsigned\20int*\2c\20unsigned\20int\2c\20unsigned\20int\2c\20unsigned\20int\29
   (i32.add
    (local.get $0)
    (i32.const 8)
   )
   (i32.add
    (local.get $0)
    (i32.const 4)
   )
   (i32.load
    (local.get $0)
   )
   (local.get $1)
   (i32.const 4)
  )
 )
 (func $shine::wasm::ptr_i32\28void\20const*\29 (param $0 i32) (result i32)
  (local.get $0)
 )
 (func $shine::graphics::Renderer2D::begin\28\29 (param $0 i32)
  (i32.store offset=104
   (local.get $0)
   (i32.const 0)
  )
  (i32.store offset=92
   (local.get $0)
   (i32.const 0)
  )
  (call $shine::wasm::SVector<float>::reserve\28unsigned\20int\29
   (i32.add
    (local.get $0)
    (i32.const 92)
   )
   (i32.const 65536)
  )
 )
 (func $shine::graphics::Renderer2D::checkBatch\28int\2c\20int\2c\20int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32)
  (local $5 i32)
  (local $6 i32)
  (global.set $__stack_pointer
   (local.tee $5
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 112)
    )
   )
  )
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (local.tee $6
       (i32.load offset=104
        (local.get $0)
       )
      )
     )
    )
    (br_if $label$2
     (i32.ne
      (i32.load
       (i32.add
        (local.tee $6
         (i32.add
          (i32.load offset=112
           (local.get $0)
          )
          (i32.mul
           (local.get $6)
           (i32.const 112)
          )
         )
        )
        (i32.const -100)
       )
      )
      (local.get $1)
     )
    )
    (br_if $label$2
     (i32.ne
      (i32.load
       (i32.add
        (local.get $6)
        (i32.const -112)
       )
      )
      (local.get $2)
     )
    )
    (i32.store
     (local.tee $6
      (i32.add
       (local.get $6)
       (i32.const -104)
      )
     )
     (i32.add
      (i32.load
       (local.get $6)
      )
      (local.get $4)
     )
    )
    (br $label$1)
   )
   (i32.store offset=8
    (local.tee $6
     (call $shine::graphics::Renderer2D::Batch::Batch\28\29
      (local.get $5)
     )
    )
    (local.get $4)
   )
   (i32.store offset=4
    (local.get $6)
    (local.get $3)
   )
   (i32.store
    (local.get $6)
    (local.get $2)
   )
   (i32.store offset=12
    (local.get $6)
    (local.get $1)
   )
   (call $shine::wasm::SVector<shine::graphics::Renderer2D::Batch>::push_back\28shine::graphics::Renderer2D::Batch\20const&\29
    (i32.add
     (local.get $0)
     (i32.const 104)
    )
    (local.get $6)
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $5)
    (i32.const 112)
   )
  )
 )
 (func $shine::graphics::Renderer2D::Batch::Batch\28\29 (param $0 i32) (result i32)
  (i64.store offset=8 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store align=4
   (local.get $0)
   (i64.const 0)
  )
  (drop
   (call $shine::graphics::Renderer2D::RRUniformState::RRUniformState\28\29
    (i32.add
     (local.get $0)
     (i32.const 16)
    )
   )
  )
  (local.get $0)
 )
 (func $shine::wasm::SVector<shine::graphics::Renderer2D::Batch>::push_back\28shine::graphics::Renderer2D::Batch\20const&\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.le_u
     (local.tee $3
      (i32.add
       (local.tee $2
        (i32.load
         (local.get $0)
        )
       )
       (i32.const 1)
      )
     )
     (local.tee $4
      (i32.load offset=4
       (local.get $0)
      )
     )
    )
   )
   (call $shine::wasm::SVector<shine::graphics::Renderer2D::Batch>::reserve\28unsigned\20int\29
    (local.get $0)
    (call $shine::wasm::SVector<shine::graphics::Renderer2D::Batch>::grow_cap\28unsigned\20int\2c\20unsigned\20int\29
     (local.get $4)
     (local.get $3)
    )
   )
   (local.set $2
    (i32.load
     (local.get $0)
    )
   )
  )
  (block $label$2
   (br_if $label$2
    (i32.eqz
     (i32.const 112)
    )
   )
   (memory.copy
    (i32.add
     (i32.load offset=8
      (local.get $0)
     )
     (i32.mul
      (local.get $2)
      (i32.const 112)
     )
    )
    (local.get $1)
    (i32.const 112)
   )
  )
  (i32.store
   (local.get $0)
   (local.get $3)
  )
 )
 (func $shine::graphics::Renderer2D::RRUniformState::RRUniformState\28\29 (param $0 i32) (result i32)
  (i64.store offset=88 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=80 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=72 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=64 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=56 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=48 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=40 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=32 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=24 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=16 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=8 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store align=4
   (local.get $0)
   (i64.const 0)
  )
  (local.get $0)
 )
 (func $shine::wasm::SVector<shine::graphics::Renderer2D::Batch>::grow_cap\28unsigned\20int\2c\20unsigned\20int\29 (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local.set $0
   (select
    (local.get $0)
    (i32.const 8)
    (local.get $0)
   )
  )
  (loop $label$1
   (local.set $0
    (i32.shl
     (local.tee $2
      (local.get $0)
     )
     (i32.const 1)
    )
   )
   (br_if $label$1
    (i32.lt_u
     (local.get $2)
     (local.get $1)
    )
   )
  )
  (local.get $2)
 )
 (func $shine::wasm::SVector<shine::graphics::Renderer2D::Batch>::reserve\28unsigned\20int\29 (param $0 i32) (param $1 i32)
  (call $shine::wasm::svector_reserve_impl\28void**\2c\20unsigned\20int*\2c\20unsigned\20int\2c\20unsigned\20int\2c\20unsigned\20int\29
   (i32.add
    (local.get $0)
    (i32.const 8)
   )
   (i32.add
    (local.get $0)
    (i32.const 4)
   )
   (i32.load
    (local.get $0)
   )
   (local.get $1)
   (i32.const 112)
  )
 )
 (func $shine::graphics::Renderer2D::end\28\29 (param $0 i32)
  (call $shine::graphics::Renderer2D::flush\28\29
   (local.get $0)
  )
 )
 (func $shine::graphics::Renderer2D::flush\28\29 (param $0 i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (local $8 i32)
  (local $9 i32)
  (local $10 i32)
  (local $11 i32)
  (global.set $__stack_pointer
   (local.tee $1
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 112)
    )
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.load offset=92
      (local.get $0)
     )
    )
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 5)
    (i32.const 34962)
    (i32.load offset=4
     (local.get $0)
    )
    (i32.const 0)
    (i32.const 0)
    (i32.const 0)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 6)
    (i32.const 34962)
    (call $shine::wasm::ptr_i32\28void\20const*\29
     (i32.load offset=100
      (local.get $0)
     )
    )
    (i32.load offset=92
     (local.get $0)
    )
    (i32.const 35048)
    (i32.const 0)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 16)
    (i32.load offset=8
     (local.get $0)
    )
    (i32.const 0)
    (i32.const 0)
    (i32.const 0)
    (i32.const 0)
   )
   (i64.store offset=104 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=96 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=88 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=80 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=72 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=64 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=56 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=48 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=40 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=32 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=24 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i64.store offset=16 align=4
    (local.get $1)
    (i64.const 0)
   )
   (i32.store8 offset=15
    (local.get $1)
    (i32.const 0)
   )
   (local.set $2
    (call $shine::wasm::f2i\28float\29
     (f32.convert_i32_s
      (i32.load offset=84
       (local.get $0)
      )
     )
    )
   )
   (local.set $3
    (i32.const -1)
   )
   (local.set $4
    (call $shine::wasm::f2i\28float\29
     (f32.convert_i32_s
      (i32.load offset=88
       (local.get $0)
      )
     )
    )
   )
   (local.set $5
    (i32.const 0)
   )
   (local.set $6
    (i32.const 0)
   )
   (local.set $7
    (i32.const 0)
   )
   (local.set $8
    (i32.const -1)
   )
   (loop $label$2
    (block $label$3
     (br_if $label$3
      (i32.lt_u
       (local.get $7)
       (i32.load offset=104
        (local.get $0)
       )
      )
     )
     (i32.store offset=104
      (local.get $0)
      (i32.const 0)
     )
     (br $label$1)
    )
    (block $label$4
     (br_if $label$4
      (i32.eqz
       (i32.load
        (local.tee $10
         (i32.add
          (local.tee $9
           (i32.add
            (i32.load offset=112
             (local.get $0)
            )
            (local.get $5)
           )
          )
          (i32.const 8)
         )
        )
       )
      )
     )
     (block $label$5
      (block $label$6
       (block $label$7
        (br_if $label$7
         (i32.ne
          (local.tee $11
           (i32.load
            (i32.add
             (local.get $9)
             (i32.const 12)
            )
           )
          )
          (local.get $8)
         )
        )
        (local.set $11
         (local.get $8)
        )
        (br $label$6)
       )
       (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
        (i32.const 4)
        (i32.load
         (i32.add
          (local.get $0)
          (select
           (select
            (i32.const 24)
            (i32.const 32)
            (i32.eq
             (local.get $11)
             (i32.const 2)
            )
           )
           (i32.const 12)
           (local.get $11)
          )
         )
        )
        (i32.const 0)
        (i32.const 0)
        (i32.const 0)
        (i32.const 0)
       )
       (block $label$8
        (br_if $label$8
         (local.get $11)
        )
        (local.set $8
         (i32.const 0)
        )
        (br_if $label$5
         (i32.and
          (local.get $6)
          (i32.const 1)
         )
        )
        (local.set $8
         (i32.const 0)
        )
        (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
         (i32.const 17)
         (i32.load offset=20
          (local.get $0)
         )
         (local.get $2)
         (local.get $4)
         (i32.load offset=16
          (local.get $0)
         )
         (i32.const 0)
        )
        (local.set $6
         (i32.or
          (local.get $6)
          (i32.const 1)
         )
        )
        (br $label$5)
       )
       (block $label$9
        (br_if $label$9
         (i32.ne
          (local.get $11)
          (i32.const 2)
         )
        )
        (local.set $8
         (i32.const 2)
        )
        (br_if $label$5
         (i32.and
          (local.get $6)
          (i32.const 4)
         )
        )
        (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
         (i32.const 13)
         (i32.load offset=28
          (local.get $0)
         )
         (local.get $2)
         (local.get $4)
         (i32.const 0)
         (i32.const 0)
        )
        (local.set $6
         (i32.or
          (local.get $6)
          (i32.const 4)
         )
        )
        (br $label$5)
       )
       (br_if $label$6
        (i32.and
         (local.get $6)
         (i32.const 2)
        )
       )
       (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
        (i32.const 13)
        (i32.load offset=36
         (local.get $0)
        )
        (local.get $2)
        (local.get $4)
        (i32.const 0)
        (i32.const 0)
       )
       (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
        (i32.const 11)
        (i32.load offset=40
         (local.get $0)
        )
        (i32.const 0)
        (i32.const 0)
        (i32.const 0)
        (i32.const 0)
       )
       (local.set $6
        (i32.or
         (local.get $6)
         (i32.const 2)
        )
       )
      )
      (block $label$10
       (br_if $label$10
        (i32.eq
         (local.get $11)
         (i32.const 1)
        )
       )
       (local.set $8
        (local.get $11)
       )
       (br $label$5)
      )
      (call $shine::graphics::Renderer2D::updateRRUniforms\28shine::graphics::Renderer2D::RRUniformState\20const&\2c\20shine::graphics::Renderer2D::RRUniformState&\2c\20bool&\29
       (local.get $0)
       (i32.add
        (local.get $9)
        (i32.const 16)
       )
       (i32.add
        (local.get $1)
        (i32.const 16)
       )
       (i32.add
        (local.get $1)
        (i32.const 15)
       )
      )
      (local.set $8
       (i32.const 1)
      )
     )
     (block $label$11
      (br_if $label$11
       (i32.eq
        (local.tee $11
         (i32.load
          (local.get $9)
         )
        )
        (local.get $3)
       )
      )
      (block $label$12
       (br_if $label$12
        (i32.eq
         (local.get $8)
         (i32.const 2)
        )
       )
       (block $label$13
        (br_if $label$13
         (local.get $11)
        )
        (local.set $3
         (i32.const 0)
        )
        (br $label$11)
       )
       (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
        (i32.const 10)
        (i32.const 3553)
        (local.get $11)
        (i32.const 0)
        (i32.const 0)
        (i32.const 0)
       )
      )
      (local.set $3
       (local.get $11)
      )
     )
     (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
      (i32.const 8)
      (i32.const 4)
      (i32.load
       (i32.add
        (local.get $9)
        (i32.const 4)
       )
      )
      (i32.load
       (local.get $10)
      )
      (i32.const 0)
      (i32.const 0)
     )
    )
    (local.set $5
     (i32.add
      (local.get $5)
      (i32.const 112)
     )
    )
    (local.set $7
     (i32.add
      (local.get $7)
      (i32.const 1)
     )
    )
    (br $label$2)
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $1)
    (i32.const 112)
   )
  )
 )
 (func $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (param $5 i32)
  (call $shine::graphics::CommandBuffer::push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
   (call $shine::graphics::CommandBuffer::instance\28\29)
   (local.get $0)
   (local.get $1)
   (local.get $2)
   (local.get $3)
   (local.get $4)
   (local.get $5)
   (i32.const 0)
   (i32.const 0)
  )
 )
 (func $shine::wasm::f2i\28float\29 (param $0 f32) (result i32)
  (i32.reinterpret_f32
   (local.get $0)
  )
 )
 (func $shine::graphics::Renderer2D::updateRRUniforms\28shine::graphics::Renderer2D::RRUniformState\20const&\2c\20shine::graphics::Renderer2D::RRUniformState&\2c\20bool&\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32)
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.ne
      (i32.load8_u
       (local.get $3)
      )
      (i32.const 1)
     )
    )
    (br_if $label$1
     (i32.eqz
      (call $memcmp
       (local.get $1)
       (local.get $2)
       (i32.const 96)
      )
     )
    )
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 13)
    (i32.load offset=52
     (local.get $0)
    )
    (i32.load offset=4
     (local.get $1)
    )
    (i32.load offset=8
     (local.get $1)
    )
    (i32.const 0)
    (i32.const 0)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 11)
    (i32.load offset=44
     (local.get $0)
    )
    (i32.load
     (local.get $1)
    )
    (i32.const 0)
    (i32.const 0)
    (i32.const 0)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 14)
    (i32.load offset=48
     (local.get $0)
    )
    (i32.load offset=12
     (local.get $1)
    )
    (i32.load offset=16
     (local.get $1)
    )
    (i32.load offset=20
     (local.get $1)
    )
    (i32.load offset=24
     (local.get $1)
    )
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 14)
    (i32.load offset=56
     (local.get $0)
    )
    (i32.load offset=28
     (local.get $1)
    )
    (i32.load offset=32
     (local.get $1)
    )
    (i32.load offset=36
     (local.get $1)
    )
    (i32.load offset=40
     (local.get $1)
    )
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 14)
    (i32.load offset=60
     (local.get $0)
    )
    (i32.load offset=44
     (local.get $1)
    )
    (i32.load offset=48
     (local.get $1)
    )
    (i32.load offset=52
     (local.get $1)
    )
    (i32.load offset=56
     (local.get $1)
    )
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 12)
    (i32.load offset=64
     (local.get $0)
    )
    (i32.load offset=60
     (local.get $1)
    )
    (i32.const 0)
    (i32.const 0)
    (i32.const 0)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 14)
    (i32.load offset=68
     (local.get $0)
    )
    (i32.load offset=64
     (local.get $1)
    )
    (i32.load offset=68
     (local.get $1)
    )
    (i32.load offset=72
     (local.get $1)
    )
    (i32.load offset=76
     (local.get $1)
    )
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 13)
    (i32.load offset=72
     (local.get $0)
    )
    (i32.load offset=80
     (local.get $1)
    )
    (i32.load offset=84
     (local.get $1)
    )
    (i32.const 0)
    (i32.const 0)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 12)
    (i32.load offset=76
     (local.get $0)
    )
    (i32.load offset=88
     (local.get $1)
    )
    (i32.const 0)
    (i32.const 0)
    (i32.const 0)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
    (i32.const 12)
    (i32.load offset=80
     (local.get $0)
    )
    (i32.load offset=92
     (local.get $1)
    )
    (i32.const 0)
    (i32.const 0)
    (i32.const 0)
   )
   (block $label$3
    (br_if $label$3
     (i32.eqz
      (i32.const 96)
     )
    )
    (memory.copy
     (local.get $2)
     (local.get $1)
     (i32.const 96)
    )
   )
   (i32.store8
    (local.get $3)
    (i32.const 1)
   )
  )
 )
 (func $shine::graphics::Renderer2D::allocVtx\28int\2c\20int*\29 (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local.set $3
   (i32.load offset=92
    (local.get $0)
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $2)
    )
   )
   (i32.store
    (local.get $2)
    (i32.div_u
     (local.get $3)
     (i32.const 5)
    )
   )
  )
  (local.set $4
   (i32.add
    (local.get $0)
    (i32.const 92)
   )
  )
  (block $label$2
   (br_if $label$2
    (i32.le_u
     (local.tee $5
      (i32.add
       (local.get $3)
       (local.get $1)
      )
     )
     (local.tee $2
      (i32.load offset=96
       (local.get $0)
      )
     )
    )
   )
   (local.set $2
    (select
     (local.get $2)
     (i32.const 256)
     (local.get $2)
    )
   )
   (loop $label$3
    (local.set $2
     (i32.shl
      (local.tee $1
       (local.get $2)
      )
      (i32.const 1)
     )
    )
    (br_if $label$3
     (i32.lt_u
      (local.get $1)
      (local.get $5)
     )
    )
   )
   (call $shine::wasm::SVector<float>::reserve\28unsigned\20int\29
    (local.get $4)
    (local.get $1)
   )
  )
  (call $shine::wasm::SVector<float>::resize_uninitialized\28unsigned\20int\29
   (local.get $4)
   (local.get $5)
  )
  (i32.add
   (i32.load offset=100
    (local.get $0)
   )
   (i32.shl
    (local.get $3)
    (i32.const 2)
   )
  )
 )
 (func $shine::wasm::SVector<float>::resize_uninitialized\28unsigned\20int\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (block $label$1
   (br_if $label$1
    (i32.le_u
     (local.get $1)
     (i32.load
      (local.get $0)
     )
    )
   )
   (br_if $label$1
    (i32.le_u
     (local.get $1)
     (local.tee $2
      (i32.load offset=4
       (local.get $0)
      )
     )
    )
   )
   (call $shine::wasm::SVector<float>::reserve\28unsigned\20int\29
    (local.get $0)
    (call $shine::wasm::SVector<float>::grow_cap\28unsigned\20int\2c\20unsigned\20int\29
     (local.get $2)
     (local.get $1)
    )
   )
  )
  (i32.store
   (local.get $0)
   (local.get $1)
  )
 )
 (func $shine::wasm::SVector<float>::grow_cap\28unsigned\20int\2c\20unsigned\20int\29 (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local.set $0
   (select
    (local.get $0)
    (i32.const 8)
    (local.get $0)
   )
  )
  (loop $label$1
   (local.set $0
    (i32.shl
     (local.tee $2
      (local.get $0)
     )
     (i32.const 1)
    )
   )
   (br_if $label$1
    (i32.lt_u
     (local.get $2)
     (local.get $1)
    )
   )
  )
  (local.get $2)
 )
 (func $shine::graphics::Renderer2D::drawRectColor\28float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 f32) (param $4 f32) (param $5 f32) (param $6 f32) (param $7 f32)
  (local $8 i32)
  (local $9 i32)
  (local $10 f32)
  (local $11 f32)
  (global.set $__stack_pointer
   (local.tee $8
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=12
   (local.get $8)
   (i32.const 0)
  )
  (local.set $9
   (call $shine::graphics::Renderer2D::allocVtx\28int\2c\20int*\29
    (local.get $0)
    (i32.const 30)
    (i32.add
     (local.get $8)
     (i32.const 12)
    )
   )
  )
  (call $shine::graphics::Renderer2D::checkBatch\28int\2c\20int\2c\20int\2c\20int\29
   (local.get $0)
   (i32.const 2)
   (i32.const 0)
   (i32.load offset=12
    (local.get $8)
   )
   (i32.const 6)
  )
  (f32.store offset=116
   (local.get $9)
   (local.get $7)
  )
  (f32.store offset=112
   (local.get $9)
   (local.get $6)
  )
  (f32.store offset=108
   (local.get $9)
   (local.get $5)
  )
  (f32.store offset=104
   (local.get $9)
   (local.tee $4
    (f32.add
     (local.tee $10
      (f32.mul
       (local.get $4)
       (f32.const 0.5)
      )
     )
     (local.get $2)
    )
   )
  )
  (f32.store offset=100
   (local.get $9)
   (local.tee $3
    (f32.add
     (local.tee $11
      (f32.mul
       (local.get $3)
       (f32.const 0.5)
      )
     )
     (local.get $1)
    )
   )
  )
  (f32.store offset=96
   (local.get $9)
   (local.get $7)
  )
  (f32.store offset=92
   (local.get $9)
   (local.get $6)
  )
  (f32.store offset=88
   (local.get $9)
   (local.get $5)
  )
  (f32.store offset=84
   (local.get $9)
   (local.tee $2
    (f32.sub
     (local.get $2)
     (local.get $10)
    )
   )
  )
  (f32.store offset=80
   (local.get $9)
   (local.get $3)
  )
  (f32.store offset=76
   (local.get $9)
   (local.get $7)
  )
  (f32.store offset=72
   (local.get $9)
   (local.get $6)
  )
  (f32.store offset=68
   (local.get $9)
   (local.get $5)
  )
  (f32.store offset=64
   (local.get $9)
   (local.get $4)
  )
  (f32.store offset=60
   (local.get $9)
   (local.tee $1
    (f32.sub
     (local.get $1)
     (local.get $11)
    )
   )
  )
  (f32.store offset=56
   (local.get $9)
   (local.get $7)
  )
  (f32.store offset=52
   (local.get $9)
   (local.get $6)
  )
  (f32.store offset=48
   (local.get $9)
   (local.get $5)
  )
  (f32.store offset=44
   (local.get $9)
   (local.get $4)
  )
  (f32.store offset=40
   (local.get $9)
   (local.get $1)
  )
  (f32.store offset=36
   (local.get $9)
   (local.get $7)
  )
  (f32.store offset=32
   (local.get $9)
   (local.get $6)
  )
  (f32.store offset=28
   (local.get $9)
   (local.get $5)
  )
  (f32.store offset=24
   (local.get $9)
   (local.get $2)
  )
  (f32.store offset=20
   (local.get $9)
   (local.get $3)
  )
  (f32.store offset=16
   (local.get $9)
   (local.get $7)
  )
  (f32.store offset=12
   (local.get $9)
   (local.get $6)
  )
  (f32.store offset=8
   (local.get $9)
   (local.get $5)
  )
  (f32.store offset=4
   (local.get $9)
   (local.get $2)
  )
  (f32.store
   (local.get $9)
   (local.get $1)
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $8)
    (i32.const 16)
   )
  )
 )
 (func $shine::graphics::Renderer2D::drawRectUV\28int\2c\20float\2c\20float\2c\20float\2c\20float\29 (param $0 i32) (param $1 i32) (param $2 f32) (param $3 f32) (param $4 f32) (param $5 f32)
  (local $6 i32)
  (local $7 i32)
  (local $8 f32)
  (local $9 f32)
  (global.set $__stack_pointer
   (local.tee $6
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=12
   (local.get $6)
   (i32.const 0)
  )
  (local.set $7
   (call $shine::graphics::Renderer2D::allocVtx\28int\2c\20int*\29
    (local.get $0)
    (i32.const 30)
    (i32.add
     (local.get $6)
     (i32.const 12)
    )
   )
  )
  (call $shine::graphics::Renderer2D::checkBatch\28int\2c\20int\2c\20int\2c\20int\29
   (local.get $0)
   (i32.const 0)
   (local.get $1)
   (i32.load offset=12
    (local.get $6)
   )
   (i32.const 6)
  )
  (i32.store offset=116
   (local.get $7)
   (i32.const 0)
  )
  (i64.store offset=108 align=4
   (local.get $7)
   (i64.const 4575657222473777152)
  )
  (f32.store offset=104
   (local.get $7)
   (local.tee $5
    (f32.add
     (local.tee $8
      (f32.mul
       (local.get $5)
       (f32.const 0.5)
      )
     )
     (local.get $3)
    )
   )
  )
  (f32.store offset=100
   (local.get $7)
   (local.tee $4
    (f32.add
     (local.tee $9
      (f32.mul
       (local.get $4)
       (f32.const 0.5)
      )
     )
     (local.get $2)
    )
   )
  )
  (i32.store offset=96
   (local.get $7)
   (i32.const 0)
  )
  (i64.store offset=88 align=4
   (local.get $7)
   (i64.const 1065353216)
  )
  (f32.store offset=84
   (local.get $7)
   (local.tee $3
    (f32.sub
     (local.get $3)
     (local.get $8)
    )
   )
  )
  (f32.store offset=80
   (local.get $7)
   (local.get $4)
  )
  (i32.store offset=76
   (local.get $7)
   (i32.const 0)
  )
  (i64.store offset=68 align=4
   (local.get $7)
   (i64.const 4575657221408423936)
  )
  (f32.store offset=64
   (local.get $7)
   (local.get $5)
  )
  (f32.store offset=60
   (local.get $7)
   (local.tee $2
    (f32.sub
     (local.get $2)
     (local.get $9)
    )
   )
  )
  (i32.store offset=56
   (local.get $7)
   (i32.const 0)
  )
  (i64.store offset=48 align=4
   (local.get $7)
   (i64.const 4575657221408423936)
  )
  (f32.store offset=44
   (local.get $7)
   (local.get $5)
  )
  (f32.store offset=40
   (local.get $7)
   (local.get $2)
  )
  (i32.store offset=36
   (local.get $7)
   (i32.const 0)
  )
  (i64.store offset=28 align=4
   (local.get $7)
   (i64.const 1065353216)
  )
  (f32.store offset=24
   (local.get $7)
   (local.get $3)
  )
  (f32.store offset=20
   (local.get $7)
   (local.get $4)
  )
  (i32.store offset=16
   (local.get $7)
   (i32.const 0)
  )
  (i64.store offset=8 align=4
   (local.get $7)
   (i64.const 0)
  )
  (f32.store offset=4
   (local.get $7)
   (local.get $3)
  )
  (f32.store
   (local.get $7)
   (local.get $2)
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $6)
    (i32.const 16)
   )
  )
 )
 (func $shine::graphics::Renderer2D::drawRoundRect\28float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20shine::graphics::Renderer2D::Color4\20const&\2c\20int\2c\20shine::graphics::Renderer2D::Color4\20const&\2c\20float\2c\20shine::graphics::Renderer2D::Color4\20const&\2c\20float\2c\20float\2c\20float\2c\20float\2c\20shine::graphics::Renderer2D::Color4\20const&\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 f32) (param $4 f32) (param $5 f32) (param $6 i32) (param $7 i32) (param $8 i32) (param $9 f32) (param $10 i32) (param $11 f32) (param $12 f32) (param $13 f32) (param $14 f32) (param $15 i32)
  (local $16 i32)
  (local $17 i32)
  (local $18 f32)
  (local $19 f32)
  (global.set $__stack_pointer
   (local.tee $16
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 224)
    )
   )
  )
  (i32.store offset=220
   (local.get $16)
   (i32.const 0)
  )
  (local.set $17
   (call $shine::graphics::Renderer2D::allocVtx\28int\2c\20int*\29
    (local.get $0)
    (i32.const 30)
    (i32.add
     (local.get $16)
     (i32.const 220)
    )
   )
  )
  (i32.store offset=124
   (local.get $16)
   (i32.ne
    (local.get $7)
    (i32.const 0)
   )
  )
  (i32.store offset=128
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.mul
     (local.get $5)
     (local.tee $18
      (select
       (f32.div
        (f32.const 1)
        (local.get $3)
       )
       (f32.const 0)
       (f32.gt
        (local.get $3)
        (f32.const 0.10000000149011612)
       )
      )
     )
    )
   )
  )
  (i32.store offset=132
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.mul
     (local.get $5)
     (local.tee $19
      (select
       (f32.div
        (f32.const 1)
        (local.get $4)
       )
       (f32.const 0)
       (f32.gt
        (local.get $4)
        (f32.const 0.10000000149011612)
       )
      )
     )
    )
   )
  )
  (i32.store offset=136
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load
     (local.get $6)
    )
   )
  )
  (i32.store offset=140
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=4
     (local.get $6)
    )
   )
  )
  (i32.store offset=144
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=8
     (local.get $6)
    )
   )
  )
  (i32.store offset=148
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=12
     (local.get $6)
    )
   )
  )
  (i32.store offset=152
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load
     (local.get $8)
    )
   )
  )
  (i32.store offset=156
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=4
     (local.get $8)
    )
   )
  )
  (i32.store offset=160
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=8
     (local.get $8)
    )
   )
  )
  (i32.store offset=164
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=12
     (local.get $8)
    )
   )
  )
  (i32.store offset=168
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load
     (local.get $10)
    )
   )
  )
  (i32.store offset=172
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=4
     (local.get $10)
    )
   )
  )
  (i32.store offset=176
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=8
     (local.get $10)
    )
   )
  )
  (i32.store offset=180
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=12
     (local.get $10)
    )
   )
  )
  (i32.store offset=184
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.mul
     (local.tee $5
      (select
       (f32.div
        (f32.const 2)
        (local.tee $5
         (f32.add
          (local.get $3)
          (local.get $4)
         )
        )
       )
       (f32.const 0)
       (f32.gt
        (local.get $5)
        (f32.const 0.10000000149011612)
       )
      )
     )
     (local.get $9)
    )
   )
  )
  (i32.store offset=188
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load
     (local.get $15)
    )
   )
  )
  (i32.store offset=192
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=4
     (local.get $15)
    )
   )
  )
  (i32.store offset=196
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=8
     (local.get $15)
    )
   )
  )
  (i32.store offset=200
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.load offset=12
     (local.get $15)
    )
   )
  )
  (i32.store offset=204
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.mul
     (local.get $18)
     (local.get $11)
    )
   )
  )
  (i32.store offset=208
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.mul
     (local.get $19)
     (local.get $12)
    )
   )
  )
  (i32.store offset=212
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.mul
     (local.get $5)
     (local.get $13)
    )
   )
  )
  (i32.store offset=216
   (local.get $16)
   (call $shine::wasm::f2i\28float\29
    (f32.mul
     (local.get $5)
     (local.get $14)
    )
   )
  )
  (local.set $8
   (i32.add
    (local.get $0)
    (i32.const 104)
   )
  )
  (block $label$1
   (block $label$2
    (br_if $label$2
     (local.tee $6
      (i32.load offset=104
       (local.get $0)
      )
     )
    )
    (i32.store
     (local.tee $6
      (call $shine::graphics::Renderer2D::Batch::Batch\28\29
       (i32.add
        (local.get $16)
        (i32.const 12)
       )
      )
     )
     (local.get $7)
    )
    (i64.store offset=8 align=4
     (local.get $6)
     (i64.const 4294967296)
    )
    (i32.store offset=4
     (local.get $6)
     (i32.load offset=220
      (local.get $16)
     )
    )
    (block $label$3
     (br_if $label$3
      (i32.eqz
       (i32.const 96)
      )
     )
     (memory.copy
      (i32.add
       (local.get $6)
       (i32.const 16)
      )
      (i32.add
       (local.get $16)
       (i32.const 124)
      )
      (i32.const 96)
     )
    )
    (call $shine::wasm::SVector<shine::graphics::Renderer2D::Batch>::push_back\28shine::graphics::Renderer2D::Batch\20const&\29
     (local.get $8)
     (local.get $6)
    )
    (br $label$1)
   )
   (block $label$4
    (br_if $label$4
     (i32.ne
      (i32.load
       (i32.add
        (local.tee $6
         (i32.add
          (i32.load offset=112
           (local.get $0)
          )
          (i32.mul
           (local.get $6)
           (i32.const 112)
          )
         )
        )
        (i32.const -100)
       )
      )
      (i32.const 1)
     )
    )
    (br_if $label$4
     (i32.ne
      (i32.load
       (i32.add
        (local.get $6)
        (i32.const -112)
       )
      )
      (local.get $7)
     )
    )
    (br_if $label$1
     (i32.eqz
      (call $memcmp
       (i32.add
        (local.get $6)
        (i32.const -96)
       )
       (i32.add
        (local.get $16)
        (i32.const 124)
       )
       (i32.const 96)
      )
     )
    )
   )
   (i32.store
    (local.tee $6
     (call $shine::graphics::Renderer2D::Batch::Batch\28\29
      (i32.add
       (local.get $16)
       (i32.const 12)
      )
     )
    )
    (local.get $7)
   )
   (i64.store offset=8 align=4
    (local.get $6)
    (i64.const 4294967296)
   )
   (i32.store offset=4
    (local.get $6)
    (i32.load offset=220
     (local.get $16)
    )
   )
   (block $label$5
    (br_if $label$5
     (i32.eqz
      (i32.const 96)
     )
    )
    (memory.copy
     (i32.add
      (local.get $6)
      (i32.const 16)
     )
     (i32.add
      (local.get $16)
      (i32.const 124)
     )
     (i32.const 96)
    )
   )
   (call $shine::wasm::SVector<shine::graphics::Renderer2D::Batch>::push_back\28shine::graphics::Renderer2D::Batch\20const&\29
    (local.get $8)
    (local.get $6)
   )
  )
  (i32.store offset=116
   (local.get $17)
   (i32.const 0)
  )
  (i64.store offset=108 align=4
   (local.get $17)
   (i64.const 4575657222473777152)
  )
  (i32.store offset=96
   (local.get $17)
   (i32.const 0)
  )
  (i64.store offset=88 align=4
   (local.get $17)
   (i64.const 1065353216)
  )
  (i32.store offset=76
   (local.get $17)
   (i32.const 0)
  )
  (i64.store offset=68 align=4
   (local.get $17)
   (i64.const 4575657221408423936)
  )
  (i32.store offset=56
   (local.get $17)
   (i32.const 0)
  )
  (i64.store offset=48 align=4
   (local.get $17)
   (i64.const 4575657221408423936)
  )
  (i32.store offset=36
   (local.get $17)
   (i32.const 0)
  )
  (i64.store offset=28 align=4
   (local.get $17)
   (i64.const 1065353216)
  )
  (i32.store offset=16
   (local.get $17)
   (i32.const 0)
  )
  (i64.store offset=8 align=4
   (local.get $17)
   (i64.const 0)
  )
  (f32.store offset=104
   (local.get $17)
   (local.tee $4
    (f32.add
     (local.tee $5
      (f32.mul
       (local.get $4)
       (f32.const 0.5)
      )
     )
     (local.get $2)
    )
   )
  )
  (f32.store offset=100
   (local.get $17)
   (local.tee $3
    (f32.add
     (local.tee $9
      (f32.mul
       (local.get $3)
       (f32.const 0.5)
      )
     )
     (local.get $1)
    )
   )
  )
  (f32.store offset=84
   (local.get $17)
   (local.tee $5
    (f32.sub
     (local.get $2)
     (local.get $5)
    )
   )
  )
  (f32.store offset=80
   (local.get $17)
   (local.get $3)
  )
  (f32.store offset=64
   (local.get $17)
   (local.get $4)
  )
  (f32.store offset=60
   (local.get $17)
   (local.tee $2
    (f32.sub
     (local.get $1)
     (local.get $9)
    )
   )
  )
  (f32.store offset=44
   (local.get $17)
   (local.get $4)
  )
  (f32.store offset=40
   (local.get $17)
   (local.get $2)
  )
  (f32.store offset=24
   (local.get $17)
   (local.get $5)
  )
  (f32.store offset=20
   (local.get $17)
   (local.get $3)
  )
  (f32.store offset=4
   (local.get $17)
   (local.get $5)
  )
  (f32.store
   (local.get $17)
   (local.get $2)
  )
  (i32.store
   (local.tee $17
    (i32.add
     (i32.add
      (i32.load offset=112
       (local.get $0)
      )
      (i32.mul
       (i32.load offset=104
        (local.get $0)
       )
       (i32.const 112)
      )
     )
     (i32.const -104)
    )
   )
   (i32.add
    (i32.load
     (local.get $17)
    )
    (i32.const 6)
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $16)
    (i32.const 224)
   )
  )
 )
 (func $ui_draw_rect_uv (param $0 i32) (param $1 f32) (param $2 f32) (param $3 f32) (param $4 f32) (param $5 i32)
  (call $shine::graphics::Renderer2D::drawRectUV\28int\2c\20float\2c\20float\2c\20float\2c\20float\29
   (i32.const 37452)
   (local.get $5)
   (local.get $1)
   (local.get $2)
   (local.get $3)
   (local.get $4)
  )
 )
 (func $_GLOBAL__sub_I_renderer_2d.cpp
  (drop
   (call $__cxa_atexit
    (i32.const 1)
    (i32.const 0)
    (i32.const 4660)
   )
  )
 )
 (func $shine::graphics::TextureManager::instance\28\29 (result i32)
  (i32.const 4152)
 )
 (func $shine::graphics::TextureManager::request_url\28char\20const*\2c\20int\2c\20int*\2c\20int*\2c\20int*\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (param $5 i32) (result i32)
  (call $shine::wasm::TexLoader::request_async_url\28int\2c\20char\20const*\2c\20int\2c\20int*\2c\20int*\2c\20int*\29
   (local.get $0)
   (i32.load
    (call $shine::engine::Engine::instance\28\29)
   )
   (local.get $1)
   (local.get $2)
   (local.get $3)
   (local.get $4)
   (local.get $5)
  )
 )
 (func $shine::wasm::TexLoader::request_async_url\28int\2c\20char\20const*\2c\20int\2c\20int*\2c\20int*\2c\20int*\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (param $5 i32) (param $6 i32) (result i32)
  (local $7 i32)
  (local $8 i32)
  (local.set $7
   (i32.const 0)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $2)
    )
   )
   (br_if $label$1
    (i32.lt_s
     (local.get $3)
     (i32.const 1)
    )
   )
   (br_if $label$1
    (i32.eqz
     (local.get $4)
    )
   )
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (local.tee $2
       (call $js_tex_load_url_sync
        (local.get $1)
        (local.tee $8
         (call $shine::wasm::ptr_i32\28void\20const*\29.1
          (local.get $2)
         )
        )
        (local.get $3)
       )
      )
     )
    )
    (i32.store
     (local.get $4)
     (local.get $2)
    )
    (call $shine::wasm::TexLoader::set_wh_from_tex\28int\2c\20int\2c\20int*\2c\20int*\29
     (local.get $0)
     (local.get $1)
     (local.get $2)
     (local.get $5)
     (local.get $6)
    )
    (return
     (i32.const 0)
    )
   )
   (local.set $7
    (i32.const 0)
   )
   (br_if $label$1
    (i32.lt_s
     (local.tee $2
      (call $shine::wasm::TexLoader::alloc_slot\28\29
       (local.get $0)
      )
     )
     (i32.const 0)
    )
   )
   (i32.store offset=16
    (local.tee $2
     (i32.add
      (local.get $0)
      (i32.mul
       (local.get $2)
       (i32.const 20)
      )
     )
    )
    (local.get $6)
   )
   (i32.store offset=12
    (local.get $2)
    (local.get $5)
   )
   (i32.store offset=8
    (local.get $2)
    (local.get $4)
   )
   (i32.store offset=4
    (local.get $2)
    (local.tee $7
     (i32.load offset=320
      (local.get $0)
     )
    )
   )
   (i32.store
    (local.get $2)
    (i32.const 1)
   )
   (i32.store offset=320
    (local.get $0)
    (i32.add
     (local.get $7)
     (i32.const 1)
    )
   )
   (call $js_tex_load_url
    (local.get $1)
    (local.get $8)
    (local.get $3)
    (local.get $7)
   )
  )
  (local.get $7)
 )
 (func $shine::wasm::ptr_i32\28void\20const*\29.1 (param $0 i32) (result i32)
  (local.get $0)
 )
 (func $shine::wasm::TexLoader::set_wh_from_tex\28int\2c\20int\2c\20int*\2c\20int*\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.or
      (local.get $3)
      (local.get $4)
     )
    )
   )
   (local.set $2
    (call $js_tex_get_wh
     (local.get $1)
     (local.get $2)
    )
   )
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (local.get $3)
     )
    )
    (i32.store
     (local.get $3)
     (i32.shr_u
      (local.get $2)
      (i32.const 16)
     )
    )
   )
   (br_if $label$1
    (i32.eqz
     (local.get $4)
    )
   )
   (i32.store
    (local.get $4)
    (i32.and
     (local.get $2)
     (i32.const 65535)
    )
   )
  )
 )
 (func $shine::wasm::TexLoader::alloc_slot\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (local.set $1
   (i32.const 0)
  )
  (block $label$1
   (loop $label$2
    (block $label$3
     (br_if $label$3
      (i32.ne
       (local.get $1)
       (i32.const 16)
      )
     )
     (local.set $1
      (i32.const -1)
     )
     (br $label$1)
    )
    (br_if $label$1
     (i32.eqz
      (i32.load
       (local.get $0)
      )
     )
    )
    (local.set $0
     (i32.add
      (local.get $0)
      (i32.const 20)
     )
    )
    (local.set $1
     (i32.add
      (local.get $1)
      (i32.const 1)
     )
    )
    (br $label$2)
   )
  )
  (local.get $1)
 )
 (func $shine::graphics::TextureManager::on_loaded\28int\2c\20int\2c\20int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32)
  (call $shine::wasm::TexLoader::on_loaded\28int\2c\20int\2c\20int\2c\20int\29
   (local.get $0)
   (local.get $1)
   (local.get $2)
   (local.get $3)
   (local.get $4)
  )
 )
 (func $shine::wasm::TexLoader::on_loaded\28int\2c\20int\2c\20int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local.set $5
   (i32.const 0)
  )
  (loop $label$1
   (block $label$2
    (block $label$3
     (br_if $label$3
      (i32.eq
       (local.get $5)
       (i32.const 320)
      )
     )
     (br_if $label$2
      (i32.eqz
       (i32.load
        (local.tee $6
         (i32.add
          (local.get $0)
          (local.get $5)
         )
        )
       )
      )
     )
     (br_if $label$2
      (i32.ne
       (i32.load
        (i32.add
         (local.get $6)
         (i32.const 4)
        )
       )
       (local.get $1)
      )
     )
     (block $label$4
      (br_if $label$4
       (i32.eqz
        (local.tee $5
         (i32.load
          (i32.add
           (local.get $6)
           (i32.const 8)
          )
         )
        )
       )
      )
      (i32.store
       (local.get $5)
       (local.get $2)
      )
     )
     (block $label$5
      (br_if $label$5
       (i32.eqz
        (local.tee $5
         (i32.load
          (i32.add
           (local.get $6)
           (i32.const 12)
          )
         )
        )
       )
      )
      (i32.store
       (local.get $5)
       (local.get $3)
      )
     )
     (block $label$6
      (br_if $label$6
       (i32.eqz
        (local.tee $0
         (i32.load
          (local.tee $5
           (i32.add
            (local.get $6)
            (i32.const 16)
           )
          )
         )
        )
       )
      )
      (i32.store
       (local.get $0)
       (local.get $4)
      )
     )
     (i64.store align=4
      (local.get $6)
      (i64.const 0)
     )
     (i32.store
      (i32.add
       (local.get $6)
       (i32.const 8)
      )
      (i32.const 0)
     )
     (i32.store
      (i32.add
       (local.get $6)
       (i32.const 12)
      )
      (i32.const 0)
     )
     (i32.store
      (local.get $5)
      (i32.const 0)
     )
    )
    (return)
   )
   (local.set $5
    (i32.add
     (local.get $5)
     (i32.const 20)
    )
   )
   (br $label$1)
  )
 )
 (func $shine::graphics::TextureManager::on_failed\28int\29 (param $0 i32) (param $1 i32)
  (call $shine::wasm::TexLoader::on_failed\28int\29
   (local.get $0)
   (local.get $1)
  )
 )
 (func $shine::wasm::TexLoader::on_failed\28int\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local.set $2
   (i32.const 0)
  )
  (loop $label$1
   (block $label$2
    (block $label$3
     (br_if $label$3
      (i32.eq
       (local.get $2)
       (i32.const 320)
      )
     )
     (br_if $label$2
      (i32.eqz
       (i32.load
        (local.tee $3
         (i32.add
          (local.get $0)
          (local.get $2)
         )
        )
       )
      )
     )
     (br_if $label$2
      (i32.ne
       (i32.load
        (i32.add
         (local.get $3)
         (i32.const 4)
        )
       )
       (local.get $1)
      )
     )
     (block $label$4
      (br_if $label$4
       (i32.eqz
        (local.tee $0
         (i32.load
          (local.tee $2
           (i32.add
            (local.get $3)
            (i32.const 8)
           )
          )
         )
        )
       )
      )
      (i32.store
       (local.get $0)
       (i32.const -1)
      )
     )
     (i64.store align=4
      (local.get $3)
      (i64.const 0)
     )
     (i32.store
      (local.get $2)
      (i32.const 0)
     )
     (i64.store align=4
      (i32.add
       (local.get $3)
       (i32.const 12)
      )
      (i64.const 0)
     )
    )
    (return)
   )
   (local.set $2
    (i32.add
     (local.get $2)
     (i32.const 20)
    )
   )
   (br $label$1)
  )
 )
 (func $__cxx_global_array_dtor.1 (param $0 i32)
  (drop
   (call $shine::wasm::SVector<shine::ui::Element*>::~SVector\28\29
    (i32.const 37568)
   )
  )
 )
 (func $shine::wasm::SVector<shine::ui::Element*>::~SVector\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $1
      (i32.load offset=8
       (local.get $0)
      )
     )
    )
   )
   (call $free
    (local.get $1)
   )
   (i32.store offset=8
    (local.get $0)
    (i32.const 0)
   )
  )
  (i64.store align=4
   (local.get $0)
   (i64.const 0)
  )
  (local.get $0)
 )
 (func $shine::ui::UIManager::instance\28\29 (result i32)
  (i32.const 37568)
 )
 (func $shine::ui::UIManager::add\28shine::ui::Element*\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (global.set $__stack_pointer
   (local.tee $2
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=12
   (local.get $2)
   (local.get $1)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (call $shine::wasm::SVector<shine::ui::Element*>::push_back\28shine::ui::Element*\20const&\29
    (local.get $0)
    (i32.add
     (local.get $2)
     (i32.const 12)
    )
   )
   (br_if $label$1
    (i32.lt_s
     (local.tee $1
      (i32.load offset=12
       (local.get $0)
      )
     )
     (i32.const 1)
    )
   )
   (br_if $label$1
    (i32.lt_s
     (local.tee $0
      (i32.load offset=16
       (local.get $0)
      )
     )
     (i32.const 1)
    )
   )
   (call_indirect (type $i32_i32_i32_=>_none)
    (local.tee $3
     (i32.load offset=12
      (local.get $2)
     )
    )
    (local.get $1)
    (local.get $0)
    (i32.load offset=20
     (i32.load
      (local.get $3)
     )
    )
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $2)
    (i32.const 16)
   )
  )
 )
 (func $shine::wasm::SVector<shine::ui::Element*>::push_back\28shine::ui::Element*\20const&\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.le_u
     (local.tee $3
      (i32.add
       (local.tee $2
        (i32.load
         (local.get $0)
        )
       )
       (i32.const 1)
      )
     )
     (local.tee $4
      (i32.load offset=4
       (local.get $0)
      )
     )
    )
   )
   (call $shine::wasm::SVector<shine::ui::Element*>::reserve\28unsigned\20int\29
    (local.get $0)
    (call $shine::wasm::SVector<shine::ui::Element*>::grow_cap\28unsigned\20int\2c\20unsigned\20int\29
     (local.get $4)
     (local.get $3)
    )
   )
   (local.set $2
    (i32.load
     (local.get $0)
    )
   )
  )
  (i32.store
   (local.get $0)
   (local.get $3)
  )
  (i32.store
   (i32.add
    (i32.load offset=8
     (local.get $0)
    )
    (i32.shl
     (local.get $2)
     (i32.const 2)
    )
   )
   (i32.load
    (local.get $1)
   )
  )
 )
 (func $shine::wasm::SVector<shine::ui::Element*>::grow_cap\28unsigned\20int\2c\20unsigned\20int\29 (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local.set $0
   (select
    (local.get $0)
    (i32.const 8)
    (local.get $0)
   )
  )
  (loop $label$1
   (local.set $0
    (i32.shl
     (local.tee $2
      (local.get $0)
     )
     (i32.const 1)
    )
   )
   (br_if $label$1
    (i32.lt_u
     (local.get $2)
     (local.get $1)
    )
   )
  )
  (local.get $2)
 )
 (func $shine::wasm::SVector<shine::ui::Element*>::reserve\28unsigned\20int\29 (param $0 i32) (param $1 i32)
  (call $shine::wasm::svector_reserve_impl\28void**\2c\20unsigned\20int*\2c\20unsigned\20int\2c\20unsigned\20int\2c\20unsigned\20int\29
   (i32.add
    (local.get $0)
    (i32.const 8)
   )
   (i32.add
    (local.get $0)
    (i32.const 4)
   )
   (i32.load
    (local.get $0)
   )
   (local.get $1)
   (i32.const 4)
  )
 )
 (func $shine::ui::UIManager::clear\28\29 (param $0 i32)
  (i32.store
   (local.get $0)
   (i32.const 0)
  )
 )
 (func $shine::ui::UIManager::onResize\28int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.ne
      (local.get $1)
      (i32.load offset=12
       (local.get $0)
      )
     )
    )
    (br_if $label$1
     (i32.eq
      (local.get $2)
      (i32.load offset=16
       (local.get $0)
      )
     )
    )
   )
   (i32.store offset=16
    (local.get $0)
    (local.get $2)
   )
   (i32.store offset=12
    (local.get $0)
    (local.get $1)
   )
   (local.set $3
    (i32.load
     (local.get $0)
    )
   )
   (local.set $4
    (i32.const 0)
   )
   (loop $label$3
    (br_if $label$1
     (i32.eqz
      (local.get $3)
     )
    )
    (block $label$4
     (br_if $label$4
      (i32.eqz
       (local.tee $5
        (i32.load
         (i32.add
          (i32.load offset=8
           (local.get $0)
          )
          (local.get $4)
         )
        )
       )
      )
     )
     (call_indirect (type $i32_i32_i32_=>_none)
      (local.get $5)
      (local.get $1)
      (local.get $2)
      (i32.load offset=20
       (i32.load
        (local.get $5)
       )
      )
     )
    )
    (local.set $4
     (i32.add
      (local.get $4)
      (i32.const 4)
     )
    )
    (local.set $3
     (i32.add
      (local.get $3)
      (i32.const -1)
     )
    )
    (br $label$3)
   )
  )
 )
 (func $shine::ui::UIManager::onRender\28int\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $2
      (i32.load
       (local.get $0)
      )
     )
    )
   )
   (local.set $3
    (i32.const 0)
   )
   (loop $label$2
    (br_if $label$1
     (i32.eqz
      (local.get $2)
     )
    )
    (block $label$3
     (br_if $label$3
      (i32.eqz
       (local.tee $4
        (i32.load
         (i32.add
          (i32.load offset=8
           (local.get $0)
          )
          (local.get $3)
         )
        )
       )
      )
     )
     (br_if $label$3
      (i32.eqz
       (i32.and
        (i32.load8_u offset=76
         (local.get $4)
        )
        (i32.const 1)
       )
      )
     )
     (call_indirect (type $i32_i32_=>_none)
      (local.get $4)
      (local.get $1)
      (i32.load offset=24
       (i32.load
        (local.get $4)
       )
      )
     )
    )
    (local.set $3
     (i32.add
      (local.get $3)
      (i32.const 4)
     )
    )
    (local.set $2
     (i32.add
      (local.get $2)
      (i32.const -1)
     )
    )
    (br $label$2)
   )
  )
 )
 (func $shine::ui::UIManager::onPointer\28float\2c\20float\2c\20int\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $4
      (i32.load
       (local.get $0)
      )
     )
    )
   )
   (local.set $5
    (i32.add
     (local.tee $0
      (i32.load offset=8
       (local.get $0)
      )
     )
     (i32.shl
      (local.get $4)
      (i32.const 2)
     )
    )
   )
   (loop $label$2
    (br_if $label$1
     (i32.ge_u
      (local.get $0)
      (local.get $5)
     )
    )
    (local.set $4
     (i32.load
      (local.get $0)
     )
    )
    (local.set $0
     (local.tee $6
      (i32.add
       (local.get $0)
       (i32.const 4)
      )
     )
    )
    (br_if $label$2
     (i32.eqz
      (local.get $4)
     )
    )
    (local.set $0
     (local.get $6)
    )
    (br_if $label$2
     (i32.eqz
      (i32.and
       (i32.load8_u offset=76
        (local.get $4)
       )
       (i32.const 1)
      )
     )
    )
    (call_indirect (type $i32_f32_f32_i32_=>_none)
     (local.get $4)
     (local.get $1)
     (local.get $2)
     (local.get $3)
     (i32.load offset=16
      (i32.load
       (local.get $4)
      )
     )
    )
    (local.set $0
     (local.get $6)
    )
    (br $label$2)
   )
  )
 )
 (func $_GLOBAL__sub_I_ui_manager.cpp
  (drop
   (call $__cxa_atexit
    (i32.const 2)
    (i32.const 0)
    (i32.const 4660)
   )
  )
 )
 (func $shine::engine::Engine::instance\28\29 (result i32)
  (i32.const 4476)
 )
 (func $shine::engine::Engine::init\28int\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (block $label$1
   (block $label$2
    (block $label$3
     (br_if $label$3
      (i32.eqz
       (i32.load
        (local.get $0)
       )
      )
     )
     (i32.store8 offset=4
      (local.get $0)
      (i32.const 1)
     )
     (br $label$2)
    )
    (i32.store
     (local.get $0)
     (local.tee $2
      (call $js_create_context
       (call $shine::wasm::ptr_i32\28void\20const*\29.2
        (i32.const 3099)
       )
       (i32.const 1)
      )
     )
    )
    (i32.store8 offset=4
     (local.get $0)
     (i32.ne
      (local.get $2)
      (i32.const 0)
     )
    )
    (br_if $label$1
     (i32.eqz
      (local.get $2)
     )
    )
   )
   (call $shine::graphics::Renderer2D::init\28int\29
    (call $shine::graphics::Renderer2D::instance\28\29)
    (i32.load
     (local.get $0)
    )
   )
   (br_if $label$1
    (i32.load offset=32
     (local.get $0)
    )
   )
   (i32.store offset=32
    (local.get $0)
    (local.tee $2
     (call $CreateGame\28\29)
    )
   )
   (call_indirect (type $i32_i32_=>_none)
    (local.get $2)
    (local.get $0)
    (i32.load offset=8
     (i32.load
      (local.get $2)
     )
    )
   )
  )
 )
 (func $shine::wasm::ptr_i32\28void\20const*\29.2 (param $0 i32) (result i32)
  (local.get $0)
 )
 (func $shine::engine::Engine::onResize\28int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32)
  (local $3 i32)
  (i32.store offset=12
   (local.get $0)
   (local.get $2)
  )
  (i32.store offset=8
   (local.get $0)
   (local.get $1)
  )
  (i32.store offset=84
   (call $shine::graphics::Renderer2D::instance\28\29)
   (local.get $1)
  )
  (i32.store offset=88
   (call $shine::graphics::Renderer2D::instance\28\29)
   (local.get $2)
  )
  (call $shine::ui::UIManager::onResize\28int\2c\20int\29
   (call $shine::ui::UIManager::instance\28\29)
   (local.get $1)
   (local.get $2)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $3
      (i32.load offset=32
       (local.get $0)
      )
     )
    )
   )
   (call_indirect (type $i32_i32_i32_i32_=>_none)
    (local.get $3)
    (local.get $0)
    (local.get $1)
    (local.get $2)
    (i32.load offset=12
     (i32.load
      (local.get $3)
     )
    )
   )
  )
 )
 (func $shine::engine::Engine::frame\28float\29 (param $0 i32) (param $1 f32)
  (local $2 i32)
  (block $label$1
   (br_if $label$1
    (i32.ne
     (i32.load8_u offset=4
      (local.get $0)
     )
     (i32.const 1)
    )
   )
   (call $shine::util::TimerQueue::tick\28float\29
    (i32.add
     (local.get $0)
     (i32.const 20)
    )
    (local.get $1)
   )
   (call $shine::graphics::CommandBuffer::reset\28\29
    (call $shine::graphics::CommandBuffer::instance\28\29)
   )
   (call $shine::graphics::Renderer2D::begin\28\29
    (call $shine::graphics::Renderer2D::instance\28\29)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.1
    (i32.const 1)
    (i32.const 0)
    (i32.const 0)
    (i32.load offset=8
     (local.get $0)
    )
    (i32.load offset=12
     (local.get $0)
    )
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.1
    (i32.const 2)
    (i32.const 1032805417)
    (i32.const 1032805417)
    (i32.const 1032805417)
    (i32.const 1065353216)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.1
    (i32.const 3)
    (i32.const 16384)
    (i32.const 0)
    (i32.const 0)
    (i32.const 0)
   )
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (local.tee $2
       (i32.load offset=32
        (local.get $0)
       )
      )
     )
    )
    (call_indirect (type $i32_i32_f32_=>_none)
     (local.get $2)
     (local.get $0)
     (local.get $1)
     (i32.load offset=16
      (i32.load
       (local.get $2)
      )
     )
    )
    (call_indirect (type $i32_i32_f32_=>_none)
     (local.tee $2
      (i32.load offset=32
       (local.get $0)
      )
     )
     (local.get $0)
     (local.get $1)
     (i32.load offset=20
      (i32.load
       (local.get $2)
      )
     )
    )
   )
   (call $shine::graphics::Renderer2D::end\28\29
    (call $shine::graphics::Renderer2D::instance\28\29)
   )
   (local.set $2
    (call $shine::graphics::CommandBuffer::instance\28\29)
   )
   (call $gl_submit
    (i32.load
     (local.get $0)
    )
    (call $shine::wasm::ptr_i32\28void\20const*\29.2
     (call $shine::graphics::CommandBuffer::getData\28\29
      (local.get $2)
     )
    )
    (call $shine::graphics::CommandBuffer::getCount\28\29\20const
     (local.get $2)
    )
   )
   (i32.store offset=16
    (local.get $0)
    (i32.add
     (i32.load offset=16
      (local.get $0)
     )
     (i32.const 1)
    )
   )
  )
 )
 (func $shine::util::TimerQueue::tick\28float\29 (param $0 i32) (param $1 f32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 f32)
  (f32.store offset=8
   (local.get $0)
   (local.get $1)
  )
  (local.set $2
   (local.get $0)
  )
  (block $label$1
   (loop $label$2
    (br_if $label$1
     (i32.eqz
      (local.tee $3
       (i32.load
        (local.get $2)
       )
      )
     )
    )
    (block $label$3
     (br_if $label$3
      (i32.eqz
       (i32.load8_u offset=21
        (local.get $3)
       )
      )
     )
     (i32.store
      (local.get $2)
      (i32.load offset=24
       (local.get $3)
      )
     )
     (call $free
      (local.get $3)
     )
     (br $label$2)
    )
    (block $label$4
     (br_if $label$4
      (i32.eqz
       (f32.le
        (f32.load offset=4
         (local.get $3)
        )
        (f32.load offset=8
         (local.get $0)
        )
       )
      )
     )
     (block $label$5
      (block $label$6
       (block $label$7
        (br_if $label$7
         (i32.eqz
          (local.tee $4
           (i32.load offset=12
            (local.get $3)
           )
          )
         )
        )
        (call_indirect (type $i32_i32_=>_none)
         (i32.load
          (local.get $3)
         )
         (i32.load offset=16
          (local.get $3)
         )
         (local.get $4)
        )
        (br_if $label$6
         (i32.load8_u offset=21
          (local.get $3)
         )
        )
       )
       (br_if $label$5
        (i32.load8_u offset=20
         (local.get $3)
        )
       )
      )
      (i32.store
       (local.get $2)
       (i32.load offset=24
        (local.get $3)
       )
      )
      (call $free
       (local.get $3)
      )
      (br $label$2)
     )
     (f32.store offset=4
      (local.get $3)
      (select
       (f32.add
        (local.tee $1
         (f32.load offset=8
          (local.get $3)
         )
        )
        (local.tee $5
         (f32.load offset=8
          (local.get $0)
         )
        )
       )
       (local.tee $1
        (f32.add
         (f32.load offset=4
          (local.get $3)
         )
         (local.get $1)
        )
       )
       (f32.le
        (local.get $1)
        (local.get $5)
       )
      )
     )
    )
    (local.set $2
     (i32.add
      (local.get $3)
      (i32.const 24)
     )
    )
    (br $label$2)
   )
  )
 )
 (func $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.1 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32)
  (call $shine::graphics::CommandBuffer::push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
   (call $shine::graphics::CommandBuffer::instance\28\29)
   (local.get $0)
   (local.get $1)
   (local.get $2)
   (local.get $3)
   (local.get $4)
   (i32.const 0)
   (i32.const 0)
   (i32.const 0)
  )
 )
 (func $shine::engine::Engine::pointer\28float\2c\20float\2c\20int\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 i32)
  (local $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $4
      (i32.load offset=32
       (local.get $0)
      )
     )
    )
   )
   (call_indirect (type $i32_i32_f32_f32_i32_=>_none)
    (local.get $4)
    (local.get $0)
    (local.get $1)
    (local.get $2)
    (local.get $3)
    (i32.load offset=24
     (i32.load
      (local.get $4)
     )
    )
   )
  )
 )
 (func $DemoGame::onInit\28shine::engine::Engine&\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (i32.store offset=37588
   (i32.const 0)
   (local.get $0)
  )
  (i32.store offset=64
   (local.get $0)
   (i32.const 3)
  )
  (i32.store offset=60
   (local.get $0)
   (i32.const 4)
  )
  (i32.store offset=56
   (local.get $0)
   (i32.const 0)
  )
  (i32.store offset=104
   (local.get $0)
   (call $gl_create_program_from_source\28int\2c\20char\20const*\2c\20char\20const*\29
    (local.tee $1
     (i32.load
      (local.get $1)
     )
    )
    (i32.const 3104)
    (i32.const 3248)
   )
  )
  (i32.store offset=108
   (local.get $0)
   (call $gl_create_buffer
    (local.get $1)
   )
  )
  (i32.store offset=112
   (local.get $0)
   (local.tee $2
    (call $gl_create_vertex_array
     (local.get $1)
    )
   )
  )
  (call $gl_bind_vertex_array
   (local.get $1)
   (local.get $2)
  )
  (call $gl_setup_attribs_basic
   (local.get $1)
   (i32.load offset=108
    (local.get $0)
   )
  )
  (call $gl_bind_vertex_array
   (local.get $1)
   (i32.const 0)
  )
  (i32.store offset=116
   (local.get $0)
   (call $gl_create_program_instanced
    (local.get $1)
    (call $gl_create_shader
     (local.get $1)
     (i32.const 35633)
     (i32.const 3360)
     (i32.const 214)
    )
    (call $gl_create_shader
     (local.get $1)
     (i32.const 35632)
     (i32.const 3248)
     (i32.const 108)
    )
   )
  )
  (i32.store offset=120
   (local.get $0)
   (local.tee $2
    (call $gl_create_buffer
     (local.get $1)
    )
   )
  )
  (call $gl_bind_buffer
   (local.get $1)
   (i32.const 34962)
   (local.get $2)
  )
  (call $gl_buffer_data_f32
   (local.get $1)
   (i32.const 34962)
   (call $shine::wasm::ptr_i32\28void\20const*\29.3
    (i32.const 3584)
   )
   (i32.const 30)
   (i32.const 35048)
  )
  (i32.store offset=124
   (local.get $0)
   (call $gl_create_buffer
    (local.get $1)
   )
  )
  (i32.store offset=128
   (local.get $0)
   (local.tee $2
    (call $gl_create_vertex_array
     (local.get $1)
    )
   )
  )
  (call $gl_bind_vertex_array
   (local.get $1)
   (local.get $2)
  )
  (call $gl_setup_attribs_instanced
   (local.get $1)
   (i32.load offset=120
    (local.get $0)
   )
   (i32.load offset=124
    (local.get $0)
   )
  )
  (call $gl_bind_vertex_array
   (local.get $1)
   (i32.const 0)
  )
  (call $DemoGame::ensure_buffer\28int\29
   (local.get $0)
   (i32.const 1500)
  )
  (call $DemoGame::ensure_instanced\28int\29
   (local.get $0)
   (i32.const 500)
  )
  (i32.store offset=68
   (local.get $0)
   (local.tee $2
    (call $shine::game::Node*\20shine::game::Node::addChildNode<shine::game::Node\2c\20char\20const\20\28&\29\20\5b7\5d>\28char\20const\20\28&\29\20\5b7\5d\29
     (i32.add
      (local.get $0)
      (i32.const 4)
     )
     (i32.const 3709)
    )
   )
  )
  (i32.store offset=72
   (local.get $0)
   (call $shine::game::Node*\20shine::game::Node::addChildNode<shine::game::Node\2c\20char\20const\20\28&\29\20\5b7\5d>\28char\20const\20\28&\29\20\5b7\5d\29
    (local.get $2)
    (i32.const 3716)
   )
  )
  (i64.store offset=56 align=4
   (local.tee $2
    (call $shine::game::Transform*\20shine::game::Node::addComponent<shine::game::Transform>\28\29
     (i32.load offset=68
      (local.get $0)
     )
    )
   )
   (i64.const 4518011146371019571)
  )
  (i64.store offset=48 align=4
   (local.get $2)
   (i64.const 0)
  )
  (i32.store offset=48
   (call $shine::game::SpriteRenderer*\20shine::game::Node::addComponent<shine::game::SpriteRenderer>\28\29
    (i32.load offset=68
     (local.get $0)
    )
   )
   (call $js_create_texture_checker
    (local.get $1)
    (i32.const 64)
   )
  )
  (i64.store offset=56 align=4
   (local.tee $2
    (call $shine::game::Transform*\20shine::game::Node::addComponent<shine::game::Transform>\28\29
     (i32.load offset=72
      (local.get $0)
     )
    )
   )
   (i64.const 4464688526090389422)
  )
  (i64.store offset=48 align=4
   (local.get $2)
   (i64.const 4417130516439262822)
  )
  (i32.store offset=60
   (local.tee $2
    (call $shine::game::SpriteRenderer*\20shine::game::Node::addComponent<shine::game::SpriteRenderer>\28\29
     (i32.load offset=72
      (local.get $0)
     )
    )
   )
   (i32.const 1045220557)
  )
  (i64.store offset=52 align=4
   (local.get $2)
   (i64.const 4489188110485579366)
  )
  (i32.store offset=48
   (local.get $2)
   (i32.const 0)
  )
  (call $shine::game::Component::attachChild\28shine::game::Component*\29
   (local.get $2)
   (call $PulseColor::PulseColor\28shine::game::SpriteRenderer*\29
    (call $operator\20new\28unsigned\20long\29
     (i32.const 56)
    )
    (local.get $2)
   )
  )
  (drop
   (call $KillOnClick*\20shine::game::Node::addComponent<KillOnClick>\28\29
    (i32.load offset=72
     (local.get $0)
    )
   )
  )
  (call $shine::ui::UIManager::clear\28\29
   (call $shine::ui::UIManager::instance\28\29)
  )
  (i32.store offset=100
   (local.tee $2
    (call $shine::ui::Button::create\28\29)
   )
   (i32.const 5)
  )
  (i32.store offset=96
   (local.get $2)
   (i32.const 6)
  )
  (i32.store offset=92
   (local.get $2)
   (i32.const 7)
  )
  (i32.store offset=132
   (local.get $0)
   (local.get $2)
  )
  (call $shine::ui::Button::setBgUrl\28char\20const*\29
   (local.get $2)
   (i32.const 3723)
  )
  (i64.store offset=20 align=4
   (local.tee $2
    (i32.load offset=132
     (local.get $0)
    )
   )
   (i64.const 4539628425446424576)
  )
  (call $shine::ui::Element::setLayoutRel\28float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\29
   (local.get $2)
   (f32.const 0.5)
   (f32.const 0.5)
   (f32.const 0)
   (f32.const 0)
   (f32.const 0.18000000715255737)
   (f32.const 0.09000000357627869)
  )
  (call $shine::ui::Element::setLayoutPx\28float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\29
   (i32.load offset=132
    (local.get $0)
   )
   (f32.const 0.5)
   (f32.const 0.5)
   (f32.const -50)
   (f32.const 50)
   (f32.const 100)
   (f32.const 100)
  )
  (call $shine::ui::UIManager::add\28shine::ui::Element*\29
   (call $shine::ui::UIManager::instance\28\29)
   (i32.load offset=132
    (local.get $0)
   )
  )
  (i32.store offset=92
   (local.tee $2
    (call $shine::ui::Button::create\28\29)
   )
   (i32.const 8)
  )
  (i32.store offset=136
   (local.get $0)
   (local.get $2)
  )
  (call $shine::ui::Element::setLayoutRel\28float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\29
   (local.get $2)
   (f32.const 0)
   (f32.const 0)
   (f32.const 12)
   (f32.const 12)
   (f32.const 0.20000000298023224)
   (f32.const 0.07999999821186066)
  )
  (call $shine::ui::UIManager::add\28shine::ui::Element*\29
   (call $shine::ui::UIManager::instance\28\29)
   (i32.load offset=136
    (local.get $0)
   )
  )
  (local.set $2
   (call $operator\20new\28unsigned\20long\29
    (i32.const 96)
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.const 96)
    )
   )
   (memory.fill
    (local.get $2)
    (i32.const 0)
    (i32.const 96)
   )
  )
  (i64.store offset=20 align=4
   (local.tee $2
    (call $shine::ui::Image::Image\28\29
     (local.get $2)
    )
   )
   (i64.const 4575657222473777152)
  )
  (i32.store offset=140
   (local.get $0)
   (local.get $2)
  )
  (call $shine::ui::Element::setLayoutRel\28float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\29
   (local.get $2)
   (f32.const 1)
   (f32.const 1)
   (f32.const -12)
   (f32.const -12)
   (f32.const 0.30000001192092896)
   (f32.const 0.2199999988079071)
  )
  (local.set $1
   (call $js_create_texture_checker
    (local.get $1)
    (i32.const 64)
   )
  )
  (i32.store offset=84
   (i32.load offset=140
    (local.get $0)
   )
   (local.get $1)
  )
  (call $shine::ui::UIManager::add\28shine::ui::Element*\29
   (call $shine::ui::UIManager::instance\28\29)
   (i32.load offset=140
    (local.get $0)
   )
  )
 )
 (func $demo_rc_draw_rect_tex\28void*\2c\20int\2c\20float\2c\20float\2c\20float\2c\20float\29 (param $0 i32) (param $1 i32) (param $2 f32) (param $3 f32) (param $4 f32) (param $5 f32)
  (call $shine::graphics::Renderer2D::drawRectUV\28int\2c\20float\2c\20float\2c\20float\2c\20float\29
   (call $shine::graphics::Renderer2D::instance\28\29)
   (local.get $1)
   (local.get $2)
   (local.get $3)
   (local.get $4)
   (local.get $5)
  )
 )
 (func $demo_rc_draw_rect_col\28void*\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 f32) (param $4 f32) (param $5 f32) (param $6 f32) (param $7 f32)
  (call $shine::graphics::Renderer2D::drawRectColor\28float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\29
   (call $shine::graphics::Renderer2D::instance\28\29)
   (local.get $1)
   (local.get $2)
   (local.get $3)
   (local.get $4)
   (local.get $5)
   (local.get $6)
   (local.get $7)
  )
 )
 (func $shine::wasm::ptr_i32\28void\20const*\29.3 (param $0 i32) (result i32)
  (local.get $0)
 )
 (func $DemoGame::ensure_buffer\28int\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (global.set $__stack_pointer
   (local.tee $2
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (drop
   (call $shine::wasm::SArray<float>::operator=\28shine::wasm::SArray<float>&&\29
    (i32.add
     (local.get $0)
     (i32.const 84)
    )
    (local.tee $3
     (call $shine::wasm::SArray<float>::SArray\28unsigned\20int\29
      (i32.add
       (local.get $2)
       (i32.const 8)
      )
      (i32.mul
       (local.get $1)
       (i32.const 15)
      )
     )
    )
   )
  )
  (drop
   (call $shine::wasm::SArray<float>::~SArray\28\29
    (local.get $3)
   )
  )
  (i32.store offset=80
   (local.get $0)
   (local.get $1)
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $2)
    (i32.const 16)
   )
  )
 )
 (func $DemoGame::ensure_instanced\28int\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (global.set $__stack_pointer
   (local.tee $2
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=92
   (local.get $0)
   (local.tee $1
    (select
     (local.tee $1
      (select
       (local.get $1)
       (i32.const 1)
       (i32.gt_s
        (local.get $1)
        (i32.const 1)
       )
      )
     )
     (i32.const 20000)
     (i32.lt_s
      (local.get $1)
      (i32.const 20000)
     )
    )
   )
  )
  (drop
   (call $shine::wasm::SArray<float>::operator=\28shine::wasm::SArray<float>&&\29
    (i32.add
     (local.get $0)
     (i32.const 96)
    )
    (local.tee $1
     (call $shine::wasm::SArray<float>::SArray\28unsigned\20int\29
      (i32.add
       (local.get $2)
       (i32.const 8)
      )
      (i32.mul
       (local.get $1)
       (i32.const 6)
      )
     )
    )
   )
  )
  (drop
   (call $shine::wasm::SArray<float>::~SArray\28\29
    (local.get $1)
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $2)
    (i32.const 16)
   )
  )
 )
 (func $shine::game::Node*\20shine::game::Node::addChildNode<shine::game::Node\2c\20char\20const\20\28&\29\20\5b7\5d>\28char\20const\20\28&\29\20\5b7\5d\29 (param $0 i32) (param $1 i32) (result i32)
  (call $shine::game::Node::attachChild\28shine::game::Node*\29
   (local.get $0)
   (local.tee $1
    (call $shine::game::Node::Node\28char\20const*\29
     (call $operator\20new\28unsigned\20long\29
      (i32.const 52)
     )
     (local.get $1)
    )
   )
  )
  (local.get $1)
 )
 (func $shine::game::Transform*\20shine::game::Node::addComponent<shine::game::Transform>\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (local.set $1
   (call $operator\20new\28unsigned\20long\29
    (i32.const 64)
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.const 64)
    )
   )
   (memory.fill
    (local.get $1)
    (i32.const 0)
    (i32.const 64)
   )
  )
  (i32.store offset=28
   (local.tee $1
    (call $shine::game::Transform::Transform\28\29
     (local.get $1)
    )
   )
   (i32.const 37596)
  )
  (call $shine::game::Node::attachComponent\28shine::game::Component*\29
   (local.get $0)
   (local.get $1)
  )
  (local.get $1)
 )
 (func $shine::game::SpriteRenderer*\20shine::game::Node::addComponent<shine::game::SpriteRenderer>\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (local.set $1
   (call $operator\20new\28unsigned\20long\29
    (i32.const 64)
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.const 64)
    )
   )
   (memory.fill
    (local.get $1)
    (i32.const 0)
    (i32.const 64)
   )
  )
  (i32.store offset=28
   (local.tee $1
    (call $shine::game::SpriteRenderer::SpriteRenderer\28\29
     (local.get $1)
    )
   )
   (i32.const 37600)
  )
  (call $shine::game::Node::attachComponent\28shine::game::Component*\29
   (local.get $0)
   (local.get $1)
  )
  (local.get $1)
 )
 (func $PulseColor::PulseColor\28shine::game::SpriteRenderer*\29 (param $0 i32) (param $1 i32) (result i32)
  (i32.store offset=52
   (local.tee $0
    (call $shine::game::Component::Component\28char\20const*\29
     (local.get $0)
     (i32.const 0)
    )
   )
   (i32.const 1048576000)
  )
  (i32.store offset=48
   (local.get $0)
   (local.get $1)
  )
  (i32.store
   (local.get $0)
   (i32.const 3784)
  )
  (i32.store offset=28
   (local.get $0)
   (i32.const 37592)
  )
  (local.get $0)
 )
 (func $shine::game::Component::attachChild\28shine::game::Component*\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (global.set $__stack_pointer
   (local.tee $2
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=12
   (local.get $2)
   (local.get $1)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (i32.store offset=44
    (local.get $1)
    (local.get $0)
   )
   (i32.store offset=24
    (local.get $1)
    (i32.load offset=24
     (local.get $0)
    )
   )
   (call $shine::wasm::SVector<shine::game::Component*>::push_back\28shine::game::Component*\20const&\29
    (i32.add
     (local.get $0)
     (i32.const 32)
    )
    (i32.add
     (local.get $2)
     (i32.const 12)
    )
   )
   (call_indirect (type $i32_=>_none)
    (local.tee $1
     (i32.load offset=12
      (local.get $2)
     )
    )
    (i32.load offset=16
     (i32.load
      (local.get $1)
     )
    )
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $2)
    (i32.const 16)
   )
  )
 )
 (func $KillOnClick*\20shine::game::Node::addComponent<KillOnClick>\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (i32.store offset=28
   (local.tee $1
    (call $KillOnClick::KillOnClick\28\29
     (call $operator\20new\28unsigned\20long\29
      (i32.const 48)
     )
    )
   )
   (i32.const 37604)
  )
  (call $shine::game::Node::attachComponent\28shine::game::Component*\29
   (local.get $0)
   (local.get $1)
  )
  (local.get $1)
 )
 (func $shine::ui::Button::create\28\29 (result i32)
  (local $0 i32)
  (local.set $0
   (call $operator\20new\28unsigned\20long\29
    (i32.const 112)
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.const 112)
    )
   )
   (memory.fill
    (local.get $0)
    (i32.const 0)
    (i32.const 112)
   )
  )
  (call_indirect (type $i32_=>_none)
   (local.tee $0
    (call $shine::ui::Button::Button\28\29
     (local.get $0)
    )
   )
   (i32.load offset=8
    (i32.load
     (local.get $0)
    )
   )
  )
  (local.get $0)
 )
 (func $DemoGame::onInit\28shine::engine::Engine&\29::$_2::__invoke\28shine::ui::Button*\29 (param $0 i32)
 )
 (func $DemoGame::onInit\28shine::engine::Engine&\29::$_1::__invoke\28shine::ui::Button*\29 (param $0 i32)
 )
 (func $DemoGame::onInit\28shine::engine::Engine&\29::$_0::__invoke\28shine::ui::Button*\29 (param $0 i32)
 )
 (func $shine::ui::Button::setBgUrl\28char\20const*\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local.set $2
   (i32.const 0)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (local.set $2
    (i32.const 0)
   )
   (loop $label$2
    (local.set $3
     (i32.add
      (local.get $1)
      (local.get $2)
     )
    )
    (local.set $2
     (local.tee $4
      (i32.add
       (local.get $2)
       (i32.const 1)
      )
     )
    )
    (br_if $label$2
     (i32.load8_u
      (local.get $3)
     )
    )
   )
   (local.set $2
    (i32.add
     (local.get $4)
     (i32.const -1)
    )
   )
  )
  (drop
   (call $shine::graphics::TextureManager::request_url\28char\20const*\2c\20int\2c\20int*\2c\20int*\2c\20int*\29
    (call $shine::graphics::TextureManager::instance\28\29)
    (local.get $1)
    (local.get $2)
    (i32.add
     (i32.load offset=84
      (local.get $0)
     )
     (i32.const 104)
    )
    (i32.const 0)
    (i32.const 0)
   )
  )
 )
 (func $shine::ui::Element::setLayoutRel\28float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 f32) (param $4 f32) (param $5 f32) (param $6 f32)
  (f32.store offset=48
   (local.get $0)
   (local.get $6)
  )
  (f32.store offset=44
   (local.get $0)
   (local.get $5)
  )
  (f32.store offset=32
   (local.get $0)
   (local.get $4)
  )
  (f32.store offset=28
   (local.get $0)
   (local.get $3)
  )
  (f32.store offset=16
   (local.get $0)
   (local.get $2)
  )
  (f32.store offset=12
   (local.get $0)
   (local.get $1)
  )
  (f32.store offset=8
   (local.get $0)
   (local.get $2)
  )
  (f32.store offset=4
   (local.get $0)
   (local.get $1)
  )
  (call_indirect (type $i32_i32_i32_=>_none)
   (local.get $0)
   (i32.load offset=68
    (local.get $0)
   )
   (i32.load offset=72
    (local.get $0)
   )
   (i32.load offset=20
    (i32.load
     (local.get $0)
    )
   )
  )
 )
 (func $shine::ui::Element::setLayoutPx\28float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20float\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 f32) (param $4 f32) (param $5 f32) (param $6 f32)
  (i64.store offset=44 align=4
   (local.get $0)
   (i64.const 0)
  )
  (f32.store offset=40
   (local.get $0)
   (local.get $6)
  )
  (f32.store offset=36
   (local.get $0)
   (local.get $5)
  )
  (f32.store offset=32
   (local.get $0)
   (local.get $4)
  )
  (f32.store offset=28
   (local.get $0)
   (local.get $3)
  )
  (f32.store offset=16
   (local.get $0)
   (local.get $2)
  )
  (f32.store offset=12
   (local.get $0)
   (local.get $1)
  )
  (f32.store offset=8
   (local.get $0)
   (local.get $2)
  )
  (f32.store offset=4
   (local.get $0)
   (local.get $1)
  )
  (call_indirect (type $i32_i32_i32_=>_none)
   (local.get $0)
   (i32.load offset=68
    (local.get $0)
   )
   (i32.load offset=72
    (local.get $0)
   )
   (i32.load offset=20
    (i32.load
     (local.get $0)
    )
   )
  )
 )
 (func $demo_on_mode_click\28shine::ui::Button*\29 (param $0 i32)
  (local $1 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $1
      (i32.load offset=37588
       (i32.const 0)
      )
     )
    )
   )
   (i32.store offset=76
    (local.get $1)
    (i32.eqz
     (i32.load offset=76
      (local.get $1)
     )
    )
   )
  )
 )
 (func $shine::ui::Image::Image\28\29 (param $0 i32) (result i32)
  (i32.store offset=92
   (local.tee $0
    (call $shine::ui::Element::Element\28\29
     (local.get $0)
    )
   )
   (i32.const 0)
  )
  (i64.store offset=84 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i32.store
   (local.get $0)
   (i32.const 3968)
  )
  (local.get $0)
 )
 (func $shine::wasm::SArray<float>::SArray\28unsigned\20int\29 (param $0 i32) (param $1 i32) (result i32)
  (i64.store align=4
   (local.get $0)
   (i64.const 0)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (i32.store
    (local.get $0)
    (local.get $1)
   )
   (i32.store offset=4
    (local.get $0)
    (call $malloc
     (i32.shl
      (local.get $1)
      (i32.const 2)
     )
    )
   )
  )
  (local.get $0)
 )
 (func $shine::wasm::SArray<float>::operator=\28shine::wasm::SArray<float>&&\29 (param $0 i32) (param $1 i32) (result i32)
  (block $label$1
   (br_if $label$1
    (i32.eq
     (local.get $0)
     (local.get $1)
    )
   )
   (call $shine::wasm::SArray<float>::reset\28\29
    (local.get $0)
   )
   (i64.store align=4
    (local.get $0)
    (i64.load align=4
     (local.get $1)
    )
   )
   (i32.store offset=4
    (local.get $1)
    (i32.const 0)
   )
   (i32.store
    (local.get $1)
    (i32.const 0)
   )
  )
  (local.get $0)
 )
 (func $shine::wasm::SArray<float>::~SArray\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $1
      (i32.load offset=4
       (local.get $0)
      )
     )
    )
   )
   (call $free
    (local.get $1)
   )
   (i32.store offset=4
    (local.get $0)
    (i32.const 0)
   )
  )
  (local.get $0)
 )
 (func $shine::game::Node::Node\28char\20const*\29 (param $0 i32) (param $1 i32) (result i32)
  (i32.store offset=48
   (local.tee $1
    (call $shine::game::Object::Object\28char\20const*\29
     (local.get $0)
     (local.get $1)
    )
   )
   (i32.const 0)
  )
  (i64.store offset=40 align=4
   (local.get $1)
   (i64.const 0)
  )
  (i64.store offset=32 align=4
   (local.get $1)
   (i64.const 0)
  )
  (i64.store offset=24 align=4
   (local.get $1)
   (i64.const 0)
  )
  (i32.store
   (local.get $1)
   (i32.const 4004)
  )
  (local.get $1)
 )
 (func $shine::game::Node::attachChild\28shine::game::Node*\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (global.set $__stack_pointer
   (local.tee $2
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=12
   (local.get $2)
   (local.get $1)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (i32.store offset=24
    (local.get $1)
    (local.get $0)
   )
   (call $shine::wasm::SVector<shine::game::Node*>::push_back\28shine::game::Node*\20const&\29
    (i32.add
     (local.get $0)
     (i32.const 28)
    )
    (i32.add
     (local.get $2)
     (i32.const 12)
    )
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $2)
    (i32.const 16)
   )
  )
 )
 (func $shine::game::Transform::Transform\28\29 (param $0 i32) (result i32)
  (i64.store offset=56 align=4
   (local.tee $0
    (call $shine::game::Component::Component\28char\20const*\29
     (local.get $0)
     (i32.const 0)
    )
   )
   (i64.const 4503599628419072000)
  )
  (i64.store offset=48 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i32.store
   (local.get $0)
   (i32.const 4028)
  )
  (local.get $0)
 )
 (func $shine::game::Node::attachComponent\28shine::game::Component*\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (global.set $__stack_pointer
   (local.tee $2
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=12
   (local.get $2)
   (local.get $1)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (i32.store offset=44
    (local.get $1)
    (i32.const 0)
   )
   (i32.store offset=24
    (local.get $1)
    (local.get $0)
   )
   (call $shine::wasm::SVector<shine::game::Component*>::push_back\28shine::game::Component*\20const&\29
    (i32.add
     (local.get $0)
     (i32.const 40)
    )
    (i32.add
     (local.get $2)
     (i32.const 12)
    )
   )
   (call_indirect (type $i32_=>_none)
    (local.tee $1
     (i32.load offset=12
      (local.get $2)
     )
    )
    (i32.load offset=16
     (i32.load
      (local.get $1)
     )
    )
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $2)
    (i32.const 16)
   )
  )
 )
 (func $shine::game::SpriteRenderer::SpriteRenderer\28\29 (param $0 i32) (result i32)
  (i32.store offset=60
   (local.tee $0
    (call $shine::game::Component::Component\28char\20const*\29
     (local.get $0)
     (i32.const 0)
    )
   )
   (i32.const 1065353216)
  )
  (i64.store offset=52 align=4
   (local.get $0)
   (i64.const 4575657222473777152)
  )
  (i32.store offset=48
   (local.get $0)
   (i32.const 0)
  )
  (i32.store
   (local.get $0)
   (i32.const 4072)
  )
  (local.get $0)
 )
 (func $shine::wasm::SVector<shine::game::Component*>::push_back\28shine::game::Component*\20const&\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.le_u
     (local.tee $3
      (i32.add
       (local.tee $2
        (i32.load
         (local.get $0)
        )
       )
       (i32.const 1)
      )
     )
     (local.tee $4
      (i32.load offset=4
       (local.get $0)
      )
     )
    )
   )
   (call $shine::wasm::SVector<shine::game::Component*>::reserve\28unsigned\20int\29
    (local.get $0)
    (call $shine::wasm::SVector<shine::game::Component*>::grow_cap\28unsigned\20int\2c\20unsigned\20int\29
     (local.get $4)
     (local.get $3)
    )
   )
   (local.set $2
    (i32.load
     (local.get $0)
    )
   )
  )
  (i32.store
   (local.get $0)
   (local.get $3)
  )
  (i32.store
   (i32.add
    (i32.load offset=8
     (local.get $0)
    )
    (i32.shl
     (local.get $2)
     (i32.const 2)
    )
   )
   (i32.load
    (local.get $1)
   )
  )
 )
 (func $shine::game::Component::Component\28char\20const*\29 (param $0 i32) (param $1 i32) (result i32)
  (i64.store offset=40 align=4
   (local.tee $1
    (call $shine::game::Object::Object\28char\20const*\29
     (local.get $0)
     (local.get $1)
    )
   )
   (i64.const 0)
  )
  (i64.store offset=32 align=4
   (local.get $1)
   (i64.const 0)
  )
  (i64.store offset=24 align=4
   (local.get $1)
   (i64.const 0)
  )
  (i32.store
   (local.get $1)
   (i32.const 3828)
  )
  (local.get $1)
 )
 (func $KillOnClick::KillOnClick\28\29 (param $0 i32) (result i32)
  (i32.store offset=28
   (local.tee $0
    (call $shine::game::Component::Component\28char\20const*\29
     (local.get $0)
     (i32.const 0)
    )
   )
   (i32.const 37604)
  )
  (i32.store
   (local.get $0)
   (i32.const 4116)
  )
  (local.get $0)
 )
 (func $shine::ui::Button::Button\28\29 (param $0 i32) (result i32)
  (i32.store offset=108
   (local.tee $0
    (call $shine::ui::Element::Element\28\29
     (local.get $0)
    )
   )
   (i32.const 0)
  )
  (i64.store offset=100 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=92 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=84 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i32.store
   (local.get $0)
   (i32.const 3896)
  )
  (local.get $0)
 )
 (func $shine::ui::Element::Element\28\29 (param $0 i32) (result i32)
  (i64.store offset=68 align=4
   (local.get $0)
   (i64.const 4294967297)
  )
  (i64.store offset=60 align=4
   (local.get $0)
   (i64.const 4776067405946814464)
  )
  (i64.store offset=52 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=44 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=36 align=4
   (local.get $0)
   (i64.const 4776067405946814464)
  )
  (i64.store offset=28 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=20 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=12 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=4 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i32.store
   (local.get $0)
   (i32.const 3932)
  )
  (local.get $0)
 )
 (func $DemoGame::onResize\28shine::engine::Engine&\2c\20int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32)
 )
 (func $DemoGame::onUpdate\28shine::engine::Engine&\2c\20float\29 (param $0 i32) (param $1 i32) (param $2 f32)
  (call $shine::game::Node::update\28float\29
   (local.tee $0
    (i32.add
     (local.get $0)
     (i32.const 4)
    )
   )
   (local.get $2)
  )
  (call $shine::game::Scene::collectGarbage\28\29
   (local.get $0)
  )
 )
 (func $shine::game::Node::update\28float\29 (param $0 i32) (param $1 f32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.and
      (i32.load8_u offset=12
       (local.get $0)
      )
      (i32.const 1)
     )
    )
   )
   (local.set $2
    (i32.const 0)
   )
   (local.set $3
    (i32.const 0)
   )
   (loop $label$2
    (block $label$3
     (br_if $label$3
      (i32.lt_u
       (local.get $3)
       (i32.load offset=40
        (local.get $0)
       )
      )
     )
     (local.set $2
      (i32.const 0)
     )
     (local.set $3
      (i32.const 0)
     )
     (loop $label$4
      (br_if $label$1
       (i32.ge_u
        (local.get $3)
        (i32.load offset=28
         (local.get $0)
        )
       )
      )
      (block $label$5
       (br_if $label$5
        (i32.eqz
         (local.tee $4
          (i32.load
           (i32.add
            (i32.load offset=36
             (local.get $0)
            )
            (local.get $2)
           )
          )
         )
        )
       )
       (call $shine::game::Node::update\28float\29
        (local.get $4)
        (local.get $1)
       )
      )
      (local.set $2
       (i32.add
        (local.get $2)
        (i32.const 4)
       )
      )
      (local.set $3
       (i32.add
        (local.get $3)
        (i32.const 1)
       )
      )
      (br $label$4)
     )
    )
    (block $label$6
     (br_if $label$6
      (i32.eqz
       (local.tee $4
        (i32.load
         (i32.add
          (i32.load offset=48
           (local.get $0)
          )
          (local.get $2)
         )
        )
       )
      )
     )
     (call $shine::game::Component::update\28float\29
      (local.get $4)
      (local.get $1)
     )
    )
    (local.set $2
     (i32.add
      (local.get $2)
      (i32.const 4)
     )
    )
    (local.set $3
     (i32.add
      (local.get $3)
      (i32.const 1)
     )
    )
    (br $label$2)
   )
  )
 )
 (func $shine::game::Scene::collectGarbage\28\29 (param $0 i32)
  (local $1 i32)
  (local.set $1
   (i32.const 4664)
  )
  (block $label$1
   (loop $label$2
    (block $label$3
     (br_if $label$3
      (local.tee $1
       (i32.load
        (local.get $1)
       )
      )
     )
     (call $shine::game::Node::markTree\28\29
      (local.get $0)
     )
     (local.set $0
      (i32.load offset=4664
       (i32.const 0)
      )
     )
     (loop $label$4
      (br_if $label$1
       (i32.eqz
        (local.tee $1
         (local.get $0)
        )
       )
      )
      (local.set $0
       (i32.load offset=20
        (local.get $1)
       )
      )
      (br_if $label$4
       (i32.eq
        (i32.and
         (i32.load offset=12
          (local.get $1)
         )
         (i32.const 96)
        )
        (i32.const 64)
       )
      )
      (br_if $label$4
       (call_indirect (type $i32_=>_i32)
        (local.get $1)
        (i32.load offset=12
         (i32.load
          (local.get $1)
         )
        )
       )
      )
      (call_indirect (type $i32_=>_none)
       (local.get $1)
       (i32.load offset=4
        (i32.load
         (local.get $1)
        )
       )
      )
      (br $label$4)
     )
    )
    (i32.store offset=12
     (local.get $1)
     (i32.and
      (i32.load offset=12
       (local.get $1)
      )
      (i32.const -65)
     )
    )
    (local.set $1
     (i32.add
      (local.get $1)
      (i32.const 20)
     )
    )
    (br $label$2)
   )
  )
 )
 (func $shine::game::Node::markTree\28\29 (param $0 i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (i32.store offset=12
   (local.get $0)
   (i32.or
    (i32.load offset=12
     (local.get $0)
    )
    (i32.const 64)
   )
  )
  (local.set $1
   (i32.const 0)
  )
  (local.set $2
   (i32.const 0)
  )
  (block $label$1
   (loop $label$2
    (block $label$3
     (br_if $label$3
      (i32.lt_u
       (local.get $2)
       (i32.load offset=40
        (local.get $0)
       )
      )
     )
     (local.set $1
      (i32.const 0)
     )
     (local.set $2
      (i32.const 0)
     )
     (loop $label$4
      (br_if $label$1
       (i32.ge_u
        (local.get $2)
        (i32.load offset=28
         (local.get $0)
        )
       )
      )
      (block $label$5
       (br_if $label$5
        (i32.eqz
         (local.tee $3
          (i32.load
           (i32.add
            (i32.load offset=36
             (local.get $0)
            )
            (local.get $1)
           )
          )
         )
        )
       )
       (call $shine::game::Node::markTree\28\29
        (local.get $3)
       )
      )
      (local.set $1
       (i32.add
        (local.get $1)
        (i32.const 4)
       )
      )
      (local.set $2
       (i32.add
        (local.get $2)
        (i32.const 1)
       )
      )
      (br $label$4)
     )
    )
    (block $label$6
     (br_if $label$6
      (i32.eqz
       (local.tee $3
        (i32.load
         (i32.add
          (i32.load offset=48
           (local.get $0)
          )
          (local.get $1)
         )
        )
       )
      )
     )
     (call $shine::game::Component::markTree\28\29
      (local.get $3)
     )
    )
    (local.set $1
     (i32.add
      (local.get $1)
      (i32.const 4)
     )
    )
    (local.set $2
     (i32.add
      (local.get $2)
      (i32.const 1)
     )
    )
    (br $label$2)
   )
  )
 )
 (func $DemoGame::onRender\28shine::engine::Engine&\2c\20float\29 (param $0 i32) (param $1 i32) (param $2 f32)
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.load offset=76
      (local.get $0)
     )
    )
    (call $DemoGame::update_vertices\28float\29
     (local.get $0)
     (local.get $2)
    )
    (br_if $label$1
     (i32.lt_s
      (i32.load offset=80
       (local.get $0)
      )
      (i32.const 1)
     )
    )
    (br_if $label$1
     (i32.eqz
      (i32.load offset=88
       (local.get $0)
      )
     )
    )
    (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2
     (i32.const 5)
     (i32.const 34962)
     (i32.load offset=108
      (local.get $0)
     )
     (i32.const 0)
     (i32.const 0)
    )
    (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2
     (i32.const 6)
     (i32.const 34962)
     (call $shine::wasm::ptr_i32\28void\20const*\29.3
      (i32.load offset=88
       (local.get $0)
      )
     )
     (i32.mul
      (i32.load offset=80
       (local.get $0)
      )
      (i32.const 15)
     )
     (i32.const 35048)
    )
    (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2
     (i32.const 16)
     (i32.load offset=112
      (local.get $0)
     )
     (i32.const 0)
     (i32.const 0)
     (i32.const 0)
    )
    (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2
     (i32.const 4)
     (i32.load offset=104
      (local.get $0)
     )
     (i32.const 0)
     (i32.const 0)
     (i32.const 0)
    )
    (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2
     (i32.const 8)
     (i32.const 4)
     (i32.const 0)
     (i32.mul
      (i32.load offset=80
       (local.get $0)
      )
      (i32.const 3)
     )
     (i32.const 0)
    )
    (br $label$1)
   )
   (call $DemoGame::update_instances\28float\29
    (local.get $0)
    (local.get $2)
   )
   (br_if $label$1
    (i32.lt_s
     (i32.load offset=92
      (local.get $0)
     )
     (i32.const 1)
    )
   )
   (br_if $label$1
    (i32.eqz
     (i32.load offset=100
      (local.get $0)
     )
    )
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2
    (i32.const 5)
    (i32.const 34962)
    (i32.load offset=124
     (local.get $0)
    )
    (i32.const 0)
    (i32.const 0)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2
    (i32.const 6)
    (i32.const 34962)
    (call $shine::wasm::ptr_i32\28void\20const*\29.3
     (i32.load offset=100
      (local.get $0)
     )
    )
    (i32.mul
     (i32.load offset=92
      (local.get $0)
     )
     (i32.const 6)
    )
    (i32.const 35048)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2
    (i32.const 16)
    (i32.load offset=128
     (local.get $0)
    )
    (i32.const 0)
    (i32.const 0)
    (i32.const 0)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2
    (i32.const 4)
    (i32.load offset=116
     (local.get $0)
    )
    (i32.const 0)
    (i32.const 0)
    (i32.const 0)
   )
   (call $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2
    (i32.const 15)
    (i32.const 4)
    (i32.const 0)
    (i32.const 6)
    (i32.load offset=92
     (local.get $0)
    )
   )
  )
  (call $shine::game::Node::renderTree\28shine::game::RenderContext&\2c\20float\29
   (i32.add
    (local.get $0)
    (i32.const 4)
   )
   (i32.add
    (local.get $0)
    (i32.const 56)
   )
   (local.get $2)
  )
  (local.set $0
   (i32.load
    (local.get $1)
   )
  )
  (call $shine::ui::UIManager::onRender\28int\29
   (call $shine::ui::UIManager::instance\28\29)
   (local.get $0)
  )
 )
 (func $DemoGame::update_vertices\28float\29 (param $0 i32) (param $1 f32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 f32)
  (local $7 f32)
  (local $8 f32)
  (local $9 f32)
  (local $10 f32)
  (local $11 f32)
  (local $12 f32)
  (local $13 i32)
  (local $14 i32)
  (local $15 f32)
  (local $16 f32)
  (local $17 f32)
  (local $18 f32)
  (local $19 f32)
  (local $20 f32)
  (local $21 f32)
  (block $label$1
   (br_if $label$1
    (i32.lt_s
     (i32.load offset=80
      (local.get $0)
     )
     (i32.const 1)
    )
   )
   (br_if $label$1
    (i32.eqz
     (i32.load offset=88
      (local.get $0)
     )
    )
   )
   (local.set $2
    (i32.load offset=8
     (call $shine::engine::Engine::instance\28\29)
    )
   )
   (local.set $3
    (call $shine::engine::Engine::instance\28\29)
   )
   (local.set $4
    (i32.load offset=80
     (local.get $0)
    )
   )
   (local.set $5
    (i32.load offset=12
     (local.get $3)
    )
   )
   (local.set $3
    (i32.const 0)
   )
   (loop $label$2
    (br_if $label$2
     (i32.lt_s
      (i32.mul
       (local.tee $3
        (i32.add
         (local.get $3)
         (i32.const 1)
        )
       )
       (local.get $3)
      )
      (local.get $4)
     )
    )
   )
   (local.set $7
    (select
     (f32.div
      (f32.const 1)
      (local.tee $6
       (f32.div
        (f32.convert_i32_s
         (local.get $2)
        )
        (f32.convert_i32_s
         (local.get $5)
        )
       )
      )
     )
     (f32.const 1)
     (f32.gt
      (f32.abs
       (local.get $6)
      )
      (f32.const 9.999999747378752e-06)
     )
    )
   )
   (local.set $8
    (f32.mul
     (local.get $1)
     (f32.const 0.10000000149011612)
    )
   )
   (local.set $9
    (f32.div
     (f32.const 1)
     (local.tee $6
      (f32.convert_i32_u
       (local.get $3)
      )
     )
    )
   )
   (local.set $10
    (f32.add
     (f32.mul
      (call $shine::math::sin_approx\28float\29
       (local.get $1)
      )
      (f32.const 0.5)
     )
     (f32.const 0.5)
    )
   )
   (local.set $6
    (f32.mul
     (local.tee $11
      (f32.div
       (f32.const 2)
       (local.get $6)
      )
     )
     (f32.const 0.2800000011920929)
    )
   )
   (local.set $12
    (f32.add
     (f32.mul
      (local.get $11)
      (f32.const 0.5)
     )
     (f32.const -1)
    )
   )
   (local.set $4
    (i32.load offset=88
     (local.get $0)
    )
   )
   (local.set $2
    (i32.const 0)
   )
   (local.set $13
    (i32.const 0)
   )
   (loop $label$3
    (br_if $label$1
     (i32.eq
      (local.get $3)
      (local.get $13)
     )
    )
    (local.set $14
     (i32.add
      (local.get $3)
      (local.get $2)
     )
    )
    (local.set $15
     (f32.mul
      (local.get $9)
      (local.tee $1
       (f32.convert_i32_u
        (local.get $13)
       )
      )
     )
    )
    (local.set $16
     (f32.add
      (f32.mul
       (local.get $1)
       (local.get $11)
      )
      (local.get $12)
     )
    )
    (local.set $17
     (f32.mul
      (call $shine::math::sin_approx\28float\29
       (f32.add
        (f32.mul
         (local.get $1)
         (f32.const 0.10000000149011612)
        )
        (local.get $8)
       )
      )
      (f32.const 0.05000000074505806)
     )
    )
    (local.set $1
     (f32.const 0)
    )
    (local.set $5
     (local.get $3)
    )
    (local.set $18
     (local.get $12)
    )
    (local.set $19
     (local.get $8)
    )
    (block $label$4
     (loop $label$5
      (br_if $label$4
       (i32.eqz
        (local.get $5)
       )
      )
      (br_if $label$1
       (i32.ge_s
        (local.get $2)
        (i32.load offset=80
         (local.get $0)
        )
       )
      )
      (local.set $20
       (call $shine::math::cos_approx\28float\29
        (local.get $19)
       )
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 56)
       )
       (local.get $10)
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 52)
       )
       (local.get $15)
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 48)
       )
       (local.get $1)
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 36)
       )
       (local.get $10)
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 32)
       )
       (local.get $15)
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 28)
       )
       (local.get $1)
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 16)
       )
       (local.get $10)
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 12)
       )
       (local.get $15)
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 8)
       )
       (local.get $1)
      )
      (f32.store
       (local.get $4)
       (f32.mul
        (local.get $7)
        (local.tee $21
         (f32.add
          (local.get $17)
          (local.get $18)
         )
        )
       )
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 40)
       )
       (f32.mul
        (local.get $7)
        (f32.add
         (local.get $6)
         (local.get $21)
        )
       )
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 20)
       )
       (f32.mul
        (local.get $7)
        (f32.sub
         (local.get $21)
         (local.get $6)
        )
       )
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 44)
       )
       (local.tee $20
        (f32.sub
         (local.tee $21
          (f32.add
           (f32.mul
            (local.get $20)
            (f32.const 0.05000000074505806)
           )
           (local.get $16)
          )
         )
         (local.get $6)
        )
       )
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 24)
       )
       (local.get $20)
      )
      (f32.store
       (i32.add
        (local.get $4)
        (i32.const 4)
       )
       (f32.add
        (local.get $6)
        (local.get $21)
       )
      )
      (local.set $5
       (i32.add
        (local.get $5)
        (i32.const -1)
       )
      )
      (local.set $2
       (i32.add
        (local.get $2)
        (i32.const 1)
       )
      )
      (local.set $19
       (f32.add
        (local.get $19)
        (f32.const 0.10000000149011612)
       )
      )
      (local.set $1
       (f32.add
        (local.get $9)
        (local.get $1)
       )
      )
      (local.set $18
       (f32.add
        (local.get $11)
        (local.get $18)
       )
      )
      (local.set $4
       (i32.add
        (local.get $4)
        (i32.const 60)
       )
      )
      (br $label$5)
     )
    )
    (local.set $13
     (i32.add
      (local.get $13)
      (i32.const 1)
     )
    )
    (local.set $2
     (local.get $14)
    )
    (br $label$3)
   )
  )
 )
 (func $shine::graphics::cmd_push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29.2 (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32)
  (call $shine::graphics::CommandBuffer::push\28int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\2c\20int\29
   (call $shine::graphics::CommandBuffer::instance\28\29)
   (local.get $0)
   (local.get $1)
   (local.get $2)
   (local.get $3)
   (local.get $4)
   (i32.const 0)
   (i32.const 0)
   (i32.const 0)
  )
 )
 (func $DemoGame::update_instances\28float\29 (param $0 i32) (param $1 f32)
  (local $2 i32)
  (local $3 f32)
  (local $4 f32)
  (local $5 i32)
  (local $6 f32)
  (local $7 f32)
  (local $8 f32)
  (local $9 f32)
  (local $10 f32)
  (local $11 f32)
  (block $label$1
   (br_if $label$1
    (i32.lt_s
     (local.tee $2
      (i32.load offset=92
       (local.get $0)
      )
     )
     (i32.const 1)
    )
   )
   (br_if $label$1
    (i32.eqz
     (local.tee $0
      (i32.load offset=100
       (local.get $0)
      )
     )
    )
   )
   (local.set $3
    (f32.add
     (local.tee $1
      (f32.add
       (local.get $1)
       (f32.const 0)
      )
     )
     (f32.const 4)
    )
   )
   (local.set $4
    (f32.add
     (local.get $1)
     (f32.const 2)
    )
   )
   (local.set $5
    (i32.const 0)
   )
   (loop $label$2
    (br_if $label$1
     (i32.ge_u
      (local.get $5)
      (local.get $2)
     )
    )
    (local.set $6
     (call $shine::math::sin_approx\28float\29
      (local.get $1)
     )
    )
    (local.set $7
     (call $shine::math::sin_approx\28float\29
      (local.get $1)
     )
    )
    (local.set $8
     (call $shine::math::sin_approx\28float\29
      (local.get $1)
     )
    )
    (local.set $9
     (call $shine::math::sin_approx\28float\29
      (local.get $1)
     )
    )
    (local.set $10
     (call $shine::math::sin_approx\28float\29
      (local.get $4)
     )
    )
    (local.set $11
     (call $shine::math::sin_approx\28float\29
      (local.get $3)
     )
    )
    (f32.store
     (i32.add
      (local.get $0)
      (i32.const 12)
     )
     (f32.add
      (f32.mul
       (local.get $9)
       (f32.const 0.5)
      )
      (f32.const 0.5)
     )
    )
    (f32.store
     (i32.add
      (local.get $0)
      (i32.const 4)
     )
     (f32.add
      (f32.mul
       (local.get $7)
       (f32.const 0.36000001430511475)
      )
      (f32.const 0)
     )
    )
    (f32.store
     (local.get $0)
     (f32.add
      (f32.mul
       (local.get $6)
       (f32.const 0.36000001430511475)
      )
      (f32.const 0)
     )
    )
    (f32.store
     (i32.add
      (local.get $0)
      (i32.const 16)
     )
     (f32.add
      (f32.mul
       (local.get $10)
       (f32.const 0.5)
      )
      (f32.const 0.5)
     )
    )
    (f32.store
     (i32.add
      (local.get $0)
      (i32.const 8)
     )
     (f32.add
      (f32.mul
       (f32.add
        (f32.mul
         (local.get $8)
         (f32.const 0.5)
        )
        (f32.const 0.5)
       )
       (f32.const 0.2449999898672104)
      )
      (f32.const 0.5249999761581421)
     )
    )
    (f32.store
     (i32.add
      (local.get $0)
      (i32.const 20)
     )
     (f32.add
      (f32.mul
       (local.get $11)
       (f32.const 0.5)
      )
      (f32.const 0.5)
     )
    )
    (local.set $0
     (i32.add
      (local.get $0)
      (i32.const 24)
     )
    )
    (local.set $5
     (i32.const 1)
    )
    (br $label$2)
   )
  )
 )
 (func $shine::game::Node::renderTree\28shine::game::RenderContext&\2c\20float\29 (param $0 i32) (param $1 i32) (param $2 f32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (block $label$1
   (br_if $label$1
    (i32.ne
     (i32.and
      (i32.load offset=12
       (local.get $0)
      )
      (i32.const 3)
     )
     (i32.const 3)
    )
   )
   (local.set $3
    (i32.const 0)
   )
   (local.set $4
    (i32.const 0)
   )
   (loop $label$2
    (block $label$3
     (br_if $label$3
      (i32.lt_u
       (local.get $4)
       (i32.load offset=40
        (local.get $0)
       )
      )
     )
     (local.set $3
      (i32.const 0)
     )
     (local.set $4
      (i32.const 0)
     )
     (loop $label$4
      (br_if $label$1
       (i32.ge_u
        (local.get $4)
        (i32.load offset=28
         (local.get $0)
        )
       )
      )
      (block $label$5
       (br_if $label$5
        (i32.eqz
         (local.tee $5
          (i32.load
           (i32.add
            (i32.load offset=36
             (local.get $0)
            )
            (local.get $3)
           )
          )
         )
        )
       )
       (call $shine::game::Node::renderTree\28shine::game::RenderContext&\2c\20float\29
        (local.get $5)
        (local.get $1)
        (local.get $2)
       )
      )
      (local.set $3
       (i32.add
        (local.get $3)
        (i32.const 4)
       )
      )
      (local.set $4
       (i32.add
        (local.get $4)
        (i32.const 1)
       )
      )
      (br $label$4)
     )
    )
    (block $label$6
     (br_if $label$6
      (i32.eqz
       (local.tee $5
        (i32.load
         (i32.add
          (i32.load offset=48
           (local.get $0)
          )
          (local.get $3)
         )
        )
       )
      )
     )
     (call $shine::game::Component::renderTree\28shine::game::RenderContext&\2c\20float\29
      (local.get $5)
      (local.get $1)
      (local.get $2)
     )
    )
    (local.set $3
     (i32.add
      (local.get $3)
      (i32.const 4)
     )
    )
    (local.set $4
     (i32.add
      (local.get $4)
      (i32.const 1)
     )
    )
    (br $label$2)
   )
  )
 )
 (func $shine::math::sin_approx\28float\29 (param $0 f32) (result f32)
  (f32.mul
   (local.tee $0
    (call $shine::math::wrap_pi\28float\29
     (local.get $0)
    )
   )
   (f32.add
    (f32.mul
     (f32.mul
      (local.tee $0
       (f32.mul
        (local.get $0)
        (local.get $0)
       )
      )
      (local.get $0)
     )
     (f32.const 0.008333333767950535)
    )
    (f32.add
     (f32.mul
      (local.get $0)
      (f32.const -0.1666666716337204)
     )
     (f32.const 1)
    )
   )
  )
 )
 (func $shine::math::cos_approx\28float\29 (param $0 f32) (result f32)
  (local $1 f32)
  (f32.add
   (f32.add
    (f32.mul
     (local.tee $1
      (f32.mul
       (local.tee $0
        (f32.mul
         (local.tee $0
          (call $shine::math::wrap_pi\28float\29
           (local.get $0)
          )
         )
         (local.get $0)
        )
       )
       (local.get $0)
      )
     )
     (f32.const 0.0416666679084301)
    )
    (f32.add
     (f32.mul
      (local.get $0)
      (f32.const -0.5)
     )
     (f32.const 1)
    )
   )
   (f32.mul
    (f32.mul
     (local.get $1)
     (local.get $0)
    )
    (f32.const -1.3888889225199819e-03)
   )
  )
 )
 (func $DemoGame::onPointer\28shine::engine::Engine&\2c\20float\2c\20float\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 f32) (param $3 f32) (param $4 i32)
  (call $shine::game::Node::pointerTree\28float\2c\20float\2c\20int\29
   (i32.add
    (local.get $0)
    (i32.const 4)
   )
   (local.get $2)
   (local.get $3)
   (local.get $4)
  )
  (local.set $0
   (i32.load offset=12
    (local.get $1)
   )
  )
  (local.set $1
   (i32.load offset=8
    (local.get $1)
   )
  )
  (call $shine::ui::UIManager::onPointer\28float\2c\20float\2c\20int\29
   (call $shine::ui::UIManager::instance\28\29)
   (f32.mul
    (f32.add
     (local.get $2)
     (f32.const 1)
    )
    (f32.mul
     (f32.convert_i32_s
      (local.get $1)
     )
     (f32.const 0.5)
    )
   )
   (f32.mul
    (f32.sub
     (f32.const 1)
     (local.get $3)
    )
    (f32.mul
     (f32.convert_i32_s
      (local.get $0)
     )
     (f32.const 0.5)
    )
   )
   (local.get $4)
  )
 )
 (func $shine::game::Node::pointerTree\28float\2c\20float\2c\20int\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.and
      (i32.load8_u offset=12
       (local.get $0)
      )
      (i32.const 1)
     )
    )
   )
   (local.set $4
    (i32.const 0)
   )
   (local.set $5
    (i32.const 0)
   )
   (loop $label$2
    (block $label$3
     (br_if $label$3
      (i32.lt_u
       (local.get $5)
       (i32.load offset=40
        (local.get $0)
       )
      )
     )
     (local.set $4
      (i32.const 0)
     )
     (local.set $5
      (i32.const 0)
     )
     (loop $label$4
      (br_if $label$1
       (i32.ge_u
        (local.get $5)
        (i32.load offset=28
         (local.get $0)
        )
       )
      )
      (block $label$5
       (br_if $label$5
        (i32.eqz
         (local.tee $6
          (i32.load
           (i32.add
            (i32.load offset=36
             (local.get $0)
            )
            (local.get $4)
           )
          )
         )
        )
       )
       (call $shine::game::Node::pointerTree\28float\2c\20float\2c\20int\29
        (local.get $6)
        (local.get $1)
        (local.get $2)
        (local.get $3)
       )
      )
      (local.set $4
       (i32.add
        (local.get $4)
        (i32.const 4)
       )
      )
      (local.set $5
       (i32.add
        (local.get $5)
        (i32.const 1)
       )
      )
      (br $label$4)
     )
    )
    (block $label$6
     (br_if $label$6
      (i32.eqz
       (local.tee $6
        (i32.load
         (i32.add
          (i32.load offset=48
           (local.get $0)
          )
          (local.get $4)
         )
        )
       )
      )
     )
     (call $shine::game::Component::pointerTree\28float\2c\20float\2c\20int\29
      (local.get $6)
      (local.get $1)
      (local.get $2)
      (local.get $3)
     )
    )
    (local.set $4
     (i32.add
      (local.get $4)
      (i32.const 4)
     )
    )
    (local.set $5
     (i32.add
      (local.get $5)
      (i32.const 1)
     )
    )
    (br $label$2)
   )
  )
 )
 (func $shine::wasm::SArray<float>::reset\28\29 (param $0 i32)
  (local $1 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $1
      (i32.load offset=4
       (local.get $0)
      )
     )
    )
   )
   (call $free
    (local.get $1)
   )
   (i32.store offset=4
    (local.get $0)
    (i32.const 0)
   )
  )
  (i32.store
   (local.get $0)
   (i32.const 0)
  )
 )
 (func $on_tex_loaded (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32)
  (call $shine::graphics::TextureManager::on_loaded\28int\2c\20int\2c\20int\2c\20int\29
   (call $shine::graphics::TextureManager::instance\28\29)
   (local.get $0)
   (local.get $1)
   (local.get $2)
   (local.get $3)
  )
 )
 (func $on_tex_failed (param $0 i32) (param $1 i32)
  (call $shine::graphics::TextureManager::on_failed\28int\29
   (call $shine::graphics::TextureManager::instance\28\29)
   (local.get $0)
  )
 )
 (func $CreateGame\28\29 (result i32)
  (local $0 i32)
  (local.set $0
   (call $operator\20new\28unsigned\20long\29
    (i32.const 144)
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.const 144)
    )
   )
   (memory.fill
    (local.get $0)
    (i32.const 0)
    (i32.const 144)
   )
  )
  (call $DemoGame::DemoGame\28\29
   (local.get $0)
  )
 )
 (func $DemoGame::DemoGame\28\29 (param $0 i32) (result i32)
  (i32.store
   (local.get $0)
   (i32.const 3748)
  )
  (drop
   (call $shine::game::Scene::Scene\28\29
    (i32.add
     (local.get $0)
     (i32.const 4)
    )
   )
  )
  (i64.store offset=136 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=128 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=120 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=112 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=104 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=96 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=88 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=80 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=72 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=64 align=4
   (local.get $0)
   (i64.const 0)
  )
  (i64.store offset=56 align=4
   (local.get $0)
   (i64.const 0)
  )
  (local.get $0)
 )
 (func $shine::game::Scene::Scene\28\29 (param $0 i32) (result i32)
  (call $shine::game::Node::Node\28char\20const*\29
   (local.get $0)
   (i32.const 3704)
  )
 )
 (func $DemoGame::~DemoGame\28\29 (param $0 i32) (result i32)
  (i32.store
   (local.get $0)
   (i32.const 3748)
  )
  (drop
   (call $shine::wasm::SArray<float>::~SArray\28\29
    (i32.add
     (local.get $0)
     (i32.const 96)
    )
   )
  )
  (drop
   (call $shine::wasm::SArray<float>::~SArray\28\29
    (i32.add
     (local.get $0)
     (i32.const 84)
    )
   )
  )
  (drop
   (call $shine::game::Node::~Node\28\29
    (i32.add
     (local.get $0)
     (i32.const 4)
    )
   )
  )
  (local.get $0)
 )
 (func $shine::game::Node::~Node\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (i32.store
   (local.get $0)
   (i32.const 4004)
  )
  (local.set $1
   (i32.add
    (local.get $0)
    (i32.const 40)
   )
  )
  (local.set $2
   (i32.shl
    (i32.load offset=40
     (local.get $0)
    )
    (i32.const 2)
   )
  )
  (local.set $3
   (i32.load offset=48
    (local.get $0)
   )
  )
  (loop $label$1 (result i32)
   (block $label$2
    (br_if $label$2
     (local.get $2)
    )
    (i32.store offset=40
     (local.get $0)
     (i32.const 0)
    )
    (local.set $4
     (i32.add
      (local.get $0)
      (i32.const 28)
     )
    )
    (local.set $5
     (i32.shl
      (i32.load offset=28
       (local.get $0)
      )
      (i32.const 2)
     )
    )
    (local.set $2
     (i32.load offset=36
      (local.get $0)
     )
    )
    (loop $label$3
     (block $label$4
      (br_if $label$4
       (local.get $5)
      )
      (i32.store offset=28
       (local.get $0)
       (i32.const 0)
      )
      (block $label$5
       (br_if $label$5
        (i32.eqz
         (local.tee $5
          (i32.load offset=24
           (local.get $0)
          )
         )
        )
       )
       (call $shine::game::Node::removeChild\28shine::game::Node*\29
        (local.get $5)
        (local.get $0)
       )
      )
      (drop
       (call $shine::wasm::SVector<shine::game::Component*>::~SVector\28\29
        (local.get $1)
       )
      )
      (drop
       (call $shine::wasm::SVector<shine::game::Node*>::~SVector\28\29
        (local.get $4)
       )
      )
      (return
       (call $shine::game::Object::~Object\28\29
        (local.get $0)
       )
      )
     )
     (block $label$6
      (br_if $label$6
       (i32.eqz
        (local.tee $3
         (i32.load
          (local.get $2)
         )
        )
       )
      )
      (i32.store offset=24
       (local.get $3)
       (i32.const 0)
      )
      (call_indirect (type $i32_=>_none)
       (local.get $3)
       (i32.load offset=4
        (i32.load
         (local.get $3)
        )
       )
      )
     )
     (local.set $5
      (i32.add
       (local.get $5)
       (i32.const -4)
      )
     )
     (local.set $2
      (i32.add
       (local.get $2)
       (i32.const 4)
      )
     )
     (br $label$3)
    )
   )
   (block $label$7
    (br_if $label$7
     (i32.eqz
      (local.tee $5
       (i32.load
        (local.get $3)
       )
      )
     )
    )
    (i32.store offset=44
     (local.get $5)
     (i32.const 0)
    )
    (i32.store offset=24
     (local.get $5)
     (i32.const 0)
    )
    (call_indirect (type $i32_=>_none)
     (local.get $5)
     (i32.load offset=4
      (i32.load
       (local.get $5)
      )
     )
    )
   )
   (local.set $2
    (i32.add
     (local.get $2)
     (i32.const -4)
    )
   )
   (local.set $3
    (i32.add
     (local.get $3)
     (i32.const 4)
    )
   )
   (br $label$1)
  )
 )
 (func $DemoGame::~DemoGame\28\29.1 (param $0 i32)
  (call $operator\20delete\28void*\2c\20unsigned\20long\29
   (call $DemoGame::~DemoGame\28\29
    (local.get $0)
   )
   (i32.const 144)
  )
 )
 (func $shine::wasm::SVector<shine::game::Component*>::grow_cap\28unsigned\20int\2c\20unsigned\20int\29 (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local.set $0
   (select
    (local.get $0)
    (i32.const 8)
    (local.get $0)
   )
  )
  (loop $label$1
   (local.set $0
    (i32.shl
     (local.tee $2
      (local.get $0)
     )
     (i32.const 1)
    )
   )
   (br_if $label$1
    (i32.lt_u
     (local.get $2)
     (local.get $1)
    )
   )
  )
  (local.get $2)
 )
 (func $shine::wasm::SVector<shine::game::Component*>::reserve\28unsigned\20int\29 (param $0 i32) (param $1 i32)
  (call $shine::wasm::svector_reserve_impl\28void**\2c\20unsigned\20int*\2c\20unsigned\20int\2c\20unsigned\20int\2c\20unsigned\20int\29
   (i32.add
    (local.get $0)
    (i32.const 8)
   )
   (i32.add
    (local.get $0)
    (i32.const 4)
   )
   (i32.load
    (local.get $0)
   )
   (local.get $1)
   (i32.const 4)
  )
 )
 (func $shine::game::Object::Object\28char\20const*\29 (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (i32.store
   (local.get $0)
   (i32.const 3872)
  )
  (i64.store offset=12 align=4
   (local.get $0)
   (i64.const 31)
  )
  (i32.store offset=8
   (local.get $0)
   (local.get $1)
  )
  (i32.store offset=4
   (local.get $0)
   (local.tee $2
    (i32.load offset=4512
     (i32.const 0)
    )
   )
  )
  (i32.store offset=20
   (local.get $0)
   (local.tee $1
    (i32.load offset=4664
     (i32.const 0)
    )
   )
  )
  (i32.store offset=4512
   (i32.const 0)
   (select
    (local.tee $2
     (i32.add
      (local.get $2)
      (i32.const 1)
     )
    )
    (i32.const 1)
    (i32.gt_u
     (local.get $2)
     (i32.const 1)
    )
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (i32.store offset=16
    (local.get $1)
    (local.get $0)
   )
  )
  (i32.store offset=4664
   (i32.const 0)
   (local.get $0)
  )
  (local.get $0)
 )
 (func $PulseColor::~PulseColor\28\29 (param $0 i32)
  (call $operator\20delete\28void*\2c\20unsigned\20long\29
   (call $shine::game::Component::~Component\28\29
    (local.get $0)
   )
   (i32.const 56)
  )
 )
 (func $shine::game::Component::~Component\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (i32.store
   (local.get $0)
   (i32.const 3828)
  )
  (local.set $1
   (i32.add
    (local.get $0)
    (i32.const 32)
   )
  )
  (local.set $2
   (i32.shl
    (i32.load offset=32
     (local.get $0)
    )
    (i32.const 2)
   )
  )
  (local.set $3
   (i32.load offset=40
    (local.get $0)
   )
  )
  (block $label$1
   (block $label$2
    (loop $label$3
     (block $label$4
      (br_if $label$4
       (local.get $2)
      )
      (i32.store offset=32
       (local.get $0)
       (i32.const 0)
      )
      (call_indirect (type $i32_=>_none)
       (local.get $0)
       (i32.load offset=20
        (i32.load
         (local.get $0)
        )
       )
      )
      (br_if $label$2
       (i32.eqz
        (local.tee $4
         (i32.load offset=44
          (local.get $0)
         )
        )
       )
      )
      (call $shine::game::Component::removeChild\28shine::game::Component*\29
       (local.get $4)
       (local.get $0)
      )
      (br $label$1)
     )
     (block $label$5
      (br_if $label$5
       (i32.eqz
        (local.tee $4
         (i32.load
          (local.get $3)
         )
        )
       )
      )
      (i32.store offset=24
       (local.get $4)
       (i32.const 0)
      )
      (i32.store offset=44
       (local.get $4)
       (i32.const 0)
      )
      (call_indirect (type $i32_=>_none)
       (local.get $4)
       (i32.load offset=4
        (i32.load
         (local.get $4)
        )
       )
      )
     )
     (local.set $2
      (i32.add
       (local.get $2)
       (i32.const -4)
      )
     )
     (local.set $3
      (i32.add
       (local.get $3)
       (i32.const 4)
      )
     )
     (br $label$3)
    )
   )
   (br_if $label$1
    (i32.eqz
     (local.tee $4
      (i32.load offset=24
       (local.get $0)
      )
     )
    )
   )
   (call $shine::game::Node::removeComponent\28shine::game::Component*\29
    (local.get $4)
    (local.get $0)
   )
  )
  (drop
   (call $shine::wasm::SVector<shine::game::Component*>::~SVector\28\29
    (local.get $1)
   )
  )
  (call $shine::game::Object::~Object\28\29
   (local.get $0)
  )
 )
 (func $shine::game::Component::kind\28\29\20const (param $0 i32) (result i32)
  (i32.const 2)
 )
 (func $shine::game::Component::isOwnedByDead\28\29\20const (param $0 i32) (result i32)
  (local $1 i32)
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (local.tee $1
       (i32.load offset=44
        (local.get $0)
       )
      )
     )
    )
    (br_if $label$1
     (i32.ne
      (i32.and
       (i32.load offset=12
        (local.get $1)
       )
       (i32.const 96)
      )
      (i32.const 64)
     )
    )
   )
   (block $label$3
    (br_if $label$3
     (local.tee $0
      (i32.load offset=24
       (local.get $0)
      )
     )
    )
    (return
     (i32.const 0)
    )
   )
   (br_if $label$1
    (i32.and
     (local.tee $0
      (i32.load offset=12
       (local.get $0)
      )
     )
     (i32.const 32)
    )
   )
   (return
    (i32.eqz
     (i32.and
      (local.get $0)
      (i32.const 64)
     )
    )
   )
  )
  (i32.const 1)
 )
 (func $shine::game::Component::onAttach\28\29 (param $0 i32)
 )
 (func $shine::game::Component::onDetach\28\29 (param $0 i32)
 )
 (func $PulseColor::onUpdate\28float\29 (param $0 i32) (param $1 f32)
  (local $2 f32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.load offset=48
      (local.get $0)
     )
    )
   )
   (local.set $2
    (f32.load offset=52
     (local.get $0)
    )
   )
   (local.set $1
    (call $shine::math::sin_approx\28float\29
     (f32.mul
      (local.get $1)
      (f32.const 3)
     )
    )
   )
   (f32.store offset=60
    (local.tee $0
     (i32.load offset=48
      (local.get $0)
     )
    )
    (local.tee $1
     (f32.add
      (local.get $2)
      (f32.mul
       (local.get $1)
       (f32.const 0.20000000298023224)
      )
     )
    )
   )
   (f32.store offset=56
    (local.get $0)
    (local.get $1)
   )
  )
 )
 (func $shine::game::Component::onRender\28shine::game::RenderContext&\2c\20float\29 (param $0 i32) (param $1 i32) (param $2 f32)
 )
 (func $shine::game::Component::onPointer\28float\2c\20float\2c\20int\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 i32)
 )
 (func $shine::game::Component::~Component\28\29.1 (param $0 i32)
  (call $operator\20delete\28void*\2c\20unsigned\20long\29
   (call $shine::game::Component::~Component\28\29
    (local.get $0)
   )
   (i32.const 48)
  )
 )
 (func $shine::game::Component::onUpdate\28float\29 (param $0 i32) (param $1 f32)
 )
 (func $shine::game::Object::~Object\28\29 (param $0 i32) (result i32)
  (i32.store
   (local.get $0)
   (i32.const 3872)
  )
  (call $shine::game::Object::gc_unlink\28\29
   (local.get $0)
  )
  (local.get $0)
 )
 (func $shine::game::Object::gc_unlink\28\29 (param $0 i32)
  (local $1 i32)
  (local $2 i32)
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (local.tee $1
       (i32.load offset=16
        (local.get $0)
       )
      )
     )
    )
    (i32.store offset=20
     (local.get $1)
     (local.tee $2
      (i32.load offset=20
       (local.get $0)
      )
     )
    )
    (br $label$1)
   )
   (local.set $2
    (i32.load offset=20
     (local.get $0)
    )
   )
   (br_if $label$1
    (i32.ne
     (i32.load offset=4664
      (i32.const 0)
     )
     (local.get $0)
    )
   )
   (i32.store offset=4664
    (i32.const 0)
    (local.get $2)
   )
  )
  (block $label$3
   (br_if $label$3
    (i32.eqz
     (local.get $2)
    )
   )
   (i32.store offset=16
    (local.get $2)
    (local.get $1)
   )
  )
  (i64.store offset=16 align=4
   (local.get $0)
   (i64.const 0)
  )
 )
 (func $shine::game::Object::~Object\28\29.1 (param $0 i32)
  (unreachable)
 )
 (func $shine::game::Object::isOwnedByDead\28\29\20const (param $0 i32) (result i32)
  (i32.const 0)
 )
 (func $shine::game::Component::removeChild\28shine::game::Component*\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (global.set $__stack_pointer
   (local.tee $2
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=12
   (local.get $2)
   (local.get $1)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (drop
    (call $shine::wasm::SVector<shine::game::Component*>::erase_first_unordered\28shine::game::Component*\20const&\29
     (i32.add
      (local.get $0)
      (i32.const 32)
     )
     (i32.add
      (local.get $2)
      (i32.const 12)
     )
    )
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $2)
    (i32.const 16)
   )
  )
 )
 (func $shine::wasm::SVector<shine::game::Component*>::erase_first_unordered\28shine::game::Component*\20const&\29 (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local.set $2
   (i32.load
    (local.get $1)
   )
  )
  (local.set $3
   (i32.load offset=8
    (local.get $0)
   )
  )
  (local.set $4
   (i32.load
    (local.get $0)
   )
  )
  (local.set $1
   (i32.const 0)
  )
  (loop $label$1 (result i32)
   (block $label$2
    (block $label$3
     (br_if $label$3
      (i32.eq
       (local.get $4)
       (local.get $1)
      )
     )
     (br_if $label$2
      (i32.ne
       (i32.load
        (local.get $3)
       )
       (local.get $2)
      )
     )
     (local.set $5
      (call $shine::wasm::SVector<shine::game::Component*>::erase_unordered_at\28unsigned\20int\29
       (local.get $0)
       (local.get $1)
      )
     )
    )
    (return
     (i32.and
      (i32.lt_u
       (local.get $1)
       (local.get $4)
      )
      (local.get $5)
     )
    )
   )
   (local.set $3
    (i32.add
     (local.get $3)
     (i32.const 4)
    )
   )
   (local.set $1
    (i32.add
     (local.get $1)
     (i32.const 1)
    )
   )
   (br $label$1)
  )
 )
 (func $shine::game::Node::removeComponent\28shine::game::Component*\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (global.set $__stack_pointer
   (local.tee $2
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=12
   (local.get $2)
   (local.get $1)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (drop
    (call $shine::wasm::SVector<shine::game::Component*>::erase_first_unordered\28shine::game::Component*\20const&\29
     (i32.add
      (local.get $0)
      (i32.const 40)
     )
     (i32.add
      (local.get $2)
      (i32.const 12)
     )
    )
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $2)
    (i32.const 16)
   )
  )
 )
 (func $shine::wasm::SVector<shine::game::Component*>::~SVector\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $1
      (i32.load offset=8
       (local.get $0)
      )
     )
    )
   )
   (call $free
    (local.get $1)
   )
   (i32.store offset=8
    (local.get $0)
    (i32.const 0)
   )
  )
  (i64.store align=4
   (local.get $0)
   (i64.const 0)
  )
  (local.get $0)
 )
 (func $shine::wasm::SVector<shine::game::Component*>::erase_unordered_at\28unsigned\20int\29 (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.ge_u
     (local.get $1)
     (local.tee $2
      (i32.load
       (local.get $0)
      )
     )
    )
   )
   (local.set $3
    (i32.const 0)
   )
   (block $label$2
    (br_if $label$2
     (i32.lt_u
      (local.get $2)
      (i32.const 2)
     )
    )
    (br_if $label$2
     (i32.eq
      (local.get $1)
      (local.tee $3
       (i32.add
        (local.get $2)
        (i32.const -1)
       )
      )
     )
    )
    (i32.store
     (i32.add
      (local.tee $4
       (i32.load offset=8
        (local.get $0)
       )
      )
      (i32.shl
       (local.get $1)
       (i32.const 2)
      )
     )
     (i32.load
      (i32.add
       (local.get $4)
       (i32.shl
        (local.get $3)
        (i32.const 2)
       )
      )
     )
    )
   )
   (i32.store
    (local.get $0)
    (local.get $3)
   )
  )
  (i32.lt_u
   (local.get $1)
   (local.get $2)
  )
 )
 (func $shine::ui::Button::~Button\28\29 (param $0 i32)
  (call $operator\20delete\28void*\2c\20unsigned\20long\29
   (local.get $0)
   (i32.const 112)
  )
 )
 (func $shine::ui::Button::init\28\29 (param $0 i32)
  (local $1 i32)
  (i32.store8 offset=76
   (local.get $0)
   (i32.const 1)
  )
  (local.set $1
   (call $operator\20new\28unsigned\20long\29
    (i32.const 124)
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.const 124)
    )
   )
   (memory.copy
    (local.get $1)
    (i32.const 4516)
    (i32.const 124)
   )
  )
  (i32.store offset=88
   (local.get $0)
   (local.get $1)
  )
  (i32.store offset=84
   (local.get $0)
   (local.get $1)
  )
 )
 (func $shine::ui::Element::hit\28float\2c\20float\29\20const (param $0 i32) (param $1 f32) (param $2 f32) (result i32)
  (local $3 i32)
  (local $4 f32)
  (local $5 f32)
  (local.set $3
   (i32.const 0)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (f32.ge
      (local.get $1)
      (f32.sub
       (local.tee $4
        (f32.load offset=52
         (local.get $0)
        )
       )
       (local.tee $5
        (f32.mul
         (f32.load offset=60
          (local.get $0)
         )
         (f32.const 0.5)
        )
       )
      )
     )
    )
   )
   (br_if $label$1
    (i32.eqz
     (f32.le
      (local.get $1)
      (f32.add
       (local.get $5)
       (local.get $4)
      )
     )
    )
   )
   (br_if $label$1
    (i32.eqz
     (f32.ge
      (local.get $2)
      (f32.sub
       (local.tee $1
        (f32.load offset=56
         (local.get $0)
        )
       )
       (local.tee $4
        (f32.mul
         (f32.load offset=64
          (local.get $0)
         )
         (f32.const 0.5)
        )
       )
      )
     )
    )
   )
   (local.set $3
    (f32.le
     (local.get $2)
     (f32.add
      (local.get $4)
      (local.get $1)
     )
    )
   )
  )
  (local.get $3)
 )
 (func $shine::ui::Button::pointer\28float\2c\20float\2c\20int\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 i32)
  (local $4 i32)
  (local $5 i32)
  (block $label$1
   (br_if $label$1
    (i32.xor
     (local.tee $4
      (call_indirect (type $i32_f32_f32_=>_i32)
       (local.get $0)
       (local.get $1)
       (local.get $2)
       (i32.load offset=12
        (i32.load
         (local.get $0)
        )
       )
      )
     )
     (i32.eqz
      (i32.and
       (local.tee $5
        (i32.load8_u offset=76
         (local.get $0)
        )
       )
       (i32.const 2)
      )
     )
    )
   )
   (i32.store8 offset=76
    (local.get $0)
    (i32.or
     (i32.and
      (local.get $5)
      (i32.const 253)
     )
     (select
      (i32.const 2)
      (i32.const 0)
      (local.get $4)
     )
    )
   )
   (i32.store offset=88
    (local.get $0)
    (i32.add
     (i32.load offset=84
      (local.get $0)
     )
     (select
      (i32.const 16)
      (i32.const 0)
      (local.get $4)
     )
    )
   )
   (block $label$2
    (block $label$3
     (br_if $label$3
      (i32.eqz
       (local.get $4)
      )
     )
     (br_if $label$2
      (local.tee $5
       (i32.load offset=96
        (local.get $0)
       )
      )
     )
     (br $label$1)
    )
    (br_if $label$1
     (i32.eqz
      (local.tee $5
       (i32.load offset=100
        (local.get $0)
       )
      )
     )
    )
   )
   (call_indirect (type $i32_=>_none)
    (local.get $0)
    (local.get $5)
   )
  )
  (local.set $5
   (i32.load offset=108
    (local.get $0)
   )
  )
  (block $label$4
   (block $label$5
    (block $label$6
     (block $label$7
      (br_if $label$7
       (i32.eqz
        (local.get $3)
       )
      )
      (local.set $3
       (i32.const 1)
      )
      (br_if $label$5
       (i32.or
        (i32.xor
         (local.get $4)
         (i32.const 1)
        )
        (i32.ne
         (local.get $5)
         (i32.const 0)
        )
       )
      )
      (local.set $4
       (i32.or
        (i32.load8_u offset=76
         (local.get $0)
        )
        (i32.const 4)
       )
      )
      (local.set $3
       (i32.const 1)
      )
      (br $label$6)
     )
     (br_if $label$4
      (i32.eqz
       (local.get $5)
      )
     )
     (block $label$8
      (br_if $label$8
       (i32.or
        (i32.xor
         (local.get $4)
         (i32.const 1)
        )
        (i32.eqz
         (i32.and
          (local.tee $4
           (i32.load8_u offset=76
            (local.get $0)
           )
          )
          (i32.const 4)
         )
        )
       )
      )
      (i32.store offset=104
       (local.get $0)
       (i32.const 1)
      )
      (i32.store offset=88
       (local.get $0)
       (i32.add
        (i32.load offset=84
         (local.get $0)
        )
        (i32.const 16)
       )
      )
      (br_if $label$8
       (i32.eqz
        (local.tee $3
         (i32.load offset=92
          (local.get $0)
         )
        )
       )
      )
      (call_indirect (type $i32_=>_none)
       (local.get $0)
       (local.get $3)
      )
      (local.set $4
       (i32.load8_u offset=76
        (local.get $0)
       )
      )
     )
     (local.set $4
      (i32.and
       (local.get $4)
       (i32.const 251)
      )
     )
     (local.set $3
      (i32.const 0)
     )
    )
    (i32.store8 offset=76
     (local.get $0)
     (local.get $4)
    )
   )
   (i32.store offset=108
    (local.get $0)
    (local.get $3)
   )
  )
 )
 (func $shine::ui::Button::onResize\28int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32)
  (call $shine::ui::Element::onResize\28int\2c\20int\29
   (local.get $0)
   (local.get $1)
   (local.get $2)
  )
 )
 (func $shine::ui::Element::onResize\28int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32)
  (local $3 f32)
  (local $4 f32)
  (local $5 f32)
  (local $6 f32)
  (local $7 f32)
  (i32.store offset=72
   (local.get $0)
   (local.tee $2
    (select
     (local.get $2)
     (i32.const 1)
     (i32.gt_s
      (local.get $2)
      (i32.const 1)
     )
    )
   )
  )
  (i32.store offset=68
   (local.get $0)
   (local.tee $1
    (select
     (local.get $1)
     (i32.const 1)
     (i32.gt_s
      (local.get $1)
      (i32.const 1)
     )
    )
   )
  )
  (local.set $5
   (f32.add
    (f32.mul
     (local.tee $3
      (f32.load offset=4
       (local.get $0)
      )
     )
     (local.tee $4
      (f32.convert_i32_u
       (local.get $1)
      )
     )
    )
    (f32.load offset=28
     (local.get $0)
    )
   )
  )
  (local.set $6
   (f32.convert_i32_u
    (local.get $2)
   )
  )
  (block $label$1
   (block $label$2
    (br_if $label$2
     (f32.eq
      (local.get $3)
      (local.tee $7
       (f32.load offset=12
        (local.get $0)
       )
      )
     )
    )
    (local.set $3
     (f32.sub
      (f32.sub
       (f32.mul
        (local.get $7)
        (local.get $4)
       )
       (f32.load offset=36
        (local.get $0)
       )
      )
      (local.get $5)
     )
    )
    (br $label$1)
   )
   (local.set $3
    (select
     (f32.mul
      (local.tee $3
       (f32.load offset=44
        (local.get $0)
       )
      )
      (local.get $4)
     )
     (f32.load offset=36
      (local.get $0)
     )
     (f32.gt
      (local.get $3)
      (f32.const 0)
     )
    )
   )
  )
  (f32.store offset=60
   (local.get $0)
   (local.tee $3
    (select
     (f32.const 0)
     (local.get $3)
     (f32.lt
      (local.get $3)
      (f32.const 0)
     )
    )
   )
  )
  (f32.store offset=52
   (local.get $0)
   (f32.add
    (f32.mul
     (local.get $3)
     (f32.sub
      (f32.const 0.5)
      (f32.load offset=20
       (local.get $0)
      )
     )
    )
    (local.get $5)
   )
  )
  (local.set $3
   (f32.add
    (f32.mul
     (local.tee $5
      (f32.load offset=8
       (local.get $0)
      )
     )
     (local.get $6)
    )
    (f32.load offset=32
     (local.get $0)
    )
   )
  )
  (block $label$3
   (block $label$4
    (br_if $label$4
     (f32.eq
      (local.get $5)
      (local.tee $4
       (f32.load offset=16
        (local.get $0)
       )
      )
     )
    )
    (local.set $6
     (f32.sub
      (f32.sub
       (f32.mul
        (local.get $4)
        (local.get $6)
       )
       (f32.load offset=40
        (local.get $0)
       )
      )
      (local.get $3)
     )
    )
    (br $label$3)
   )
   (local.set $6
    (select
     (f32.mul
      (local.tee $5
       (f32.load offset=48
        (local.get $0)
       )
      )
      (local.get $6)
     )
     (f32.load offset=40
      (local.get $0)
     )
     (f32.gt
      (local.get $5)
      (f32.const 0)
     )
    )
   )
  )
  (f32.store offset=64
   (local.get $0)
   (local.tee $6
    (select
     (f32.const 0)
     (local.get $6)
     (f32.lt
      (local.get $6)
      (f32.const 0)
     )
    )
   )
  )
  (f32.store offset=56
   (local.get $0)
   (f32.add
    (f32.mul
     (local.get $6)
     (f32.sub
      (f32.const 0.5)
      (f32.load offset=24
       (local.get $0)
      )
     )
    )
    (local.get $3)
   )
  )
 )
 (func $shine::ui::Button::render\28int\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 f32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.and
      (i32.load8_u offset=76
       (local.get $0)
      )
      (i32.const 1)
     )
    )
   )
   (local.set $3
    (f32.load offset=52
     (local.tee $2
      (i32.load offset=84
       (local.get $0)
      )
     )
    )
   )
   (call $shine::graphics::Renderer2D::drawRoundRect\28float\2c\20float\2c\20float\2c\20float\2c\20float\2c\20shine::graphics::Renderer2D::Color4\20const&\2c\20int\2c\20shine::graphics::Renderer2D::Color4\20const&\2c\20float\2c\20shine::graphics::Renderer2D::Color4\20const&\2c\20float\2c\20float\2c\20float\2c\20float\2c\20shine::graphics::Renderer2D::Color4\20const&\29
    (call $shine::graphics::Renderer2D::instance\28\29)
    (f32.load offset=52
     (local.get $0)
    )
    (f32.load offset=56
     (local.get $0)
    )
    (f32.load offset=60
     (local.get $0)
    )
    (f32.load offset=64
     (local.get $0)
    )
    (f32.load offset=48
     (local.get $2)
    )
    (i32.load offset=88
     (local.get $0)
    )
    (i32.load offset=104
     (local.get $2)
    )
    (i32.add
     (local.get $2)
     (i32.const 108)
    )
    (local.get $3)
    (i32.add
     (local.get $2)
     (i32.const 56)
    )
    (f32.load offset=72
     (local.get $2)
    )
    (f32.load offset=76
     (local.get $2)
    )
    (f32.load offset=80
     (local.get $2)
    )
    (f32.load offset=84
     (local.get $2)
    )
    (i32.add
     (local.get $2)
     (i32.const 88)
    )
   )
  )
 )
 (func $shine::ui::Element::~Element\28\29 (param $0 i32)
  (call $operator\20delete\28void*\2c\20unsigned\20long\29
   (local.get $0)
   (i32.const 84)
  )
 )
 (func $shine::ui::Element::init\28\29 (param $0 i32)
  (i32.store8 offset=76
   (local.get $0)
   (i32.const 1)
  )
 )
 (func $shine::ui::Element::pointer\28float\2c\20float\2c\20int\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 i32)
  (local $4 i32)
  (local.set $4
   (call_indirect (type $i32_f32_f32_=>_i32)
    (local.get $0)
    (local.get $1)
    (local.get $2)
    (i32.load offset=12
     (i32.load
      (local.get $0)
     )
    )
   )
  )
  (i32.store8 offset=76
   (local.get $0)
   (select
    (local.tee $4
     (i32.or
      (i32.and
       (i32.load8_u offset=76
        (local.get $0)
       )
       (i32.const -3)
      )
      (select
       (i32.const 2)
       (i32.const 0)
       (local.get $4)
      )
     )
    )
    (i32.and
     (local.get $4)
     (i32.const 251)
    )
    (local.get $3)
   )
  )
 )
 (func $shine::ui::Element::render\28int\29 (param $0 i32) (param $1 i32)
 )
 (func $shine::ui::Element::~Element\28\29.1 (param $0 i32) (result i32)
  (local.get $0)
 )
 (func $shine::ui::Image::~Image\28\29 (param $0 i32)
  (call $operator\20delete\28void*\2c\20unsigned\20long\29
   (local.get $0)
   (i32.const 96)
  )
 )
 (func $shine::ui::Image::onResize\28int\2c\20int\29 (param $0 i32) (param $1 i32) (param $2 i32)
  (call $shine::ui::Element::onResize\28int\2c\20int\29
   (local.get $0)
   (local.get $1)
   (local.get $2)
  )
 )
 (func $shine::ui::Image::render\28int\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $2
      (i32.load offset=84
       (local.get $0)
      )
     )
    )
   )
   (call $ui_draw_rect_uv
    (local.get $1)
    (f32.load offset=52
     (local.get $0)
    )
    (f32.load offset=56
     (local.get $0)
    )
    (f32.load offset=60
     (local.get $0)
    )
    (f32.load offset=64
     (local.get $0)
    )
    (local.get $2)
   )
  )
 )
 (func $shine::game::Component::update\28float\29 (param $0 i32) (param $1 f32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.and
      (local.tee $2
       (i32.load offset=12
        (local.get $0)
       )
      )
      (i32.const 1)
     )
    )
   )
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (i32.and
       (local.get $2)
       (i32.const 4)
      )
     )
    )
    (call_indirect (type $i32_f32_=>_none)
     (local.get $0)
     (local.get $1)
     (i32.load offset=24
      (i32.load
       (local.get $0)
      )
     )
    )
   )
   (local.set $3
    (i32.const 0)
   )
   (local.set $2
    (i32.const 0)
   )
   (loop $label$3
    (br_if $label$1
     (i32.ge_u
      (local.get $2)
      (i32.load offset=32
       (local.get $0)
      )
     )
    )
    (block $label$4
     (br_if $label$4
      (i32.eqz
       (local.tee $4
        (i32.load
         (i32.add
          (i32.load offset=40
           (local.get $0)
          )
          (local.get $3)
         )
        )
       )
      )
     )
     (call $shine::game::Component::update\28float\29
      (local.get $4)
      (local.get $1)
     )
    )
    (local.set $3
     (i32.add
      (local.get $3)
      (i32.const 4)
     )
    )
    (local.set $2
     (i32.add
      (local.get $2)
      (i32.const 1)
     )
    )
    (br $label$3)
   )
  )
 )
 (func $shine::game::Component::markTree\28\29 (param $0 i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (i32.store offset=12
   (local.get $0)
   (i32.or
    (i32.load offset=12
     (local.get $0)
    )
    (i32.const 64)
   )
  )
  (local.set $1
   (i32.const 0)
  )
  (local.set $2
   (i32.const 0)
  )
  (block $label$1
   (loop $label$2
    (br_if $label$1
     (i32.ge_u
      (local.get $2)
      (i32.load offset=32
       (local.get $0)
      )
     )
    )
    (block $label$3
     (br_if $label$3
      (i32.eqz
       (local.tee $3
        (i32.load
         (i32.add
          (i32.load offset=40
           (local.get $0)
          )
          (local.get $1)
         )
        )
       )
      )
     )
     (call $shine::game::Component::markTree\28\29
      (local.get $3)
     )
    )
    (local.set $1
     (i32.add
      (local.get $1)
      (i32.const 4)
     )
    )
    (local.set $2
     (i32.add
      (local.get $2)
      (i32.const 1)
     )
    )
    (br $label$2)
   )
  )
 )
 (func $shine::game::Component::renderTree\28shine::game::RenderContext&\2c\20float\29 (param $0 i32) (param $1 i32) (param $2 f32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.and
      (local.tee $3
       (i32.load offset=12
        (local.get $0)
       )
      )
      (i32.const 1)
     )
    )
   )
   (block $label$2
    (br_if $label$2
     (i32.ne
      (i32.and
       (local.get $3)
       (i32.const 10)
      )
      (i32.const 10)
     )
    )
    (call_indirect (type $i32_i32_f32_=>_none)
     (local.get $0)
     (local.get $1)
     (local.get $2)
     (i32.load offset=28
      (i32.load
       (local.get $0)
      )
     )
    )
   )
   (local.set $4
    (i32.const 0)
   )
   (local.set $3
    (i32.const 0)
   )
   (loop $label$3
    (br_if $label$1
     (i32.ge_u
      (local.get $3)
      (i32.load offset=32
       (local.get $0)
      )
     )
    )
    (block $label$4
     (br_if $label$4
      (i32.eqz
       (local.tee $5
        (i32.load
         (i32.add
          (i32.load offset=40
           (local.get $0)
          )
          (local.get $4)
         )
        )
       )
      )
     )
     (call $shine::game::Component::renderTree\28shine::game::RenderContext&\2c\20float\29
      (local.get $5)
      (local.get $1)
      (local.get $2)
     )
    )
    (local.set $4
     (i32.add
      (local.get $4)
      (i32.const 4)
     )
    )
    (local.set $3
     (i32.add
      (local.get $3)
      (i32.const 1)
     )
    )
    (br $label$3)
   )
  )
 )
 (func $shine::game::Component::pointerTree\28float\2c\20float\2c\20int\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (i32.and
      (local.tee $4
       (i32.load offset=12
        (local.get $0)
       )
      )
      (i32.const 1)
     )
    )
   )
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (i32.and
       (local.get $4)
       (i32.const 16)
      )
     )
    )
    (call_indirect (type $i32_f32_f32_i32_=>_none)
     (local.get $0)
     (local.get $1)
     (local.get $2)
     (local.get $3)
     (i32.load offset=32
      (i32.load
       (local.get $0)
      )
     )
    )
   )
   (local.set $5
    (i32.const 0)
   )
   (local.set $4
    (i32.const 0)
   )
   (loop $label$3
    (br_if $label$1
     (i32.ge_u
      (local.get $4)
      (i32.load offset=32
       (local.get $0)
      )
     )
    )
    (block $label$4
     (br_if $label$4
      (i32.eqz
       (local.tee $6
        (i32.load
         (i32.add
          (i32.load offset=40
           (local.get $0)
          )
          (local.get $5)
         )
        )
       )
      )
     )
     (call $shine::game::Component::pointerTree\28float\2c\20float\2c\20int\29
      (local.get $6)
      (local.get $1)
      (local.get $2)
      (local.get $3)
     )
    )
    (local.set $5
     (i32.add
      (local.get $5)
      (i32.const 4)
     )
    )
    (local.set $4
     (i32.add
      (local.get $4)
      (i32.const 1)
     )
    )
    (br $label$3)
   )
  )
 )
 (func $shine::math::wrap_pi\28float\29 (param $0 f32) (result f32)
  (loop $label$1 (result f32)
   (block $label$2
    (br_if $label$2
     (f32.gt
      (local.get $0)
      (f32.const 3.1415927410125732)
     )
    )
    (block $label$3
     (loop $label$4
      (br_if $label$3
       (i32.eqz
        (f32.lt
         (local.get $0)
         (f32.const -3.1415927410125732)
        )
       )
      )
      (local.set $0
       (f32.add
        (local.get $0)
        (f32.const 6.2831854820251465)
       )
      )
      (br $label$4)
     )
    )
    (return
     (local.get $0)
    )
   )
   (local.set $0
    (f32.add
     (local.get $0)
     (f32.const -6.2831854820251465)
    )
   )
   (br $label$1)
  )
 )
 (func $shine::game::Node::removeChild\28shine::game::Node*\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (global.set $__stack_pointer
   (local.tee $2
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=12
   (local.get $2)
   (local.get $1)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (drop
    (call $shine::wasm::SVector<shine::game::Node*>::erase_first_unordered\28shine::game::Node*\20const&\29
     (i32.add
      (local.get $0)
      (i32.const 28)
     )
     (i32.add
      (local.get $2)
      (i32.const 12)
     )
    )
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $2)
    (i32.const 16)
   )
  )
 )
 (func $shine::wasm::SVector<shine::game::Node*>::~SVector\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $1
      (i32.load offset=8
       (local.get $0)
      )
     )
    )
   )
   (call $free
    (local.get $1)
   )
   (i32.store offset=8
    (local.get $0)
    (i32.const 0)
   )
  )
  (i64.store align=4
   (local.get $0)
   (i64.const 0)
  )
  (local.get $0)
 )
 (func $shine::game::Node::~Node\28\29.1 (param $0 i32)
  (call $operator\20delete\28void*\2c\20unsigned\20long\29
   (call $shine::game::Node::~Node\28\29
    (local.get $0)
   )
   (i32.const 52)
  )
 )
 (func $shine::game::Node::kind\28\29\20const (param $0 i32) (result i32)
  (i32.const 1)
 )
 (func $shine::game::Node::isOwnedByDead\28\29\20const (param $0 i32) (result i32)
  (block $label$1
   (br_if $label$1
    (local.tee $0
     (i32.load offset=24
      (local.get $0)
     )
    )
   )
   (return
    (i32.const 0)
   )
  )
  (block $label$2
   (br_if $label$2
    (i32.eqz
     (i32.and
      (local.tee $0
       (i32.load offset=12
        (local.get $0)
       )
      )
      (i32.const 32)
     )
    )
   )
   (return
    (i32.const 1)
   )
  )
  (i32.eqz
   (i32.and
    (local.get $0)
    (i32.const 64)
   )
  )
 )
 (func $shine::wasm::SVector<shine::game::Node*>::erase_first_unordered\28shine::game::Node*\20const&\29 (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local.set $2
   (i32.load
    (local.get $1)
   )
  )
  (local.set $3
   (i32.load offset=8
    (local.get $0)
   )
  )
  (local.set $4
   (i32.load
    (local.get $0)
   )
  )
  (local.set $1
   (i32.const 0)
  )
  (loop $label$1 (result i32)
   (block $label$2
    (block $label$3
     (br_if $label$3
      (i32.eq
       (local.get $4)
       (local.get $1)
      )
     )
     (br_if $label$2
      (i32.ne
       (i32.load
        (local.get $3)
       )
       (local.get $2)
      )
     )
     (local.set $5
      (call $shine::wasm::SVector<shine::game::Node*>::erase_unordered_at\28unsigned\20int\29
       (local.get $0)
       (local.get $1)
      )
     )
    )
    (return
     (i32.and
      (i32.lt_u
       (local.get $1)
       (local.get $4)
      )
      (local.get $5)
     )
    )
   )
   (local.set $3
    (i32.add
     (local.get $3)
     (i32.const 4)
    )
   )
   (local.set $1
    (i32.add
     (local.get $1)
     (i32.const 1)
    )
   )
   (br $label$1)
  )
 )
 (func $shine::wasm::SVector<shine::game::Node*>::erase_unordered_at\28unsigned\20int\29 (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.ge_u
     (local.get $1)
     (local.tee $2
      (i32.load
       (local.get $0)
      )
     )
    )
   )
   (local.set $3
    (i32.const 0)
   )
   (block $label$2
    (br_if $label$2
     (i32.lt_u
      (local.get $2)
      (i32.const 2)
     )
    )
    (br_if $label$2
     (i32.eq
      (local.get $1)
      (local.tee $3
       (i32.add
        (local.get $2)
        (i32.const -1)
       )
      )
     )
    )
    (i32.store
     (i32.add
      (local.tee $4
       (i32.load offset=8
        (local.get $0)
       )
      )
      (i32.shl
       (local.get $1)
       (i32.const 2)
      )
     )
     (i32.load
      (i32.add
       (local.get $4)
       (i32.shl
        (local.get $3)
        (i32.const 2)
       )
      )
     )
    )
   )
   (i32.store
    (local.get $0)
    (local.get $3)
   )
  )
  (i32.lt_u
   (local.get $1)
   (local.get $2)
  )
 )
 (func $shine::wasm::SVector<shine::game::Node*>::push_back\28shine::game::Node*\20const&\29 (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$1
   (br_if $label$1
    (i32.le_u
     (local.tee $3
      (i32.add
       (local.tee $2
        (i32.load
         (local.get $0)
        )
       )
       (i32.const 1)
      )
     )
     (local.tee $4
      (i32.load offset=4
       (local.get $0)
      )
     )
    )
   )
   (call $shine::wasm::SVector<shine::game::Node*>::reserve\28unsigned\20int\29
    (local.get $0)
    (call $shine::wasm::SVector<shine::game::Node*>::grow_cap\28unsigned\20int\2c\20unsigned\20int\29
     (local.get $4)
     (local.get $3)
    )
   )
   (local.set $2
    (i32.load
     (local.get $0)
    )
   )
  )
  (i32.store
   (local.get $0)
   (local.get $3)
  )
  (i32.store
   (i32.add
    (i32.load offset=8
     (local.get $0)
    )
    (i32.shl
     (local.get $2)
     (i32.const 2)
    )
   )
   (i32.load
    (local.get $1)
   )
  )
 )
 (func $shine::wasm::SVector<shine::game::Node*>::grow_cap\28unsigned\20int\2c\20unsigned\20int\29 (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local.set $0
   (select
    (local.get $0)
    (i32.const 8)
    (local.get $0)
   )
  )
  (loop $label$1
   (local.set $0
    (i32.shl
     (local.tee $2
      (local.get $0)
     )
     (i32.const 1)
    )
   )
   (br_if $label$1
    (i32.lt_u
     (local.get $2)
     (local.get $1)
    )
   )
  )
  (local.get $2)
 )
 (func $shine::wasm::SVector<shine::game::Node*>::reserve\28unsigned\20int\29 (param $0 i32) (param $1 i32)
  (call $shine::wasm::svector_reserve_impl\28void**\2c\20unsigned\20int*\2c\20unsigned\20int\2c\20unsigned\20int\2c\20unsigned\20int\29
   (i32.add
    (local.get $0)
    (i32.const 8)
   )
   (i32.add
    (local.get $0)
    (i32.const 4)
   )
   (i32.load
    (local.get $0)
   )
   (local.get $1)
   (i32.const 4)
  )
 )
 (func $shine::game::Transform::~Transform\28\29 (param $0 i32)
  (call $operator\20delete\28void*\2c\20unsigned\20long\29
   (call $shine::game::Component::~Component\28\29
    (local.get $0)
   )
   (i32.const 64)
  )
 )
 (func $shine::game::SpriteRenderer::~SpriteRenderer\28\29 (param $0 i32)
  (call $operator\20delete\28void*\2c\20unsigned\20long\29
   (call $shine::game::Component::~Component\28\29
    (local.get $0)
   )
   (i32.const 64)
  )
 )
 (func $shine::game::SpriteRenderer::onRender\28shine::game::RenderContext&\2c\20float\29 (param $0 i32) (param $1 i32) (param $2 f32)
  (local $3 i32)
  (local $4 i32)
  (local $5 f32)
  (local $6 f32)
  (global.set $__stack_pointer
   (local.tee $3
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.tee $4
      (i32.load offset=24
       (local.get $0)
      )
     )
    )
   )
   (br_if $label$1
    (i32.eqz
     (local.tee $4
      (call $shine::game::Transform*\20shine::game::Node::getComponent<shine::game::Transform>\28\29
       (local.get $4)
      )
     )
    )
   )
   (call $shine::game::Transform::worldXY\28float&\2c\20float&\29\20const
    (local.get $4)
    (i32.add
     (local.get $3)
     (i32.const 12)
    )
    (i32.add
     (local.get $3)
     (i32.const 8)
    )
   )
   (local.set $5
    (f32.load offset=60
     (local.get $4)
    )
   )
   (local.set $6
    (f32.load offset=56
     (local.get $4)
    )
   )
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (local.tee $4
       (i32.load offset=48
        (local.get $0)
       )
      )
     )
    )
    (br_if $label$1
     (i32.eqz
      (local.tee $0
       (i32.load offset=8
        (local.get $1)
       )
      )
     )
    )
    (call_indirect (type $i32_i32_f32_f32_f32_f32_=>_none)
     (i32.load
      (local.get $1)
     )
     (local.get $4)
     (f32.load offset=12
      (local.get $3)
     )
     (f32.load offset=8
      (local.get $3)
     )
     (local.get $6)
     (local.get $5)
     (local.get $0)
    )
    (br $label$1)
   )
   (br_if $label$1
    (i32.eqz
     (local.tee $4
      (i32.load offset=4
       (local.get $1)
      )
     )
    )
   )
   (call_indirect (type $i32_f32_f32_f32_f32_f32_f32_f32_=>_none)
    (i32.load
     (local.get $1)
    )
    (f32.load offset=12
     (local.get $3)
    )
    (f32.load offset=8
     (local.get $3)
    )
    (local.get $6)
    (local.get $5)
    (f32.load offset=52
     (local.get $0)
    )
    (f32.load offset=56
     (local.get $0)
    )
    (f32.load offset=60
     (local.get $0)
    )
    (local.get $4)
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $3)
    (i32.const 16)
   )
  )
 )
 (func $shine::game::Transform*\20shine::game::Node::getComponent<shine::game::Transform>\28\29 (param $0 i32) (result i32)
  (local $1 i32)
  (local $2 i32)
  (local.set $1
   (i32.load offset=48
    (local.get $0)
   )
  )
  (local.set $0
   (i32.load offset=40
    (local.get $0)
   )
  )
  (block $label$1
   (loop $label$2
    (block $label$3
     (br_if $label$3
      (local.get $0)
     )
     (local.set $2
      (i32.const 0)
     )
     (br $label$1)
    )
    (block $label$4
     (br_if $label$4
      (i32.eqz
       (local.tee $2
        (i32.load
         (local.get $1)
        )
       )
      )
     )
     (br_if $label$1
      (i32.eq
       (i32.load offset=28
        (local.get $2)
       )
       (i32.const 37596)
      )
     )
    )
    (local.set $1
     (i32.add
      (local.get $1)
      (i32.const 4)
     )
    )
    (local.set $0
     (i32.add
      (local.get $0)
      (i32.const -1)
     )
    )
    (br $label$2)
   )
  )
  (local.get $2)
 )
 (func $shine::game::Transform::worldXY\28float&\2c\20float&\29\20const (param $0 i32) (param $1 i32) (param $2 i32)
  (local $3 i32)
  (f32.store
   (local.get $1)
   (f32.load offset=48
    (local.get $0)
   )
  )
  (f32.store
   (local.get $2)
   (f32.load offset=52
    (local.get $0)
   )
  )
  (block $label$1
   (block $label$2
    (block $label$3
     (br_if $label$3
      (local.tee $0
       (i32.load offset=24
        (local.get $0)
       )
      )
     )
     (local.set $0
      (i32.const 0)
     )
     (br $label$2)
    )
    (local.set $3
     (i32.const 0)
    )
    (br $label$1)
   )
   (local.set $3
    (i32.const 1)
   )
  )
  (loop $label$4
   (block $label$5
    (block $label$6
     (br_table $label$6 $label$5 $label$5
      (local.get $3)
     )
    )
    (local.set $0
     (i32.load offset=24
      (local.get $0)
     )
    )
    (local.set $3
     (i32.const 1)
    )
    (br $label$4)
   )
   (block $label$7
    (block $label$8
     (br_if $label$8
      (i32.eqz
       (local.get $0)
      )
     )
     (br_if $label$7
      (i32.eqz
       (local.tee $3
        (call $shine::game::Transform*\20shine::game::Node::getComponent<shine::game::Transform>\28\29
         (local.get $0)
        )
       )
      )
     )
     (f32.store
      (local.get $1)
      (f32.add
       (f32.load offset=48
        (local.get $3)
       )
       (f32.load
        (local.get $1)
       )
      )
     )
     (f32.store
      (local.get $2)
      (f32.add
       (f32.load offset=52
        (local.get $3)
       )
       (f32.load
        (local.get $2)
       )
      )
     )
     (br $label$7)
    )
    (return)
   )
   (local.set $3
    (i32.const 0)
   )
   (br $label$4)
  )
 )
 (func $KillOnClick::~KillOnClick\28\29 (param $0 i32)
  (call $operator\20delete\28void*\2c\20unsigned\20long\29
   (call $shine::game::Component::~Component\28\29
    (local.get $0)
   )
   (i32.const 48)
  )
 )
 (func $KillOnClick::onPointer\28float\2c\20float\2c\20int\29 (param $0 i32) (param $1 f32) (param $2 f32) (param $3 i32)
  (local $4 i32)
  (local $5 f32)
  (local $6 f32)
  (global.set $__stack_pointer
   (local.tee $4
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $3)
    )
   )
   (br_if $label$1
    (i32.eqz
     (local.tee $3
      (i32.load offset=24
       (local.get $0)
      )
     )
    )
   )
   (br_if $label$1
    (i32.eqz
     (local.tee $3
      (call $shine::game::Transform*\20shine::game::Node::getComponent<shine::game::Transform>\28\29
       (local.get $3)
      )
     )
    )
   )
   (call $shine::game::Transform::worldXY\28float&\2c\20float&\29\20const
    (local.get $3)
    (i32.add
     (local.get $4)
     (i32.const 12)
    )
    (i32.add
     (local.get $4)
     (i32.const 8)
    )
   )
   (br_if $label$1
    (f32.lt
     (local.get $1)
     (f32.sub
      (local.tee $5
       (f32.load offset=12
        (local.get $4)
       )
      )
      (local.tee $6
       (f32.mul
        (f32.load offset=56
         (local.get $3)
        )
        (f32.const 0.5)
       )
      )
     )
    )
   )
   (br_if $label$1
    (f32.gt
     (local.get $1)
     (f32.add
      (local.get $6)
      (local.get $5)
     )
    )
   )
   (br_if $label$1
    (f32.lt
     (local.get $2)
     (f32.sub
      (local.tee $1
       (f32.load offset=8
        (local.get $4)
       )
      )
      (local.tee $5
       (f32.mul
        (f32.load offset=60
         (local.get $3)
        )
        (f32.const 0.5)
       )
      )
     )
    )
   )
   (br_if $label$1
    (f32.gt
     (local.get $2)
     (f32.add
      (local.get $5)
      (local.get $1)
     )
    )
   )
   (i32.store offset=12
    (local.tee $3
     (i32.load offset=24
      (local.get $0)
     )
    )
    (i32.or
     (i32.load offset=12
      (local.get $3)
     )
     (i32.const 32)
    )
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $4)
    (i32.const 16)
   )
  )
 )
 (func $init.command_export (param $0 i32)
  (call $__wasm_call_ctors)
  (call $init
   (local.get $0)
  )
 )
 (func $resize.command_export (param $0 i32) (param $1 i32)
  (call $__wasm_call_ctors)
  (call $resize
   (local.get $0)
   (local.get $1)
  )
 )
 (func $frame.command_export (param $0 f32)
  (call $__wasm_call_ctors)
  (call $frame
   (local.get $0)
  )
 )
 (func $pointer.command_export (param $0 f32) (param $1 f32) (param $2 i32)
  (call $__wasm_call_ctors)
  (call $pointer
   (local.get $0)
   (local.get $1)
   (local.get $2)
  )
 )
 (func $malloc.command_export (param $0 i32) (result i32)
  (call $__wasm_call_ctors)
  (call $malloc
   (local.get $0)
  )
 )
 (func $free.command_export (param $0 i32)
  (call $__wasm_call_ctors)
  (call $free
   (local.get $0)
  )
 )
 (func $on_tex_loaded.command_export (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32)
  (call $__wasm_call_ctors)
  (call $on_tex_loaded
   (local.get $0)
   (local.get $1)
   (local.get $2)
   (local.get $3)
  )
 )
 (func $on_tex_failed.command_export (param $0 i32) (param $1 i32)
  (call $__wasm_call_ctors)
  (call $on_tex_failed
   (local.get $0)
   (local.get $1)
  )
 )
 ;; custom section ".debug_abbrev", size 818
 ;; custom section ".debug_info", size 6752
 ;; custom section ".debug_ranges", size 1840
 ;; custom section ".debug_str", size 2320
 ;; custom section ".debug_line", size 17049
 ;; custom section "producers", size 54
)

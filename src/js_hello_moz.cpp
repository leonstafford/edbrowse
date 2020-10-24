// This program came from the Mozilla site, for moz 52.
// https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/How_to_embed_the_JavaScript_engine
// I tweaked it for moz 60.

#include "jsapi.h"
#include "js/Initialization.h"

static JSClassOps global_ops = {
// I removed a nullptr, moz 60 has one less parameter I guess.
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JS_GlobalObjectTraceHook
};

/* The class of the global object. */
static JSClass global_class = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    &global_ops
};

int main(int argc, const char *argv[])
{
    JS_Init();

    JSContext *cx = JS_NewContext(8L * 1024 * 1024);
    if (!cx)
        return 1;
    if (!JS::InitSelfHostedCode(cx))
        return 1;

    { // Scope for our various stack objects (JSAutoRequest, RootedObject), so they all go
      // out of scope before we JS_DestroyContext.

      JSAutoRequest ar(cx); // In practice, you would want to exit this any
                            // time you're spinning the event loop

      JS::CompartmentOptions options;
      JS::RootedObject global(cx, JS_NewGlobalObject(cx, &global_class, nullptr, JS::FireOnNewGlobalHook, options));
      if (!global)
          return 1;

//      JS::RootedValue rval(cx);
// This works, but the GC rooting guide suggests doing it a different way,
// which also works. I don't know if this new approach is part of 60.
// Well the guide recommends it so I'm trying it.
// This reminds me of memory::unique_ptr, wrapper around a pointer,
// so things are disposed of when they go out of scope.
      JS::RootedValue v(cx);
      JS::MutableHandleValue rval = &v;

      { // Scope for JSAutoCompartment
        JSAutoCompartment ac(cx, global);
        JS_InitStandardClasses(cx, global);

        const char *script = "'hello'+' world, it is '+new Date()";
        const char *filename = "noname";
        int lineno = 1;
        JS::CompileOptions opts(cx);
        opts.setFileAndLine(filename, lineno);
// The call to Evaluate was passed &rval when rval was the rooted value;
// now it's rval when using the handle wrapper around the value.
// The prototype in jsapi.h expects JS::MutableHandleValue rval as fifth parameter.
// Why then does the old way work?
// Because JS::RootedValue * casts properly into JS::MutableHandleValue, I guess.
        bool ok = JS::Evaluate(cx, opts, script, strlen(script), rval);
        if (!ok)
          return 1;
      }

      JSString *str = rval.toString();
      printf("%s\n", JS_EncodeString(cx, str));
    }

    JS_DestroyContext(cx);
    JS_ShutDown();
    return 0;
}

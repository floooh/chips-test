/* sokol implementations need to live in it's own source file, because
on MacOS and iOS the implementation must be compiled as Objective-C, so there
must be a *.m file on MacOS/iOS, and *.c file everywhere else
*/
#define SOKOL_IMPL
#if defined(_WIN32)
#define SOKOL_LOG(s) OutputDebugStringA(s)
#endif
/* sokol 3D-API defines are provided by build options */
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_time.h"
#include "sokol_audio.h"
#include "sokol_args.h"
#include "sokol_gl.h"
#include "sokol_fetch.h"
#include "sokol_debugtext.h"
#include "sokol_glue.h"

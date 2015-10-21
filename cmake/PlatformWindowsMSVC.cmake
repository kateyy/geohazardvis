message(STATUS "Configuring for platform Windows/MSVC.")

list(APPEND DEFAULT_COMPILE_DEFS
    WIN32                       # Windows system
    UNICODE                     # Use unicode
    _UNICODE                    # Use unicode
#    _SCL_SECURE_NO_WARNINGS        # Calling any one of the potentially unsafe methods in the Standard C++ Library
#    _CRT_SECURE_NO_DEPRECATE   # Disable CRT deprecation warnings
)

option(OPTION_RELEASE_LTCG "Enable whole program optimization / link time code generation in release builds" OFF)
option(OPTION_DEBUG_ADDITIONAL_RUNTME_CHECKS "Include all available runtime checks in debug builds." OFF)

option(OPTION_WIN32_GUI_APP "Build an executable with a WinMain entry point on windows. This disables the console window" ON)
if(OPTION_WIN32_GUI_APP)
    set(WIN32_EXE_FLAG WIN32)
else()
    set (WIN32_EXE_FLAG "")
endif()


set(DEFAULT_COMPILE_FLAGS
    /nologo /Zc:wchar_t /Zc:forScope /GR /Zi /fp:precise /MP /W4 
    /we4150 /we4239 /we4390 /we4456 /we4457 /we4700 /we4701 /we4703 /we4715 /we4717
    /wd4127 /wd4351 /wd4458 /wd4505 /wd4718
    # nologo       -> no logo
    # Zc:wchar_t   -> treat wchar_t as built-in type: yes
    # Zc:forScope  -> force conformance in for loop scope: Yes
    # GF           -> string pooling (default: debug:off, release:on)
    # GR           -> runtime type information
    # GS           -> buffer security check
    # Zi           -> debug information format: program database
    # Gm           -> enable minimal rebuild
    # fp:precise   -> floating point model: precise
    # fp:fast      -> floating point model: fast
    # Ot           -> favor fast code
    # bigobj       -> Increase Number of Sections in .Obj file (required for libzeug and MSVC 14)
    # MP           -> build with multiple processes
    # wd           -> disable warning
    # we           -> treat warning as error
    #   4100       -> 'identifier' : unreferenced formal parameter
    #   4127       -> conditional expression is constant (caused by Qt)
    #   4189       -> 'identifier' : local variable is initialized but not referenced  
    #   4239       -> nonstandard extension used : 'token' : conversion from 'type' to 'type'
    #   4250       -> 'class1' : inherits 'class2::member' via dominance
    #   4251       -> 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
    #   4267       -> 'var' : conversion from 'size_t' to 'type', possible loss of data
    #   4351       -> new behavior: elements of array '...' will be default initialized
    #   4390       -> ';' : empty controlled statement found; is this the intent?
    #   4456       -> declaration of 'x' hides previous local declaration
    #   4457       -> declaration of 'x' hides function parameter
    #   4458       -> declaration of 'x' hides class member
    #   4505       -> 'function' : unreferenced local function has been removed (caused by libzeug)
    #   4512       -> 'class' : assignment operator could not be generated
    #   4700       -> uninitialized local variable 'name' used
    #   4701       -> Potentially uninitialized local variable 'name' used
    #   4703       -> Potentially uninitialized local pointer variable 'name' used
    #   4715       -> 'function' : not all control paths return a value
    #   4717       -> 'function' : recursive on all control paths, function will cause runtime stack overflow
    #   4718       -> 'function call' : recursive call has no side effects, deleting (QMapNode/qmap.h)
    # W4           -> warning level 4
    # WX           -> treat warnings as errors
 
    # For debug: 
    # MDd          -> Runtime Library: Multithreaded Debug DLL
    # Od           -> Optimization: none
    # RTC1         -> Runtime error checks
    
    # Zo           -> generates richer debugging information for optimized code (non- /Od builds) (VS2013 Update 3)
 
    # For release: 
    # W3           -> warn level 3
    # MD           -> runtime library: multithreaded dll
    # Ox           -> optimization: full optimization 
    # Ob2          -> inline function expansion: any suitable
    # Oi           -> enable intrinsic functions: yes
    # Oy           -> omit frame pointers: yes
    # GL           -> whole program optimization: enable link-time code generation
    # arch:SSE2    -> enable enhanced instruction set: streaming simd extensions 2
)

# version specific flags
# 1800: Visual Studio 12 2013
# 1900: Visual Studio 14 2015

if(MSVC_VERSION VERSION_GREATER 1800)
    list(APPEND DEFAULT_COMPILE_FLAGS
        /bigobj
        /guard:cf
    )
endif()

list(APPEND DEFAULT_COMPILE_FLAGS
    $<$<CONFIG:Debug>:          /MDd /RTC1 /Od /GF- >
    $<$<CONFIG:Release>:        /MD /Ot /Ob2 /Ox /GS- /GF >
    $<$<CONFIG:RelWithDebInfo>: /MD /Ot /Ob2 /Ox /GS- /GF /Zo>
    $<$<CONFIG:MinSizeRel>:     /MD /Os /Ob1 /O1 /GS- /GF >
)

if (OPTION_DEBUG_ADDITIONAL_RUNTME_CHECKS)
    list(APPEND DEFAULT_COMPILE_FLAGS
        $<$<CONFIG:Debug>:      /RTCc /GS /sdl >
    )
endif()

set(WIN32_LINKER_FLAGS
    "/NOLOGO /NXCOMPAT"
    # NOLOGO                                            -> suppress logo
    # INCREMENTAL:NO                                    -> enable incremental linking: no
    # MANIFEST                                          -> generate manifest: yes
    # MANIFESTUAC:"level='asInvoker' uiAccess='false'"  -> uac execution level: asinvoker, uac bypass ui protection: false
    # NXCOMPAT                                          -> data execution prevention (dep): image is compatible with dep
    # DYNAMICBASE:NO                                    -> randomized base address: disable image randomization
)

set(DEFAULT_LINKER_FLAGS_DEBUG
    "${WIN32_LINKER_FLAGS} /DEBUG"
    # DEBUG        -> create debug info
)

set(DEFAULT_LINKER_FLAGS_RELEASE
    "${WIN32_LINKER_FLAGS} /OPT:REF /OPT:ICF /DELAY:UNLOAD /INCREMENTAL:NO"
    # OPT:REF      -> references: eliminate unreferenced data
    # OPT:ICF      -> enable comdat folding: remove redundant comdats
    # LTCG         -> link time code generation: use link time code generation
    # DELAY:UNLOAD -> delay loaded dll: support unload
)

set(DEFAULT_LINKER_FLAGS_RELWITHDEBINFO
    "${WIN32_LINKER_FLAGS} /OPT:REF /OPT:ICF /DELAY:UNLOAD /INCREMENTAL:NO /DEBUG"
)

if (OPTION_RELEASE_LTCG)
    list(APPEND DEFAULT_COMPILE_FLAGS $<$<NOT:$<CONFIG:Debug>>: /GL> )
    set(DEFAULT_LINKER_FLAGS_RELEASE "${DEFAULT_LINKER_FLAGS_RELEASE} /LTCG")
    set(DEFAULT_LINKER_FLAGS_RELWITHDEBINFO "${DEFAULT_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
endif()

# Add platform specific libraries for linking
set(EXTRA_LIBS "")

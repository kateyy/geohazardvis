message(STATUS "Configuring for platform Windows/MSVC.")

LIST(APPEND DEFAULT_COMPILE_DEFS
    WIN32                       # Windows system
    UNICODE                     # Use unicode
    _UNICODE                    # Use unicode
#    _SCL_SECURE_NO_WARNINGS        # Calling any one of the potentially unsafe methods in the Standard C++ Library
#    _CRT_SECURE_NO_DEPRECATE   # Disable CRT deprecation warnings
)

set(WIN32_COMPILE_FLAGS
    /nologo /Zc:wchar_t /Zc:forScope /GR /Zi /fp:precise /MP /W4 /wd4127 /wd4351 /wd4505 /wd4512 /wd4718
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
    # MP           -> build with multiple processes
    # wd           -> disable warning
    #   4100       -> 'identifier' : unreferenced formal parameter
    #   4127       -> conditional expression is constant (caused by Qt)
    #   4189       -> 'identifier' : local variable is initialized but not referenced  
    #   4250       -> 'class1' : inherits 'class2::member' via dominance
    #   4251       -> 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
    #   4267       -> 'var' : conversion from 'size_t' to 'type', possible loss of data
    #   4351       -> new behavior: elements of array '...' will be default initialized
    #   4505       -> 'function' : unreferenced local function has been removed (caused by libzeug)
    #   4512       -> 'class' : assignment operator could not be generated
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

set(DEFAULT_COMPILE_FLAGS
    ${WIN32_COMPILE_FLAGS}
    $<$<CONFIG:Debug>:          /MDd /RTC1 /RTCc /Od /GS /sdl /GF- >
    $<$<CONFIG:Release>:        /MD /Ot /Ob2 /Ox /GS- /GL /GF >
    $<$<CONFIG:RelWithDebInfo>: /MD /Ot /Ob2 /Ox /GS- /GL /GF /Zo>
    $<$<CONFIG:MinSizeRel>:     /MD /Os /Ob1 /O1 /GS- /GL /GF >
)

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
    "${WIN32_LINKER_FLAGS} /OPT:REF /LTCG /OPT:ICF /DELAY:UNLOAD /INCREMENTAL:NO"
    # OPT:REF      -> references: eliminate unreferenced data
    # OPT:ICF      -> enable comdat folding: remove redundant comdats
    # LTCG         -> link time code generation: use link time code generation
    # DELAY:UNLOAD -> delay loaded dll: support unload
)

set(DEFAULT_LINKER_FLAGS_RELWITHDEBINFO
    "${WIN32_LINKER_FLAGS} /OPT:REF /LTCG /OPT:ICF /DELAY:UNLOAD /INCREMENTAL:NO /DEBUG"
)


# Add platform specific libraries for linking
set(EXTRA_LIBS "")

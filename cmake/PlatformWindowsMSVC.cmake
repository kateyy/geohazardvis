message(STATUS "Configuring for platform Windows/MSVC.")

list(APPEND DEFAULT_COMPILE_DEFS
    WIN32                       # Windows system
    UNICODE                     # Use unicode
    _UNICODE                    # Use unicode
    _SCL_SECURE_NO_WARNINGS     # Calling any one of the potentially unsafe methods in the Standard C++ Library
                                # Required for vtkDataArrayAccessor and vtkAOSDataArrayTemplate related classes
#    _CRT_SECURE_NO_DEPRECATE   # Disable CRT deprecation warnings
)

option(OPTION_RELEASE_LTCG "Enable whole program optimization / link time code generation in Release builds" OFF)
option(OPTION_RELWITHDEBINFO_LTCG "Enable whole program optimization / link time code generation in RelWithDebInfo builds" OFF)
option(OPTION_DEBUG_ADDITIONAL_RUNTME_CHECKS "Include all available runtime checks in debug builds." OFF)

option(OPTION_WIN32_GUI_APP "Build an executable with a WinMain entry point on windows. This disables the console window" ON)
if(OPTION_WIN32_GUI_APP)
    set(WIN32_EXE_FLAG WIN32)
else()
    set (WIN32_EXE_FLAG "")
endif()


set(DEFAULT_COMPILE_FLAGS
    /nologo
    /Zc:forScope /Zc:inline /Zc:rvalueCast /Zc:wchar_t
    /GR /fp:precise /MP /W4
    /we4150 /we4172 /we4239 /we4390 /we4456 /we4457 /we4458 /we4700 /we4701 /we4703 /we4715 /we4717
    /wd4127 /wd4251 /wd4351 /wd4505 /wd4661 /wd4718

    $<$<CONFIG:Debug>:             /MDd /Od /RTC1> # /Z* flag: see version specific flags
    $<$<CONFIG:Release>:           /MD  /O2 >
    $<$<CONFIG:RelWithDebInfo>:    /MD  /O2 /Zo /Zi>
    $<$<CONFIG:RelNoOptimization>: /MD  /Od /RTC1>

    # nologo       -> Suppresses the display of the copyright banner when the compiler starts up and display of informational messages during compiling.

    # MD           -> runtime library: multithreaded dll
    # MDd          -> Runtime Library: Multithreaded Debug DLL

    # Od           -> Optimization: none
    # O1           -> Minimize Size, equivalent to /Og /Os /Oy /Ob2 /Gs /GF /Gy
    # O2           -> Maximize Speed, equivalent to /Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy
    # Ob2          -> inline function expansion: any suitable
    # Ox           -> optimization: full optimization, equivalent to /Ob2 /Og /Ot /Oy
    # Oi           -> enable intrinsic functions: yes
    # Oy           -> omit frame pointers: yes
    # Ot           -> favor fast code

    # RTC1         -> Runtime error checks
    # guard:cf     -> Enable compiler generation of Control Flow Guard security checks (Visual Studio 2015, incompatible with /ZI)

    # MP           -> build with multiple processes

    # GF           -> string pooling (set by /O1, /O2 and /ZI)
    # GL           -> whole program optimization: enable link-time code generation
    # GR           -> runtime type information
    # GS           -> buffer security check
    # Gm           -> enable minimal rebuild

    # arch:SSE2    -> enable enhanced instruction set: streaming simd extensions 2
    # fp:precise   -> floating point model: precise
    # fp:fast      -> floating point model: fast
    # bigobj       -> Increase Number of Sections in .Obj file

    # Zc:wchar_t   -> treat wchar_t as built-in type: yes
    # Zc:forScope  -> force conformance in for loop scope: Yes
    # Zc:rvalueCast -> treat rvalue references according to section 5.4 of the C++11 standard
    # Zc:inline    -> enforce section 3.2 and section 7.1.2. of the C++11 standard: definitions of called inline functions must be visible
    # Zc:throwingNew -> assume operator new throws on failure (Visual Studio 2015)
    # Zc:strictStrings -> strict const-qualification conformance for pointer initlized by using string literals (Visual Studio 2015)
    # Zc:referenceBinding -> a temporary will not bind to an non-const lvalue reference (Visual Studio 2015)

    # Zi           -> debug information format: program database
    # ZI           -> debug information format: program database supporting edit and continue (Visual Studio 2015)
    # Zo           -> generates richer debugging information for optimized code (non- /Od builds) (VS2013 Update 3, default in VS2015 when /Zi is specified)

    # wd           -> disable warning
    # we           -> treat warning as error
    #   4100       -> 'identifier' : unreferenced formal parameter
    #   4127       -> conditional expression is constant (caused by Qt)
    #   4172       -> returning address of local variable or temporary
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
    #   4661       -> 'identifier' : no suitable definition provided for explicit template instantiation request (AbstractSimpleGeoCoordinateTransformFilter.h)
    #   4700       -> uninitialized local variable 'name' used
    #   4701       -> Potentially uninitialized local variable 'name' used
    #   4703       -> Potentially uninitialized local pointer variable 'name' used
    #   4715       -> 'function' : not all control paths return a value
    #   4717       -> 'function' : recursive on all control paths, function will cause runtime stack overflow
    #   4718       -> 'function call' : recursive call has no side effects, deleting (QMapNode/qmap.h)
    # W4           -> warning level 4
    # WX           -> treat warnings as errors
)

if(OPTION_DEBUG_ADDITIONAL_RUNTME_CHECKS)
    list(APPEND DEFAULT_COMPILE_FLAGS
        $<$<CONFIG:Debug>:      /RTCc /GS /sdl >
        $<$<CONFIG:RelNoOptimization>: /RTCc /GS /sdl >
    )
endif()


if (OPTION_RELEASE_LTCG)
    list(APPEND DEFAULT_COMPILE_FLAGS $<$<CONFIG:Release>: /GL > )
endif()

if (OPTION_RELWITHDEBINFO_LTCG)
    list(APPEND DEFAULT_COMPILE_FLAGS $<$<CONFIG:RelWithDebInfo>: /GL > )
endif()

# version specific compile flags
# 1800: Visual Studio 12 2013
# 1900: Visual Studio 14 2015

if(MSVC_VERSION VERSION_LESS 1900)

    list(APPEND DEFAULT_COMPILE_FLAGS
        $<$<CONFIG:Debug>: /Zi>     # this is set in RelWithDebInfo builds for all MSVC versions
        $<$<CONFIG:RelNoOptimization>: /Zi>
    )

else()  # Visual Studio 14 2015 as minimum

    list(APPEND DEFAULT_COMPILE_FLAGS
        /Zc:referenceBinding /Zc:strictStrings /Zc:throwingNew
        # allow native edit and continue: http://blogs.msdn.com/b/vcblog/archive/2015/07/22/c-edit-and-continue-in-visual-studio-2015.aspx
        $<$<CONFIG:Debug>:             /ZI>
        $<$<CONFIG:RelNoOptimization>: /ZI>
        # Enable compiler generation of Control Flow Guard security checks. (incompatible with /ZI)
        $<$<CONFIG:Release>:           /guard:cf>
        $<$<CONFIG:RelWithDebInfo>:    /guard:cf>
    )

    # Visual Studio 2015 Update 3 introduced a new optimizer
    #   https://blogs.msdn.microsoft.com/vcblog/2016/05/04/new-code-optimizer/
    # with a few bugs, see:
    # https://bugreports.qt.io/browse/QTBUG-54443
    # https://connect.microsoft.com/VisualStudio/feedback/details/2992985
    # https://connect.microsoft.com/VisualStudio/feedback/details/2988420
    # https://connect.microsoft.com/VisualStudio/Feedback/Details/2984689
    # Just to be safe, disable this optimizer until these bugs are fixed.
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "19.0.24215.1")
        message("Disabling new MSVC optimizer to workaround some bugs (-d2SSAOptimizer-).")
        list(APPEND DEFAULT_COMPILE_FLAGS -d2SSAOptimizer-)
    else()
        message(WARNING "Unknown MSVC 2015 compiler version. Please recheck bug reports regarding the new MSVC SSA Optimizer.")
    endif()

endif()


set(DEFAULT_LINKER_FLAGS
    "/NOLOGO /NXCOMPAT"
    # NOLOGO                                            -> suppress logo
    # INCREMENTAL:NO                                    -> enable incremental linking: no
    # MANIFEST                                          -> generate manifest: yes
    # MANIFESTUAC:"level='asInvoker' uiAccess='false'"  -> uac execution level: asinvoker, uac bypass ui protection: false
    # NXCOMPAT                                          -> data execution prevention (dep): image is compatible with dep
    # DYNAMICBASE:NO                                    -> randomized base address: disable image randomization
)

set(DEFAULT_LINKER_FLAGS_DEBUG
    "${DEFAULT_LINKER_FLAGS} /DEBUG"
    # DEBUG        -> create debug info, implies /INCREMENTAL
)

set(DEFAULT_LINKER_FLAGS_RELNOOPTIMIZATION ${DEFAULT_LINKER_FLAGS_DEBUG})

set(DEFAULT_LINKER_FLAGS_RELEASE
    "${DEFAULT_LINKER_FLAGS} /OPT:REF /OPT:ICF /INCREMENTAL:NO"
    # OPT:REF      -> references: eliminate unreferenced data
    # OPT:ICF      -> enable comdat folding: remove redundant comdats
    # LTCG         -> link time code generation: use link time code generation
    # DELAY:UNLOAD -> delay loaded dll: support unload
)

set(DEFAULT_LINKER_FLAGS_RELWITHDEBINFO "${DEFAULT_LINKER_FLAGS_RELEASE} /DEBUG")


set(DEFAULT_STATIC_LIBRARY_FLAGS "/NOLOGO")
set(DEFAULT_STATIC_LIBRARY_FLAGS_DEBUG ${DEFAULT_STATIC_LIBRARY_FLAGS})
set(DEFAULT_STATIC_LIBRARY_FLAGS_RELNOOPTIMIZATION ${DEFAULT_STATIC_LIBRARY_FLAGS})
set(DEFAULT_STATIC_LIBRARY_FLAGS_RELEASE ${DEFAULT_STATIC_LIBRARY_FLAGS})
set(DEFAULT_STATIC_LIBRARY_FLAGS_RELWITHDEBINFO ${DEFAULT_STATIC_LIBRARY_FLAGS})


if(OPTION_RELEASE_LTCG)
    set(DEFAULT_LINKER_FLAGS_RELEASE "${DEFAULT_LINKER_FLAGS_RELEASE} /LTCG")
    set(DEFAULT_STATIC_LIBRARY_FLAGS_RELEASE "${DEFAULT_STATIC_LIBRARY_FLAGS_RELEASE} /LTCG")
endif()

if(OPTION_RELWITHDEBINFO_LTCG)
    set(DEFAULT_LINKER_FLAGS_RELWITHDEBINFO "${DEFAULT_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
    set(DEFAULT_STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "${DEFAULT_STATIC_LIBRARY_FLAGS_RELWITHDEBINFO} /LTCG")
endif()

# version specific linker flags
if(MSVC_VERSION VERSION_GREATER 1800)
    set(DEFAULT_LINKER_FLAGS_RELEASE "${DEFAULT_LINKER_FLAGS_RELEASE} /GUARD:CF")
    set(DEFAULT_LINKER_FLAGS_RELWITHDEBINFO "${DEFAULT_LINKER_FLAGS_RELWITHDEBINFO} /GUARD:CF")
endif()

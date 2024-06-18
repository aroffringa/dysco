# Instruct the compiler to generate portable binaries if `PORTABLE=ON`.
# If `PORTABLE=OFF`, instruct the compiler to generate binaries that are optimized
# for the native CPU. The user may override this by setting `TARGET_CPU` to a
# specifc CPU architecture, e.g. `TARGET_CPU=haswell`. If the compiler doesn't know
# how to generate code for the given `TARGET_CPU`, an error will be raised.
# You cannot set `TARGET_CPU` if `PORTABLE=ON`.

include(CheckCXXCompilerFlag)
if(PORTABLE)
  # Empty by default, will be overridden when given on the command-line
  set(TARGET_CPU
      CACHE
        STRING
        "Tell the compiler to build code for the given CPU, e.g. for building containers for a different platform"
  )
  if(TARGET_CPU)
    message(
      FATAL_ERROR
        "You cannot specify a target CPU when building portable binaries. "
        "Set PORTABLE=OFF if you want to set TARGET_CPU.")
  endif()
  message(
    WARNING
      "Building portable binaries, which will have slighly decreased performance."
  )
else()
  if(NOT TARGET_CPU)
    # If not set, force it to use native CPU by default
    set(TARGET_CPU
        native
        CACHE
          STRING
          "Tell the compiler to build code for the given CPU, e.g. for building containers for a different platform"
          FORCE)
  endif()
  # Unset cached variable to force a check; value of TARGET_CPU may have changed
  unset(COMPILER_SUPPORTS_TARGET_CPU CACHE)
  check_cxx_compiler_flag("-march=${TARGET_CPU}" COMPILER_SUPPORTS_TARGET_CPU)
  if(COMPILER_SUPPORTS_TARGET_CPU)
    add_compile_options(-march=${TARGET_CPU})
  else()
    message(
      FATAL_ERROR
        "The compiler doesn't support '${TARGET_CPU}' as target CPU architecture."
    )
  endif()
endif(PORTABLE)

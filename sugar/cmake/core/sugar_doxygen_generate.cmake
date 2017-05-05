# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(CMakeParseArguments) # cmake_parse_arguments

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_test_directory_exists)
include(sugar_test_file_exists)
include(sugar_test_target_exists)

function(sugar_doxygen_generate)
  set(one_value_args TARGET DOXYTARGET DOXYFILE)
  set(options DEVELOPER)

  cmake_parse_arguments(
      doxy_generate "${options}" "${one_value_args}" "" ${ARGV}
  )

  if(doxy_generate_UNPARSED_ARGUMENTS)
    sugar_fatal_error("Unparsed: ${doxy_generate_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT doxy_generate_TARGET)
    sugar_fatal_error("TARGET is mandatory parameter")
  else()
    sugar_test_target_exists(${doxy_generate_TARGET})
  endif()

  if(NOT doxy_generate_DOXYTARGET)
    sugar_status_print("target for doxygen not provided, default used: doc")
    set(doxy_generate_DOXYTARGET "doc")
  endif()

  if(TARGET ${doxy_generate_DOXYTARGET})
    sugar_fatal_error(
        "can't use doxygen target '${doxy_generate_DOXYTARGET}' "
        "target already exists"
    )
  endif()

  sugar_test_file_exists("${doxy_generate_DOXYFILE}")

  get_target_property(sources ${doxy_generate_TARGET} SOURCES)
  list(REMOVE_DUPLICATES sources)
  math(EXPR src_num 0)
  set(doxygen_sources_list "")
  foreach(x ${sources})
    string(REGEX MATCH ".*/CMakeLists\\.txt" match_exclude "${x}")
    if(NOT match_exclude)
      string(REGEX MATCH ".*/.*\\.cmake" match_exclude "${x}")
    endif()
    if(NOT match_exclude)
      string(REGEX MATCH ".*/xcode\\.environment" match_exclude "${x}")
    endif()
    if(NOT match_exclude)
      math(EXPR src_num "${src_num} + 1")
      set(
          doxygen_sources_list
          "${doxygen_sources_list} \\
          ${x}"
      )
    endif()
  endforeach()

  # collect definitions from doxy_generate_TARGET and directory
  set(definitions "")
  set(definitions_release "")
  set(definitions_debug "")

  get_target_property(x ${doxy_generate_TARGET} COMPILE_DEFINITIONS)
  if(x)
    list(APPEND definitions ${x})
  endif()
  get_target_property(x ${doxy_generate_TARGET} COMPILE_DEFINITIONS_RELEASE)
  if(x)
    list(APPEND definitions_release ${x})
  endif()
  get_target_property(x ${doxy_generate_TARGET} COMPILE_DEFINITIONS_DEBUG)
  if(x)
    list(APPEND definitions_debug ${x})
  endif()

  get_directory_property(x COMPILE_DEFINITIONS)
  if(x)
    list(APPEND definitions ${x})
  endif()
  get_directory_property(x COMPILE_DEFINITIONS_RELEASE)
  if(x)
    list(APPEND definitions_release ${x})
  endif()
  get_directory_property(x COMPILE_DEFINITIONS_DEBUG)
  if(x)
    list(APPEND definitions_debug ${x})
  endif()

  if(${doxy_generate_DEVELOPER})
    list(APPEND definitions ${definitions_debug})
  else()
    list(APPEND definitions ${definitions_release})
  endif()
  if(definitions)
    foreach(x ${definitions})
      set(
          doxygen_predefined
          "${doxygen_predefined} \\
          ${x}"
      )
    endforeach()
    set(SUGAR_DOXYGEN_PREDEFINED "${doxygen_predefined}")
  endif()

  set(SUGAR_DOXYGEN_MACRO_EXPANSION "YES")

  get_target_property(include_dirs ${doxy_generate_TARGET} INCLUDE_DIRECTORIES)
  set(doxygen_strip_from_path "")
  foreach(x ${include_dirs})
    set(
        doxygen_strip_from_path
        "${doxygen_strip_from_path} \\
        ${x}"
    )
  endforeach()

  find_package(Doxygen REQUIRED)

  set(docdir "${PROJECT_BINARY_DIR}/doxygen-${doxy_generate_DOXYTARGET}")

  set(doxygen_directory "${docdir}/doxygen")
  set(doxygen_log "${docdir}/doxy.log")
  set(doxygen_err "${docdir}/doxy.err")

  # Common (Release & Developer)
  set(SUGAR_DOXYGEN_OUTPUT_DIRECTORY "${doxygen_directory}")
  set(SUGAR_DOXYGEN_STRIP_FROM_PATH "${doxygen_strip_from_path}")
  set(SUGAR_DOXYGEN_PROJECT_NAME ${CMAKE_PROJECT_NAME})
  set(SUGAR_DOXYGEN_INLINE_INHERITED_MEMB "YES")
  set(SUGAR_DOXYGEN_TAB_SIZE "2")
  set(SUGAR_DOXYGEN_INPUT ${doxygen_sources_list})
  set(SUGAR_DOXYGEN_SOURCE_BROWSER "YES")
  set(SUGAR_DOXYGEN_EXTRACT_LOCAL_CLASSES "YES")
  set(SUGAR_DOXYGEN_GENERATE_TREEVIEW "YES")

  set(SUGAR_DOXYGEN_GENERATE_DOCSET "NO")

  if(DOXYGEN_DOT_FOUND)
    set(SUGAR_DOXYGEN_HAVE_DOT "YES")
    set(SUGAR_DOXYGEN_UML_LOOK "YES")
  else()
    sugar_status_print("Dot found found")
    set(SUGAR_DOXYGEN_HAVE_DOT "NO")
    set(SUGAR_DOXYGEN_UML_LOOK "NO")
  endif()

  if(${doxy_generate_DEVELOPER})
    sugar_status_print("doxygen developer(verbose) version")
    set(doxygen_verbose "YES")
    set(mode "(developer)")
    set(SUGAR_DOXYGEN_HIDE_UNDOC_MEMBERS "NO")
  else()
    set(doxygen_verbose "NO")
    set(mode "(user)")
    set(SUGAR_DOXYGEN_HIDE_UNDOC_MEMBERS "YES")
  endif()

  set(SUGAR_DOXYGEN_CALLER_GRAPH "${doxygen_verbose}")
  set(SUGAR_DOXYGEN_CALL_GRAPH "${doxygen_verbose}")
  set(SUGAR_DOXYGEN_EXTRACT_ALL "${doxygen_verbose}")
  set(SUGAR_DOXYGEN_EXTRACT_ANON_NSPACES "${doxygen_verbose}")
  set(SUGAR_DOXYGEN_EXTRACT_PACKAGE "${doxygen_verbose}")
  set(SUGAR_DOXYGEN_EXTRACT_PRIVATE "${doxygen_verbose}")
  set(SUGAR_DOXYGEN_EXTRACT_STATIC "${doxygen_verbose}")
  set(SUGAR_DOXYGEN_INLINE_SOURCES "${doxygen_verbose}")
  set(SUGAR_DOXYGEN_INTERNAL_DOCS "${doxygen_verbose}")
  set(SUGAR_DOXYGEN_REFERENCED_BY_RELATION "${doxygen_verbose}")
  set(SUGAR_DOXYGEN_REFERENCES_RELATION "${doxygen_verbose}")

  set(doxyfile_output "${docdir}/Doxyfile")
  configure_file("${doxy_generate_DOXYFILE}" "${doxyfile_output}")

  set(comment_begin "Generate documentation${mode}, ${src_num} files\nUse:\n")
  set(comment_log "`tail -f ${doxygen_log}`\n")
  set(comment_err "`tail -f ${doxygen_err}`\n")
  set(comment_end "to see process\n")
  set(html "\n[HTML] ->\n    ${doxygen_directory}/html/index.html")
  set(
      comment
      "${comment_begin} ${comment_log} ${comment_err} ${comment_end} ${html}"
  )

  add_custom_target(
    ${doxy_generate_DOXYTARGET}
    COMMAND
    "${CMAKE_COMMAND}" -E make_directory "${doxygen_directory}"
    COMMAND
    "${DOXYGEN_EXECUTABLE}" ${doxyfile_output} > "${doxygen_log}" 2> "${doxygen_err}"
    WORKING_DIRECTORY
    "${CMAKE_BINARY_DIR}"
    COMMENT
    "${comment}"
    VERBATIM
  )
endfunction()

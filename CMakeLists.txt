#
# This file is open source software, licensed to you under the terms
# of the Apache License, Version 2.0 (the "License").  See the NOTICE file
# distributed with this work for additional information regarding copyright
# ownership.  You may not use this file except in compliance with the License.
#
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

#
# Copyright (C) 2018 Scylladb, Ltd.
#

cmake_minimum_required (VERSION 3.13)

list (APPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src)

cmake_policy (SET CMP0090 NEW)
if (POLICY CMP0135)
  cmake_policy (SET CMP0135 NEW)
endif ()

include (Cooking OPTIONAL)

# This variable impacts the way DPDK is configured by cmake-cooking (if DPDK is enabled), so its definition needs to
# come before PROJECT.
set (Seastar_DPDK_MACHINE
  "native"
  CACHE
  STRING
  "Configure DPDK for this processor architecture (if `Seastar_DPDK` is enabled). A DPDK code name abbreviation (e.g., ivb)")

project (Seastar
  VERSION 1.0
  LANGUAGES CXX)

# generic boolean values passed as string, potentially from configure.py
set (True_STRING_VALUES "ON" "yes" "Yes" "YES" "true" "True" "TRUE")
set (False_STRING_VALUES "OFF" "no" "No" "NO" "false" "False" "FALSE")
set (Default_STRING_VALUES "DEFAULT" "default" "Default")

set (Seastar_ALLOC_FAILURE_INJECTION
  "DEFAULT"
  CACHE
  STRING
  "Enable failure injection into the Seastar allocator. Can be ON, OFF or DEFAULT (which enables it for Dev mode)")

option (Seastar_TASK_BACKTRACE
  "Collect backtrace at deferring points."
  OFF)

option (Seastar_DEBUG_ALLOCATIONS
  "For now just writes 0xab to newly allocated memory"
  OFF)

option (Seastar_SSTRING
  "Use seastar's own string implementation"
  ON)

set (Seastar_API_LEVEL
  "6"
  CACHE
  STRING
  "Seastar compatibility API level (2=server_socket::accept() returns accept_result, 3=make_file_output_stream(), make_file_data_sink() returns future<...>, 4=when_all_succeed returns future<std::tuple<>>, 5=future<T>::get() returns T&&), 6=future is not variadic")

set_property (CACHE Seastar_API_LEVEL
  PROPERTY
  STRINGS 2 3 4 5 6)

set (Seastar_SCHEDULING_GROUPS_COUNT
  "16"
  CACHE
  STRING
  "A positive number to set Seastar's reactor number of allowed different scheduling groups.")

if (NOT Seastar_SCHEDULING_GROUPS_COUNT MATCHES "^[1-9][0-9]*")
  message(FATAL_ERROR "Seastar_SCHEDULING_GROUPS_COUNT must be a positive number (${Seastar_SCHEDULING_GROUPS_COUNT})")
endif()

#
# Add a dev build type.
#
# All pre-defined build modes include optimizations or debug info,
# which make them slow to build. The dev build mode is intended for
# fast build/test iteration.
#

if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
  set (CMAKE_CXX_FLAGS_DEV_OPT_LEVEL "-O2")
else ()
  set (CMAKE_CXX_FLAGS_DEV_OPT_LEVEL "-O1")
endif ()

set (CMAKE_CXX_FLAGS_DEV
  "${CMAKE_CXX_FLAGS_DEV_OPT_LEVEL}"
  CACHE
  STRING
  "Flags used by the C++ compiler during dev builds."
  FORCE)

set (CMAKE_C_FLAGS_DEV
  "-O1"
  CACHE
  STRING
  "Flags used by the C compiler during dev builds."
  FORCE)

set (CMAKE_EXE_LINKER_FLAGS_DEV
  ""
  CACHE
  STRING
  "Flags used for linking binaries during dev builds."
  FORCE)

set (CMAKE_SHARED_LINKER_FLAGS_DEV
  ""
  CACHE
  STRING
  "Flags used by the shared libraries linker during builds."
  FORCE)

mark_as_advanced (
    CMAKE_CXX_FLAGS_DEV
    CMAKE_C_FLAGS_DEV
    CMAKE_EXE_LINKER_FLAGS_DEV
    CMAKE_SHARED_LINKER_FLAGS_DEV)

set (CMAKE_CXX_FLAGS_SANITIZE
  "-Os -g"
  CACHE
  STRING
  "Flags used by the C++ compiler during sanitize builds."
  FORCE)

set (CMAKE_CXX_STANDARD
  "20"
  CACHE
  STRING
  "C++ standard to build with.")

set (CMAKE_C_FLAGS_SANITIZE
  "-Os -g"
  CACHE
  STRING
  "Flags used by the C compiler during sanitize builds."
  FORCE)

set (CMAKE_EXE_LINKER_FLAGS_SANITIZE
  ""
  CACHE
  STRING
  "Flags used for linking binaries during sanitize builds."
  FORCE)

set (CMAKE_SHARED_LINKER_FLAGS_SANITIZE
  ""
  CACHE
  STRING
  "Flags used by the shared libraries linker during sanitize builds."
  FORCE)

mark_as_advanced (
    CMAKE_CXX_FLAGS_SANITIZE
    CMAKE_C_FLAGS_SANITIZE
    CMAKE_EXE_LINKER_FLAGS_SANITIZE
    CMAKE_SHARED_LINKER_FLAGS_SANITIZE)

set (CMAKE_BUILD_TYPE
  "${CMAKE_BUILD_TYPE}"
  CACHE
  STRING
  "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Dev Sanitize."
  FORCE)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE "Release")
endif ()

set (Seastar_ALLOC_PAGE_SIZE
  ""
  CACHE
  STRING
  "Override the Seastar allocator page size, in bytes.")

function (set_option_if_package_is_found option_name package_name)
  # if the package is found, set the option on behalf of user unless it is
  # explicitly specified,
  if (DEFINED ${option_name})
    return ()
  endif ()
  if (${package_name}_FOUND)
    set (${option_name} ON CACHE BOOL "")
  endif ()
endfunction ()

# When Seastar is a top-level project, enable the non-library targets by default.
# If it is embedded with `add_subdirectory`, disable them.
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set (Seastar_MASTER_PROJECT ON)
else ()
  set (Seastar_MASTER_PROJECT OFF)
endif ()

option (Seastar_APPS
  "Enable application targets."
  ${Seastar_MASTER_PROJECT})

set (Seastar_CXX_FLAGS
  ""
  CACHE
  STRING
  "Semicolon-separated list of extra compilation flags for Seastar itself.")

option (Seastar_DEMOS
  "Enable demonstration targets."
  ${Seastar_MASTER_PROJECT})

option (Seastar_DOCS
  "Enable documentation targets."
  ${Seastar_MASTER_PROJECT})

option (Seastar_DPDK
  "Enable DPDK support."
  OFF)

option (Seastar_EXCLUDE_APPS_FROM_ALL
  "When enabled alongside Seastar_APPS, do not build applications by default."
  OFF)

option (Seastar_EXCLUDE_DEMOS_FROM_ALL
  "When enabled alongside Seastar_DEMOS, do not build demonstrations by default."
  OFF)

option (Seastar_EXCLUDE_TESTS_FROM_ALL
  "When enabled alongside Seastar_TESTING, do not build tests by default."
  OFF)

option (Seastar_EXECUTE_ONLY_FAST_TESTS
  "Only execute tests which run quickly."
  OFF)

option (Seastar_HWLOC
  "Enable hwloc support."
  ON)

if (DEFINED Seastar_IO_URING)
  option (Seastar_IO_URING
    "Enable io_uring support."
    ON)
endif ()

set (Seastar_JENKINS
  ""
  CACHE
  STRING
  "If non-empty, the prefix for XML files containing the results of running tests (for Jenkins).")

set (Seastar_LD_FLAGS
  ""
  CACHE
  STRING
  "Semicolon-separated list of extra linking flags for Seastar itself.")

option (Seastar_INSTALL
  "Install targets."
  ${Seastar_MASTER_PROJECT})

option (Seastar_NUMA
  "Enable NUMA support."
  ON)

option (Seastar_TESTING
  "Enable testing targets."
  ${Seastar_MASTER_PROJECT})

include (CMakeDependentOption)
cmake_dependent_option (Seastar_ENABLE_TESTS_ACCESSING_INTERNET
  "Enable tests accessing internet." ON
  "Seastar_TESTING" OFF)

option (Seastar_COMPRESS_DEBUG
  "Compress debug info."
  ON)

option (Seastar_SPLIT_DWARF
  "Use split dwarf."
  OFF)

option (Seastar_HEAP_PROFILING
    "Enable heap profiling. No effect when Seastar is compiled with the default allocator."
    OFF)

option (Seastar_DEFERRED_ACTION_REQUIRE_NOEXCEPT
    "Enable noexcept requirement for deferred actions."
    ON)

set (Seastar_TEST_TIMEOUT
  "300"
  CACHE
  STRING
  "Maximum allowed time for a test to run, in seconds.")

option (BUILD_SHARED_LIBS
  "Build seastar library as shared libraries instead of static"
  OFF)
# We set the following environment variables
# * ASAN_OPTIONS=disable_coredump=0:abort_on_error=1:detect_stack_use_after_return=1:verify_asan_link_order=0
#   By default asan disables core dumps because they used to be
#   huge. This is no longer the case since the shadow memory is
#   excluded, so it is safe to enable them.
#   Also, by default, to make sure it works as expected, asan
#   verifies if the asan dynamic runtime is the first DSO in the
#   initial library list by calling dl_iterate_phdr(3). But
#   Seastar provides this symbol, and its implementation references
#   some static variables. So if Seastar is built as a shared
#   library, this causes a chicken and egg problem. In other words,
#   ASan is loaded first, and it tries to reference a symbol which
#   is in turn provided by another shared library which is not
#   necessariy ready its static variables yet. And this leads leads
#   to a segfault. So, we have to disable this check when
#   "BUILD_SHARED_LIBS" is enabled.
# * UBSAN_OPTIONS=halt_on_error=1:abort_on_error=1
#   Fail the test if any undefined behavior is found and use abort
#   instead of exit. Using abort is what causes core dumps to be
#   produced.
# * BOOST_TEST_CATCH_SYSTEM_ERRORS=no
#   Normally the boost test library handles SIGABRT and prevents core
#   dumps from being produced.
# This works great with clang and gcc 10.2, but unfortunately not any
# previous gcc.
set (Seastar_ASAN_OPTIONS "disable_coredump=0:abort_on_error=1")
if ((NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")) OR
    (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 10.2))
  string (APPEND Seastar_ASAN_OPTIONS ":detect_stack_use_after_return=1")
endif ()
if (BUILD_SHARED_LIBS)
  string (APPEND Seastar_ASAN_OPTIONS ":verify_asan_link_order=0")
endif ()


set (Seastar_TEST_ENVIRONMENT
  "ASAN_OPTIONS=${Seastar_ASAN_OPTIONS};UBSAN_OPTIONS=halt_on_error=1:abort_on_error=1;BOOST_TEST_CATCH_SYSTEM_ERRORS=no"
  CACHE
  STRING
  "Environment variables for running tests")

option (Seastar_UNUSED_RESULT_ERROR
  "Make [[nodiscard]] violations an error (instead of a warning)."
  OFF)

set (Seastar_STACK_GUARDS
  "DEFAULT"
  CACHE
  STRING
  "Enable stack guards. Can be ON, OFF or DEFAULT (which enables it for non release builds)")

set (Seastar_SANITIZE
  "DEFAULT"
  CACHE
  STRING
  "Enable ASAN and UBSAN. Can be ON, OFF or DEFAULT (which enables it for Debug and Sanitize)")

set (Seastar_DEBUG_SHARED_PTR
  "DEFAULT"
  CACHE
  STRING
  "Enable shared_ptr debugging. Can be ON, OFF or DEFAULT (which enables it for Debug and Sanitize)")

#
# Useful (non-cache) variables.
#

set (Seastar_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (Seastar_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set (Seastar_GEN_BINARY_DIR ${Seastar_BINARY_DIR}/gen)

#
# Dependencies.
#

include (SeastarDependencies)
seastar_find_dependencies ()

# Private build dependencies not visible to consumers
find_package (ragel 6.10 REQUIRED)
find_package (Threads REQUIRED)
find_package (PthreadSetName REQUIRED)
find_package (Valgrind REQUIRED)

#
# Code generation helpers.
#

function (seastar_generate_ragel)
  set (one_value_args TARGET VAR IN_FILE OUT_FILE)
  cmake_parse_arguments (args "" "${one_value_args}" "" ${ARGN})
  get_filename_component (out_dir ${args_OUT_FILE} DIRECTORY)

  add_custom_command (
    DEPENDS ${args_IN_FILE}
    OUTPUT ${args_OUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
    COMMAND ${ragel_RAGEL_EXECUTABLE} -G2 -o ${args_OUT_FILE} ${args_IN_FILE}
    COMMAND sed -i -e "'1h;2,$$H;$$!d;g'" -re "'s/static const char _nfa[^;]*;//g'" ${args_OUT_FILE})

  add_custom_target (${args_TARGET}
    DEPENDS ${args_OUT_FILE})

  set (${args_VAR} ${args_OUT_FILE} PARENT_SCOPE)
endfunction ()

function (seastar_generate_swagger)
  set (one_value_args TARGET VAR IN_FILE OUT_DIR)
  cmake_parse_arguments (args "" "${one_value_args}" "" ${ARGN})
  get_filename_component (in_file_name ${args_IN_FILE} NAME)
  set (generator ${Seastar_SOURCE_DIR}/scripts/seastar-json2code.py)
  set (header_out ${args_OUT_DIR}/${in_file_name}.hh)
  set (source_out ${args_OUT_DIR}/${in_file_name}.cc)

  add_custom_command (
    DEPENDS
      ${args_IN_FILE}
      ${generator}
    OUTPUT ${header_out} ${source_out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${args_OUT_DIR}
    COMMAND ${generator} --create-cc -f ${args_IN_FILE} -o ${header_out})

  add_custom_target (${args_TARGET}
    DEPENDS
      ${header_out}
      ${source_out})

  set (${args_VAR} ${header_out} ${source_out} PARENT_SCOPE)
endfunction ()

#
# The `seastar` library.
#

seastar_generate_ragel (
  TARGET seastar_http_chunk_parsers
  VAR http_chunk_parsers_file
  IN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/http/chunk_parsers.rl
  OUT_FILE ${Seastar_GEN_BINARY_DIR}/include/seastar/http/chunk_parsers.hh)

seastar_generate_ragel (
  TARGET seastar_http_request_parser
  VAR http_request_parser_file
  IN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/http/request_parser.rl
  OUT_FILE ${Seastar_GEN_BINARY_DIR}/include/seastar/http/request_parser.hh)

seastar_generate_ragel (
  TARGET seastar_http_response_parser
  VAR http_response_parser_file
  IN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/http/response_parser.rl
  OUT_FILE ${Seastar_GEN_BINARY_DIR}/include/seastar/http/response_parser.hh)

if (Seastar_DPDK)
  set (seastar_dpdk_obj seastar-dpdk.o)
endif ()

INCLUDE_DIRECTORIES(
  ${BOOST_INCLUDE_DIRS}
  "${CMAKE_CURRENT_SOURCE_DIR}/src/web"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/wt"
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_BINARY_DIR}"
)

MACRO (FILE_TO_STRING infile outfile var)
ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${outfile}
               COMMAND ${CMAKE_COMMAND}
                   ARGS -Dinfile:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/${infile}
                        -Doutfile:FILEPATH=${CMAKE_CURRENT_BINARY_DIR}/${outfile}
                        -Dvar:STRING=${var}
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/filetostring.cmake
           MAIN_DEPENDENCY
                ${CMAKE_CURRENT_SOURCE_DIR}/${infile}
)
ENDMACRO (FILE_TO_STRING)

FILE_TO_STRING(src/web/skeleton/Plain.html    Plain_html.C      Plain_html)
FILE_TO_STRING(src/web/skeleton/Hybrid.html   Hybrid_html.C     Hybrid_html)
FILE_TO_STRING(src/web/skeleton/Boot.html     Boot_html.C       Boot_html)
FILE_TO_STRING(src/web/skeleton/Wt.min.js Wt_js.C Wt_js)
FILE_TO_STRING(src/web/skeleton/Boot.min.js Boot_js.C Boot_js)
FILE_TO_STRING(src/js/WCartesianChart.min.js WCartesianChart_js.C WCartesianChart_js)
FILE_TO_STRING(src/web/skeleton/jquery.min.js JQuery_js.C JQuery_js)

FILE_TO_STRING(src/xml/wt.xml Wt_xml.C Wt_xml)
FILE_TO_STRING(src/xml/auth_strings.xml AuthStrings_xml.C AuthStrings_xml)
FILE_TO_STRING(src/xml/bootstrap_theme.xml BootstrapTheme_xml.C BootstrapTheme_xml)
FILE_TO_STRING(src/xml/bootstrap3_theme.xml Bootstrap3Theme_xml.C Bootstrap3Theme_xml)
FILE_TO_STRING(src/xml/bootstrap5_theme.xml Bootstrap5Theme_xml.C Bootstrap5Theme_xml)
FILE_TO_STRING(src/xml/auth_css_theme.xml AuthCssTheme_xml.C AuthCssTheme_xml)

add_definitions(-DWT_BUILDING)
#add_definitions(-DWT_DATE_TZ_USE_STD)
add_definitions(-DUSE_OS_TZDB=1)
set(_TZ_BUILD_DEFS "")

#IF (WIN32)
#  set(_TZ_BUILD_DEFS USE_OS_TZDB=0 HAS_REMOTE_API=0)
#ELSE (WIN32)
  set(_TZ_BUILD_DEFS USE_OS_TZDB=1)
#ENDIF (WIN32)

IF (SHARED_LIBS)
  list(APPEND _TZ_BUILD_DEFS DATE_BUILD_DLL)
ELSE (SHARED_LIBS)
  list(APPEND _TZ_BUILD_DEFS DATE_BUILD_LIB)
ENDIF (SHARED_LIBS)

set_source_files_properties(Wt/Date/src/tz.cpp PROPERTIES COMPILE_DEFINITIONS "${_TZ_BUILD_DEFS}")

unset(_TZ_BUILD_DEFS)

add_library (seastar
  ${http_chunk_parsers_file}
  ${http_request_parser_file}
  ${seastar_dpdk_obj}
  include/seastar/core/abort_source.hh
  include/seastar/core/alien.hh
  include/seastar/core/align.hh
  include/seastar/core/aligned_buffer.hh
  include/seastar/core/app-template.hh
  include/seastar/core/array_map.hh
  include/seastar/core/bitops.hh
  include/seastar/core/bitset-iter.hh
  include/seastar/core/byteorder.hh
  include/seastar/core/cacheline.hh
  include/seastar/core/checked_ptr.hh
  include/seastar/core/chunked_fifo.hh
  include/seastar/core/circular_buffer.hh
  include/seastar/core/circular_buffer_fixed_capacity.hh
  include/seastar/core/condition-variable.hh
  include/seastar/core/deleter.hh
  include/seastar/core/distributed.hh
  include/seastar/core/do_with.hh
  include/seastar/core/dpdk_rte.hh
  include/seastar/core/enum.hh
  include/seastar/core/exception_hacks.hh
  include/seastar/core/execution_stage.hh
  include/seastar/core/expiring_fifo.hh
  include/seastar/core/fair_queue.hh
  include/seastar/core/file.hh
  include/seastar/core/file-types.hh
  include/seastar/core/fsqual.hh
  include/seastar/core/fstream.hh
  include/seastar/core/function_traits.hh
  include/seastar/core/future-util.hh
  include/seastar/core/future.hh
  include/seastar/core/gate.hh
  include/seastar/core/iostream-impl.hh
  include/seastar/core/iostream.hh
  include/seastar/util/later.hh
  include/seastar/core/layered_file.hh
  include/seastar/core/linux-aio.hh
  include/seastar/core/loop.hh
  include/seastar/core/lowres_clock.hh
  include/seastar/core/manual_clock.hh
  include/seastar/core/map_reduce.hh
  include/seastar/core/memory.hh
  include/seastar/core/metrics.hh
  include/seastar/core/metrics_api.hh
  include/seastar/core/metrics_registration.hh
  include/seastar/core/metrics_types.hh
  include/seastar/core/pipe.hh
  include/seastar/core/posix.hh
  include/seastar/core/preempt.hh
  include/seastar/core/prefetch.hh
  include/seastar/core/print.hh
  include/seastar/core/prometheus.hh
  include/seastar/core/queue.hh
  include/seastar/core/ragel.hh
  include/seastar/core/reactor.hh
  include/seastar/core/report_exception.hh
  include/seastar/core/resource.hh
  include/seastar/core/rwlock.hh
  include/seastar/core/scattered_message.hh
  include/seastar/core/scheduling.hh
  include/seastar/core/scollectd.hh
  include/seastar/core/scollectd_api.hh
  include/seastar/core/seastar.hh
  include/seastar/core/semaphore.hh
  include/seastar/core/sharded.hh
  include/seastar/core/shared_future.hh
  include/seastar/core/shared_mutex.hh
  include/seastar/core/shared_ptr.hh
  include/seastar/core/shared_ptr_debug_helper.hh
  include/seastar/core/shared_ptr_incomplete.hh
  include/seastar/core/simple-stream.hh
  include/seastar/core/slab.hh
  include/seastar/core/sleep.hh
  include/seastar/core/sstring.hh
  include/seastar/core/stall_sampler.hh
  include/seastar/core/stream.hh
  include/seastar/core/systemwide_memory_barrier.hh
  include/seastar/core/task.hh
  include/seastar/core/temporary_buffer.hh
  include/seastar/core/thread.hh
  include/seastar/core/thread_cputime_clock.hh
  include/seastar/core/thread_impl.hh
  include/seastar/core/timed_out_error.hh
  include/seastar/core/timer-set.hh
  include/seastar/core/timer.hh
  include/seastar/core/transfer.hh
  include/seastar/core/unaligned.hh
  include/seastar/core/units.hh
  include/seastar/core/vector-data-sink.hh
  include/seastar/core/weak_ptr.hh
  include/seastar/core/when_all.hh
  include/seastar/core/with_scheduling_group.hh
  include/seastar/core/with_timeout.hh
  include/seastar/http/api_docs.hh
  include/seastar/http/common.hh
  include/seastar/http/exception.hh
  include/seastar/http/file_handler.hh
  include/seastar/http/function_handlers.hh
  include/seastar/http/handlers.hh
  include/seastar/http/httpd.hh
  include/seastar/http/json_path.hh
  include/seastar/http/matcher.hh
  include/seastar/http/matchrules.hh
  include/seastar/http/mime_types.hh
  include/seastar/http/reply.hh
  include/seastar/http/request.hh
  include/seastar/http/routes.hh
  include/seastar/http/short_streams.hh
  include/seastar/http/transformers.hh
  include/seastar/http/client.hh
  include/seastar/json/formatter.hh
  include/seastar/json/json_elements.hh
  include/seastar/net/api.hh
  include/seastar/net/arp.hh
  include/seastar/net/byteorder.hh
  include/seastar/net/config.hh
  include/seastar/net/const.hh
  include/seastar/net/dhcp.hh
  include/seastar/net/dns.hh
  include/seastar/net/dpdk.hh
  include/seastar/net/ethernet.hh
  include/seastar/net/inet_address.hh
  include/seastar/net/ip.hh
  include/seastar/net/ip_checksum.hh
  include/seastar/net/native-stack.hh
  include/seastar/net/net.hh
  include/seastar/net/packet-data-source.hh
  include/seastar/net/packet-util.hh
  include/seastar/net/packet.hh
  include/seastar/net/posix-stack.hh
  include/seastar/net/proxy.hh
  include/seastar/net/socket_defs.hh
  include/seastar/net/stack.hh
  include/seastar/net/tcp-stack.hh
  include/seastar/net/tcp.hh
  include/seastar/net/tls.hh
  include/seastar/net/toeplitz.hh
  include/seastar/net/udp.hh
  include/seastar/net/unix_address.hh
  include/seastar/net/virtio-interface.hh
  include/seastar/net/virtio.hh
  include/seastar/rpc/lz4_compressor.hh
  include/seastar/rpc/lz4_fragmented_compressor.hh
  include/seastar/rpc/multi_algo_compressor_factory.hh
  include/seastar/rpc/rpc.hh
  include/seastar/rpc/rpc_impl.hh
  include/seastar/rpc/rpc_types.hh
  include/seastar/util/alloc_failure_injector.hh
  include/seastar/util/backtrace.hh
  include/seastar/util/concepts.hh
  include/seastar/util/bool_class.hh
  include/seastar/util/conversions.hh
  include/seastar/util/defer.hh
  include/seastar/util/eclipse.hh
  include/seastar/util/function_input_iterator.hh
  include/seastar/util/indirect.hh
  include/seastar/util/is_smart_ptr.hh
  include/seastar/util/lazy.hh
  include/seastar/util/log-cli.hh
  include/seastar/util/log-impl.hh
  include/seastar/util/log.hh
  include/seastar/util/noncopyable_function.hh
  include/seastar/util/optimized_optional.hh
  include/seastar/util/print_safe.hh
  include/seastar/util/process.hh
  include/seastar/util/program-options.hh
  include/seastar/util/read_first_line.hh
  include/seastar/util/reference_wrapper.hh
  include/seastar/util/spinlock.hh
  include/seastar/util/std-compat.hh
  include/seastar/util/transform_iterator.hh
  include/seastar/util/tuple_utils.hh
  include/seastar/util/variant_utils.hh
  include/seastar/util/closeable.hh
  include/seastar/util/source_location-compat.hh
  include/seastar/util/short_streams.hh
  include/seastar/websocket/server.hh
  src/core/alien.cc
  src/core/file.cc
  src/core/fair_queue.cc
  src/core/reactor_backend.cc
  src/core/thread_pool.cc
  src/core/app-template.cc
  src/core/dpdk_rte.cc
  src/core/exception_hacks.cc
  src/core/execution_stage.cc
  src/core/file-impl.hh
  src/core/fsnotify.cc
  src/core/fsqual.cc
  src/core/fstream.cc
  src/core/future.cc
  src/core/future-util.cc
  src/core/linux-aio.cc
  src/core/memory.cc
  src/core/metrics.cc
  src/core/on_internal_error.cc
  src/core/posix.cc
  src/core/prometheus.cc
  src/core/program_options.cc
  src/core/reactor.cc
  src/core/resource.cc
  src/core/sharded.cc
  src/core/scollectd.cc
  src/core/scollectd-impl.hh
  src/core/systemwide_memory_barrier.cc
  src/core/smp.cc
  src/core/sstring.cc
  src/core/thread.cc
  src/core/uname.cc
  src/core/vla.hh
  src/core/io_queue.cc
  src/core/semaphore.cc
  src/core/condition-variable.cc
  src/http/api_docs.cc
  src/http/common.cc
  src/http/file_handler.cc
  src/http/httpd.cc
  src/http/json_path.cc
  src/http/matcher.cc
  src/http/mime_types.cc
  src/http/reply.cc
  src/http/routes.cc
  src/http/transformers.cc
  src/http/url.cc
  src/http/client.cc
  src/http/request.cc
  src/json/formatter.cc
  src/json/json_elements.cc
  src/net/arp.cc
  src/net/config.cc
  src/net/dhcp.cc
  src/net/dns.cc
  src/net/dpdk.cc
  src/net/ethernet.cc
  src/net/inet_address.cc
  src/net/ip.cc
  src/net/ip_checksum.cc
  src/net/native-stack-impl.hh
  src/net/native-stack.cc
  src/net/net.cc
  src/net/packet.cc
  src/net/posix-stack.cc
  src/net/proxy.cc
  src/net/socket_address.cc
  src/net/stack.cc
  src/net/tcp.cc
  src/net/tls.cc
  src/net/udp.cc
  src/net/unix_address.cc
  src/net/virtio.cc
  src/rpc/lz4_compressor.cc
  src/rpc/lz4_fragmented_compressor.cc
  src/rpc/rpc.cc
  src/util/alloc_failure_injector.cc
  src/util/backtrace.cc
  src/util/conversions.cc
  src/util/exceptions.cc
  src/util/file.cc
  src/util/log.cc
  src/util/process.cc
  src/util/program-options.cc
  src/util/read_first_line.cc
  src/util/tmp_file.cc
  src/util/short_streams.cc
  src/websocket/server.cc

  src/Wt/FlexLayoutImpl.h src/Wt/FlexLayoutImpl.C
  src/Wt/FlexItemImpl.h src/Wt/FlexItemImpl.C
  src/Wt/PopupWindow.h src/Wt/PopupWindow.C
  src/Wt/Resizable.h src/Wt/Resizable.C
  src/Wt/ResizeSensor.h src/Wt/ResizeSensor.C
  src/Wt/ServerSideFontMetrics.h src/Wt/ServerSideFontMetrics.C
  src/Wt/SizeHandle.h src/Wt/SizeHandle.C
  src/Wt/StdGridLayoutImpl2.h src/Wt/StdGridLayoutImpl2.C
  src/Wt/StdLayoutImpl.h src/Wt/StdLayoutImpl.C
  src/Wt/StdLayoutItemImpl.h src/Wt/StdLayoutItemImpl.C
  src/Wt/StdWidgetItemImpl.h src/Wt/StdWidgetItemImpl.C
  src/Wt/Utils.h src/Wt/Utils.C
  src/Wt/WAbstractArea.h src/Wt/WAbstractArea.C
  src/Wt/WAbstractItemDelegate.h src/Wt/WAbstractItemDelegate.C
  src/Wt/WAbstractItemModel.h src/Wt/WAbstractItemModel.C
  src/Wt/WAbstractItemView.h src/Wt/WAbstractItemView.C
  src/Wt/WAbstractListModel.h src/Wt/WAbstractListModel.C
  src/Wt/WAbstractMedia.h src/Wt/WAbstractMedia.C
  src/Wt/WAbstractProxyModel.h src/Wt/WAbstractProxyModel.C
  src/Wt/WAbstractSpinBox.h src/Wt/WAbstractSpinBox.C
  src/Wt/WAbstractTableModel.h src/Wt/WAbstractTableModel.C
  src/Wt/WAbstractToggleButton.h src/Wt/WAbstractToggleButton.C
  src/Wt/WAggregateProxyModel.h src/Wt/WAggregateProxyModel.C
  src/Wt/WAnchor.h src/Wt/WAnchor.C
  src/Wt/WAnimation.h src/Wt/WAnimation.C
  src/Wt/WAny.h src/Wt/WAny.C
  src/Wt/WApplication.h src/Wt/WApplication.C
  src/Wt/WAudio.h src/Wt/WAudio.C
  src/Wt/WBatchEditProxyModel.h src/Wt/WBatchEditProxyModel.C
  src/Wt/WBootstrapTheme.h src/Wt/WBootstrapTheme.C
  src/Wt/WBootstrap2Theme.h src/Wt/WBootstrap2Theme.C
  src/Wt/WBootstrap3Theme.h src/Wt/WBootstrap3Theme.C
  src/Wt/WBootstrap5Theme.h src/Wt/WBootstrap5Theme.C
  src/Wt/WBorder.h src/Wt/WBorder.C
  src/Wt/WBorderLayout.h src/Wt/WBorderLayout.C
  src/Wt/WBoxLayout.h src/Wt/WBoxLayout.C
  src/Wt/WBreak.h src/Wt/WBreak.C
  src/Wt/WBrush.h src/Wt/WBrush.C
  src/Wt/WButtonGroup.h src/Wt/WButtonGroup.C
  src/Wt/WCalendar.h src/Wt/WCalendar.C
  src/Wt/WCanvasPaintDevice.h src/Wt/WCanvasPaintDevice.C
  src/Wt/WCheckBox.h src/Wt/WCheckBox.C
  src/Wt/WCircleArea.h src/Wt/WCircleArea.C
  src/Wt/WColor.h src/Wt/WColor.C
  src/Wt/WColorPicker.h src/Wt/WColorPicker.C
  src/Wt/WCombinedLocalizedStrings.h src/Wt/WCombinedLocalizedStrings.C
  src/Wt/WComboBox.h src/Wt/WComboBox.C
  src/Wt/WCompositeWidget.h src/Wt/WCompositeWidget.C
  src/Wt/WContainerWidget.h src/Wt/WContainerWidget.C
  src/Wt/WCssDecorationStyle.h src/Wt/WCssDecorationStyle.C
  src/Wt/WCssStyleSheet.h src/Wt/WCssStyleSheet.C
  src/Wt/WCssTheme.h src/Wt/WCssTheme.C
  src/Wt/WDate.h src/Wt/WDate.C
  src/Wt/WDateEdit.h src/Wt/WDateEdit.C
  src/Wt/WDatePicker.h src/Wt/WDatePicker.C
  src/Wt/WDateTime.h src/Wt/WDateTime.C
  src/Wt/WDateValidator.h src/Wt/WDateValidator.C
  src/Wt/WDefaultLoadingIndicator.h src/Wt/WDefaultLoadingIndicator.C
  src/Wt/WDialog.h src/Wt/WDialog.C
  src/Wt/WDoubleSpinBox.h src/Wt/WDoubleSpinBox.C
  src/Wt/WDoubleValidator.h src/Wt/WDoubleValidator.C
  src/Wt/WEnvironment.h src/Wt/WEnvironment.C
  src/Wt/WEvent.h src/Wt/WEvent.C
  src/Wt/WException.h src/Wt/WException.C
  src/Wt/WFileDropWidget.h src/Wt/WFileDropWidget.C
  src/Wt/WFileResource.h src/Wt/WFileResource.C
  src/Wt/WFileUpload.h src/Wt/WFileUpload.C
  src/Wt/WFitLayout.h src/Wt/WFitLayout.C
  src/Wt/WFlashObject.h src/Wt/WFlashObject.C
  src/Wt/WFont.h src/Wt/WFont.C
  src/Wt/WFontMetrics.h src/Wt/WFontMetrics.C
  src/Wt/WFormModel.h src/Wt/WFormModel.C
  src/Wt/WFormWidget.h src/Wt/WFormWidget.C
  src/Wt/WGLWidget.h src/Wt/WGLWidget.C
  src/Wt/WAbstractGLImplementation.h src/Wt/WAbstractGLImplementation.C
  src/Wt/WClientGLWidget.h src/Wt/WClientGLWidget.C
  src/Wt/WGoogleMap.h src/Wt/WGoogleMap.C
  src/Wt/WGradient.h src/Wt/WGradient.C
  src/Wt/WGridLayout.h src/Wt/WGridLayout.C
  src/Wt/WGroupBox.h src/Wt/WGroupBox.C
  src/Wt/WHBoxLayout.h src/Wt/WHBoxLayout.C
  src/Wt/WIcon.h src/Wt/WIcon.C
  src/Wt/WIconPair.h src/Wt/WIconPair.C
  src/Wt/WImage.h src/Wt/WImage.C
  src/Wt/WInPlaceEdit.h src/Wt/WInPlaceEdit.C
  src/Wt/WIntValidator.h src/Wt/WIntValidator.C
  src/Wt/WInteractWidget.h src/Wt/WInteractWidget.C
  src/Wt/WIOService.h src/Wt/WIOService.C
  src/Wt/WLocale.h src/Wt/WLocale.C
  src/Wt/WLocalDateTime.h src/Wt/WLocalDateTime.C
  src/Wt/WIdentityProxyModel.h src/Wt/WIdentityProxyModel.C
  src/Wt/WItemDelegate.h src/Wt/WItemDelegate.C
  src/Wt/WItemSelectionModel.h src/Wt/WItemSelectionModel.C
  src/Wt/WJavaScript.h src/Wt/WJavaScript.C
  src/Wt/WJavaScriptExposableObject.h src/Wt/WJavaScriptExposableObject.C
  src/Wt/WJavaScriptHandle.h src/Wt/WJavaScriptHandle.C
  src/Wt/WJavaScriptObjectStorage.h src/Wt/WJavaScriptObjectStorage.C
  src/Wt/WJavaScriptPreamble.h src/Wt/WJavaScriptPreamble.C
  src/Wt/WJavaScriptSlot.h src/Wt/WJavaScriptSlot.C
  src/Wt/WLabel.h src/Wt/WLabel.C
  src/Wt/WLayout.h src/Wt/WLayout.C
  src/Wt/WLayoutImpl.h src/Wt/WLayoutImpl.C
  src/Wt/WLayoutItem.h src/Wt/WLayoutItem.C
  src/Wt/WLayoutItemImpl.h src/Wt/WLayoutItemImpl.C
  src/Wt/WLeafletMap.h src/Wt/WLeafletMap.C
  src/Wt/WLength.h src/Wt/WLength.C
  src/Wt/WLengthValidator.h src/Wt/WLengthValidator.C
  src/Wt/WLineEdit.h src/Wt/WLineEdit.C
  src/Wt/WLineF.h src/Wt/WLineF.C
  src/Wt/WLink.h src/Wt/WLink.C
  src/Wt/WLinkedCssStyleSheet.h src/Wt/WLinkedCssStyleSheet.C
  src/Wt/WLoadingIndicator.h src/Wt/WLoadingIndicator.C
  src/Wt/WLocalizedStrings.h src/Wt/WLocalizedStrings.C
  src/Wt/WLogger.h src/Wt/WLogger.C
  src/Wt/WLogSink.h src/Wt/WLogSink.C
  src/Wt/WMatrix4x4.h src/Wt/WMatrix4x4.C
  src/Wt/WMeasurePaintDevice.h src/Wt/WMeasurePaintDevice.C
  src/Wt/WMediaPlayer.h src/Wt/WMediaPlayer.C
  src/Wt/WMemoryResource.h src/Wt/WMemoryResource.C
  src/Wt/WMenu.h src/Wt/WMenu.C
  src/Wt/WMenuItem.h src/Wt/WMenuItem.C
  src/Wt/WMessageBox.h src/Wt/WMessageBox.C
  src/Wt/WMessageResourceBundle.h src/Wt/WMessageResourceBundle.C
  src/Wt/WMessageResources.h src/Wt/WMessageResources.C
  src/Wt/WModelIndex.h src/Wt/WModelIndex.C
  src/Wt/WNavigationBar.h src/Wt/WNavigationBar.C
  src/Wt/WObject.h src/Wt/WObject.C
  src/Wt/WOverlayLoadingIndicator.h src/Wt/WOverlayLoadingIndicator.C
  src/Wt/WPaintDevice.h src/Wt/WPaintDevice.C
  src/Wt/WPaintedWidget.h src/Wt/WPaintedWidget.C
  src/Wt/WPainter.h src/Wt/WPainter.C
  src/Wt/WPainterPath.h src/Wt/WPainterPath.C
  src/Wt/WPanel.h src/Wt/WPanel.C
  src/Wt/WPen.h src/Wt/WPen.C
  src/Wt/WPoint.h src/Wt/WPoint.C
  src/Wt/WPointF.h src/Wt/WPointF.C
  src/Wt/WPolygonArea.h src/Wt/WPolygonArea.C
  src/Wt/WPopupMenu.h src/Wt/WPopupMenu.C
  src/Wt/WPopupWidget.h src/Wt/WPopupWidget.C
  src/Wt/WProgressBar.h src/Wt/WProgressBar.C
  src/Wt/WPushButton.h src/Wt/WPushButton.C
  src/Wt/WRadioButton.h src/Wt/WRadioButton.C
  src/Wt/WRandom.h src/Wt/WRandom.C
  src/Wt/WReadOnlyProxyModel.h src/Wt/WReadOnlyProxyModel.C
  src/Wt/WRectArea.h src/Wt/WRectArea.C
  src/Wt/WRectF.h src/Wt/WRectF.C
  src/Wt/WRegExpValidator.h src/Wt/WRegExpValidator.C
  src/Wt/WResource.h src/Wt/WResource.C
  src/Wt/WSelectionBox.h src/Wt/WSelectionBox.C
  src/Wt/WServer.h src/Wt/WServer.C
  src/Wt/WShadow.h src/Wt/WShadow.C
  src/Wt/WSignal.h src/Wt/WSignal.C
  src/Wt/WSlider.h src/Wt/WSlider.C
  src/Wt/WSocketNotifier.h src/Wt/WSocketNotifier.C
  src/Wt/WSortFilterProxyModel.h src/Wt/WSortFilterProxyModel.C
  src/Wt/WSpinBox.h src/Wt/WSpinBox.C
  src/Wt/WSplitButton.h src/Wt/WSplitButton.C
  src/Wt/WSound.h src/Wt/WSound.C
  src/Wt/WSslCertificate.h src/Wt/WSslCertificate.C
  src/Wt/WSslInfo.h src/Wt/WSslInfo.C
  src/Wt/WStackedWidget.h src/Wt/WStackedWidget.C
  src/Wt/WStandardItem.h src/Wt/WStandardItem.C
  src/Wt/WStandardItemModel.h src/Wt/WStandardItemModel.C
  src/Wt/WStatelessSlot.h src/Wt/WStatelessSlot.C
  src/Wt/WString.h src/Wt/WString.C
  src/Wt/WStreamResource.h src/Wt/WStreamResource.C
  src/Wt/WStringListModel.h src/Wt/WStringListModel.C
  src/Wt/WStringStream.h src/Wt/WStringStream.C
  src/Wt/WStringUtil.h src/Wt/WStringUtil.C
  src/Wt/WSuggestionPopup.h src/Wt/WSuggestionPopup.C
  src/Wt/WSvgImage.h src/Wt/WSvgImage.C
  src/Wt/WTabWidget.h src/Wt/WTabWidget.C
  src/Wt/WTable.h src/Wt/WTable.C
  src/Wt/WTableCell.h src/Wt/WTableCell.C
  src/Wt/WTableColumn.h src/Wt/WTableColumn.C
  src/Wt/WTableRow.h src/Wt/WTableRow.C
  src/Wt/WTemplate.h src/Wt/WTemplate.C
  src/Wt/WTemplateFormView.h src/Wt/WTemplateFormView.C
  src/Wt/WTableView.h src/Wt/WTableView.C
  src/Wt/WText.h src/Wt/WText.C
  src/Wt/WTextArea.h src/Wt/WTextArea.C
  src/Wt/WTextEdit.h src/Wt/WTextEdit.C
  src/Wt/WTheme.h src/Wt/WTheme.C
  src/Wt/WTime.h src/Wt/WTime.C
  src/Wt/WTimeEdit.h src/Wt/WTimeEdit.C
  src/Wt/WTimer.h src/Wt/WTimer.C
  src/Wt/WTimerWidget.h src/Wt/WTimerWidget.C
  src/Wt/WTimePicker.h src/Wt/WTimePicker.C
  src/Wt/WTimeValidator.h src/Wt/WTimeValidator.C
  src/Wt/WToolBar.h src/Wt/WToolBar.C
  src/Wt/WTransform.h src/Wt/WTransform.C
  src/Wt/WTree.h src/Wt/WTree.C
  src/Wt/WTreeNode.h src/Wt/WTreeNode.C
  src/Wt/WTreeTable.h src/Wt/WTreeTable.C
  src/Wt/WTreeTableNode.h src/Wt/WTreeTableNode.C
  src/Wt/WTreeView.h src/Wt/WTreeView.C
  src/Wt/WValidator.h src/Wt/WValidator.C
  src/Wt/WVBoxLayout.h src/Wt/WVBoxLayout.C
  src/Wt/WVector3.h src/Wt/WVector3.C
  src/Wt/WVector4.h src/Wt/WVector4.C
  src/Wt/WVectorImage.h src/Wt/WVectorImage.C
  src/Wt/WVideo.h src/Wt/WVideo.C
  src/Wt/WVmlImage.h src/Wt/WVmlImage.C
  src/Wt/WViewWidget.h src/Wt/WViewWidget.C
  src/Wt/WVirtualImage.h src/Wt/WVirtualImage.C
  src/Wt/WWebWidget.h src/Wt/WWebWidget.C
  src/Wt/WWidget.h src/Wt/WWidget.C
  src/Wt/WWidgetItem.h src/Wt/WWidgetItem.C
  src/Wt/WWidgetItemImpl.h src/Wt/WWidgetItemImpl.C
  src/Wt/Auth/AbstractPasswordService.h src/Wt/Auth/AbstractPasswordService.C
  src/Wt/Auth/AbstractUserDatabase.h src/Wt/Auth/AbstractUserDatabase.C
  src/Wt/Auth/AuthModel.h src/Wt/Auth/AuthModel.C
  src/Wt/Auth/AuthService.h src/Wt/Auth/AuthService.C
  src/Wt/Auth/AuthWidget.h src/Wt/Auth/AuthWidget.C
  src/Wt/Auth/FacebookService.h src/Wt/Auth/FacebookService.C
  src/Wt/Auth/FormBaseModel.h src/Wt/Auth/FormBaseModel.C
  src/Wt/Auth/GoogleService.h src/Wt/Auth/GoogleService.C
  src/Wt/Auth/HashFunction.h src/Wt/Auth/HashFunction.C
  src/Wt/Auth/Identity.h src/Wt/Auth/Identity.C
  src/Wt/Auth/Login.h src/Wt/Auth/Login.C
  src/Wt/Auth/LostPasswordWidget.h src/Wt/Auth/LostPasswordWidget.C
  src/Wt/Auth/OAuthService.h src/Wt/Auth/OAuthService.C
  src/Wt/Auth/OAuthWidget.h src/Wt/Auth/OAuthWidget.C
  src/Wt/Auth/passwdqc.h src/Wt/Auth/passwdqc_check.c
  src/Wt/Auth/PasswordHash.h src/Wt/Auth/PasswordHash.C
  src/Wt/Auth/PasswordPromptDialog.h src/Wt/Auth/PasswordPromptDialog.C
  src/Wt/Auth/PasswordService.h src/Wt/Auth/PasswordService.C
  src/Wt/Auth/PasswordStrengthValidator.h src/Wt/Auth/PasswordStrengthValidator.C
  src/Wt/Auth/PasswordVerifier.h src/Wt/Auth/PasswordVerifier.C
  src/Wt/Auth/RegistrationModel.h src/Wt/Auth/RegistrationModel.C
  src/Wt/Auth/RegistrationWidget.h src/Wt/Auth/RegistrationWidget.C
  src/Wt/Auth/Token.h src/Wt/Auth/Token.C
  src/Wt/Auth/UpdatePasswordWidget.h src/Wt/Auth/UpdatePasswordWidget.C
  src/Wt/Auth/User.h src/Wt/Auth/User.C
  src/Wt/Auth/AuthUtils.h src/Wt/Auth/AuthUtils.C
  src/Wt/Auth/MailUtils.h src/Wt/Auth/MailUtils.C
  src/Wt/Auth/OAuthClient.h src/Wt/Auth/OAuthClient.C
  src/Wt/Auth/OAuthTokenEndpoint.h src/Wt/Auth/OAuthTokenEndpoint.C
  src/Wt/Auth/OidcUserInfoEndpoint.h src/Wt/Auth/OidcUserInfoEndpoint.C
  src/Wt/Auth/OAuthAuthorizationEndpointProcess.h src/Wt/Auth/OAuthAuthorizationEndpointProcess.C
  src/Wt/Auth/IssuedToken.h src/Wt/Auth/IssuedToken.C
  src/Wt/Auth/OidcService.h src/Wt/Auth/OidcService.C
  src/Wt/Chart/WAbstractChart.h src/Wt/Chart/WAbstractChart.C
  src/Wt/Chart/WAbstractChartModel.h src/Wt/Chart/WAbstractChartModel.C
  src/Wt/Chart/WAxis.h src/Wt/Chart/WAxis.C
  src/Wt/Chart/WAxisSliderWidget.h src/Wt/Chart/WAxisSliderWidget.C
  src/Wt/Chart/WDataSeries.h src/Wt/Chart/WDataSeries.C
  src/Wt/Chart/WPieChart.h src/Wt/Chart/WPieChart.C
  src/Wt/Chart/WCartesianChart.h src/Wt/Chart/WCartesianChart.C
  src/Wt/Chart/WCartesian3DChart.h src/Wt/Chart/WCartesian3DChart.C
  src/Wt/Chart/WAbstractChartImplementation.h src/Wt/Chart/WAbstractChartImplementation.C
  src/Wt/Chart/WChart2DImplementation.h src/Wt/Chart/WChart2DImplementation.C
  src/Wt/Chart/WChart3DImplementation.h src/Wt/Chart/WChart3DImplementation.C
  src/Wt/Chart/WChartPalette.h src/Wt/Chart/WChartPalette.C
  src/Wt/Chart/WStandardChartProxyModel.h src/Wt/Chart/WStandardChartProxyModel.C
  src/Wt/Chart/WStandardPalette.h src/Wt/Chart/WStandardPalette.C
  src/Wt/Chart/WStandardColorMap.h src/Wt/Chart/WStandardColorMap.C
  src/Wt/Chart/WLegend.h src/Wt/Chart/WLegend.C
  src/Wt/Chart/WLegend3D.h src/Wt/Chart/WLegend3D.C
  src/Wt/Chart/WAbstractDataSeries3D.h src/Wt/Chart/WAbstractDataSeries3D.C
  src/Wt/Chart/WAbstractGridData.h src/Wt/Chart/WAbstractGridData.C
  src/Wt/Chart/WGridData.h src/Wt/Chart/WGridData.C
  src/Wt/Chart/WEquidistantGridData.h src/Wt/Chart/WEquidistantGridData.C
  src/Wt/Chart/WScatterData.h src/Wt/Chart/WScatterData.C
  src/Wt/Chart/WSelection.h src/Wt/Chart/WSelection.C
  src/Wt/Core/observable.hpp src/Wt/Core/observable.cpp
  src/Wt/Core/observing_ptr.hpp src/Wt/Core/observing_ptr.cpp
  src/Wt/cpp17/any.hpp src/Wt/cpp17/any/any.hpp
  src/Wt/cpp20/date.hpp src/Wt/cpp20/tz.hpp
  src/Wt/Date/date.h src/Wt/Date/include/date/date.h
  src/Wt/Date/tz.h src/Wt/Date/include/date/tz.h src/Wt/Date/src/tz.cpp

  src/Wt/Form/WAbstractFormDelegate.h src/Wt/Form/WAbstractFormDelegate.C
  src/Wt/Form/WFormDelegate.h src/Wt/Form/WFormDelegate.C
  src/Wt/Json/Array.h src/Wt/Json/Array.C
  src/Wt/Json/Object.h src/Wt/Json/Object.C
  src/Wt/Json/Parser.h src/Wt/Json/Parser.C
  src/Wt/Json/Serializer.h src/Wt/Json/Serializer.C
  src/Wt/Json/Value.h src/Wt/Json/Value.C
  src/Wt/Http/HttpUtils.h src/Wt/Http/HttpUtils.C
  src/Wt/Http/Client.h src/Wt/Http/Client.C
  src/Wt/Http/Cookie.h src/Wt/Http/Cookie.C
  src/Wt/Http/Message.h src/Wt/Http/Message.C
  src/Wt/Http/Request.h src/Wt/Http/Request.C
  src/Wt/Http/Response.h src/Wt/Http/Response.C
  src/Wt/Http/ResponseContinuation.h src/Wt/Http/ResponseContinuation.C
  src/Wt/Http/WtClient.h src/Wt/Http/WtClient.C
  src/Wt/Mail/Client.h src/Wt/Mail/Client.C
  src/Wt/Mail/Mailbox.h src/Wt/Mail/Mailbox.C
  src/Wt/Mail/Message.h src/Wt/Mail/Message.C
  src/Wt/Payment/Address.h src/Wt/Payment/Address.C
  src/Wt/Payment/PayPal.h src/Wt/Payment/PayPal.C
  src/Wt/Payment/Customer.h src/Wt/Payment/Customer.C
  src/Wt/Payment/Money.h src/Wt/Payment/Money.C
  src/Wt/Payment/Order.h src/Wt/Payment/Order.C
  src/Wt/Payment/OrderItem.h src/Wt/Payment/OrderItem.C
  src/Wt/Payment/Result.h src/Wt/Payment/Result.C
  src/Wt/Render/Block.h src/Wt/Render/Block.C
  src/Wt/Render/CssData.h src/Wt/Render/CssData.C
  src/Wt/Render/CssData_p.h src/Wt/Render/CssData_p.C
  src/Wt/Render/CssParser.h src/Wt/Render/CssParser.C
  src/Wt/Render/Line.h src/Wt/Render/Line.C
  src/Wt/Render/LayoutBox.h src/Wt/Render/LayoutBox.C
  src/Wt/Render/RenderUtils.h src/Wt/Render/RenderUtils.C
  src/Wt/Render/Specificity.h src/Wt/Render/Specificity.C
  src/Wt/Render/WTextRenderer.h src/Wt/Render/WTextRenderer.C
  src/Wt/Signals/signals.hpp src/Wt/Signals/signals.cpp
  src/web/md5.h src/web/md5.c
  src/web/sha1.h src/web/sha1.c
  src/web/CgiParser.h src/web/CgiParser.C
  src/web/Configuration.h src/web/Configuration.C
  src/web/DomElement.h src/web/DomElement.C
  src/web/EntryPoint.h src/web/EntryPoint.C
  src/web/EscapeOStream.h src/web/EscapeOStream.C
  src/web/FileServe.h src/web/FileServe.C
  src/web/ColorUtils.h src/web/ColorUtils.C
  src/web/ImageUtils.h src/web/ImageUtils.C
  src/web/RefEncoder.h src/web/RefEncoder.C
  src/web/SoundManager.h src/web/SoundManager.C
  src/web/WebController.h src/web/WebController.C
  src/web/WebMain.h src/web/WebMain.C
  src/web/WebRequest.h src/web/WebRequest.C
  src/web/WebStream.h src/web/WebStream.C
  src/web/WebSession.h src/web/WebSession.C
  src/web/WebSocketMessage.h src/web/WebSocketMessage.C
  src/web/WebRenderer.h src/web/WebRenderer.C
  src/web/WebUtils.h src/web/WebUtils.C
  src/web/FileUtils.h src/web/FileUtils.C
  src/web/PdfUtils.h src/web/PdfUtils.C
  src/web/StringUtils.h src/web/StringUtils.cpp
  src/web/TimeUtil.h src/web/TimeUtil.C
  src/web/XSSFilter.h src/web/XSSFilter.C
  src/web/XSSUtils.h src/web/XSSUtils.C
  src/web/SslUtils.h src/web/SslUtils.C
  src/web/UriUtils.h src/web/UriUtils.C
  src/web/base64.h src/web/base64.cpp
  src/web/SocketNotifier.h src/web/SocketNotifier.C
  Plain_html.C
  Boot_html.C
  Hybrid_html.C
  Boot_js.C
  Wt_js.C
  WCartesianChart_js.C
  Wt_xml.C
  AuthStrings_xml.C
  BootstrapTheme_xml.C
  Bootstrap3Theme_xml.C
  Bootstrap5Theme_xml.C
  AuthCssTheme_xml.C

  src/Wt/Auth/bcrypt/crypt_blowfish.h src/Wt/Auth/bcrypt/crypt_blowfish.c
  src/Wt/Auth/bcrypt/crypt_gensalt.h src/Wt/Auth/bcrypt/crypt_gensalt.c
  src/Wt/Auth/bcrypt/wrapper.c
  )

add_library (Seastar::seastar ALIAS seastar)

add_dependencies (seastar
  seastar_http_chunk_parsers
  seastar_http_request_parser
  seastar_http_response_parser)

target_include_directories (seastar
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${Seastar_GEN_BINARY_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

set (Seastar_PRIVATE_CXX_FLAGS
  -UNDEBUG
  -Wall
  #-Werror
  -Wno-array-bounds # Disabled because of https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93437
  -Wno-error=deprecated-declarations)
if (NOT BUILD_SHARED_LIBS)
  list (APPEND Seastar_PRIVATE_CXX_FLAGS -fvisibility=hidden)
endif ()

if (Seastar_COMPRESS_DEBUG)
  # -gz doesn't imply -g, so it is safe to add it regardless of debug
  # info being enabled.
  list (APPEND Seastar_PRIVATE_CXX_FLAGS -gz)
endif()

target_link_libraries (seastar
  PUBLIC
    Boost::boost
    Boost::program_options
    Boost::thread
    Boost::filesystem
    c-ares::cares
    cryptopp::cryptopp
    fmt::fmt
    lz4::lz4
    SourceLocation::source_location
  PRIVATE
    ${CMAKE_DL_LIBS}
    GnuTLS::gnutls
    StdAtomic::atomic
    lksctp-tools::lksctp-tools
    rt::rt
    yaml-cpp::yaml-cpp
    "$<BUILD_INTERFACE:Valgrind::valgrind>"
    Threads::Threads)

set (Seastar_SANITIZE_MODES "Debug" "Sanitize")
if ((Seastar_SANITIZE STREQUAL "ON") OR
    ((Seastar_SANITIZE STREQUAL "DEFAULT") AND
     (CMAKE_BUILD_TYPE IN_LIST Seastar_SANITIZE_MODES)))
  if (NOT Sanitizers_FOUND)
    message (FATAL_ERROR "Sanitizers not found!")
  endif ()
  set (Seastar_Sanitizers_OPTIONS ${Sanitizers_COMPILER_OPTIONS})
  target_link_libraries (seastar
    PUBLIC
      Sanitizers::address
      Sanitizers::undefined_behavior)
endif ()

# We only need valgrind to find uninitialized memory uses, so disable
# the leak sanitizer.
# To test with valgrind run "ctest -T memcheck"
set( MEMORYCHECK_COMMAND_OPTIONS "--error-exitcode=1 --leak-check=no --trace-children=yes" )
include (CTest)

#
# We want asserts enabled on all modes, but cmake defaults to passing
# -DNDEBUG in some modes. We add -UNDEBUG to our private options to
# reenable it. To force asserts off pass -DNDEBUG in
# Seastar_CXX_FLAGS.
#
# To disable -Werror, pass -Wno-error to Seastar_CXX_FLAGS.
#
# We disable _FORTIFY_SOURCE because it generates false positives with longjmp() (src/core/thread.cc)
#

target_compile_options (seastar
  PUBLIC
    -U_FORTIFY_SOURCE)

target_compile_definitions(seastar
  PUBLIC
  SEASTAR_API_LEVEL=${Seastar_API_LEVEL}
  $<$<BOOL:${BUILD_SHARED_LIBS}>:SEASTAR_BUILD_SHARED_LIBS>)

target_compile_features(seastar
  PUBLIC
    cxx_std_${CMAKE_CXX_STANDARD})

function (seastar_supports_flag flag var)
  set (CMAKE_REQUIRED_FLAGS "${flag}")
  check_cxx_source_compiles ("int main() { return 0; }" tmp_${var})
  set (${var} ${tmp_${var}} PARENT_SCOPE)
endfunction ()

seastar_supports_flag ("-Wno-maybe-uninitialized -Werror" MaybeUninitialized_FOUND)
if (MaybeUninitialized_FOUND)
  target_compile_options (seastar
    PUBLIC
      # With std::experimental::optional it is easy to hit
      # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88897.  We disable
      # -Wmaybe-uninitialized in here since otherwise we would have to
      # disable it on many types used inside optional<>.
      -Wno-maybe-uninitialized)
endif ()

if (Seastar_SSTRING)
  target_compile_options (seastar PUBLIC -DSEASTAR_SSTRING)
endif ()

if (LinuxMembarrier_FOUND)
  list (APPEND Seastar_PRIVATE_COMPILE_DEFINITIONS SEASTAR_HAS_MEMBARRIER)

  target_link_libraries (seastar
    PRIVATE LinuxMembarrier::membarrier)
endif ()

set (Seastar_ALLOC_FAILURE_INJECTION_MODES "Dev")
if ((Seastar_ALLOC_FAILURE_INJECTION IN_LIST True_STRING_VALUES) OR
    ((Seastar_ALLOC_FAILURE_INJECTION STREQUAL "DEFAULT") AND
     (CMAKE_BUILD_TYPE IN_LIST Seastar_ALLOC_FAILURE_INJECTION_MODES)))
  target_compile_definitions (seastar
    PUBLIC SEASTAR_ENABLE_ALLOC_FAILURE_INJECTION)
endif ()

if (Seastar_TASK_BACKTRACE)
  target_compile_definitions (seastar
    PUBLIC SEASTAR_TASK_BACKTRACE)
endif ()

if (Seastar_DEBUG_ALLOCATIONS)
  target_compile_definitions (seastar
    PRIVATE SEASTAR_DEBUG_ALLOCATIONS)
endif ()

if (Sanitizers_FIBER_SUPPORT)
  list (APPEND Seastar_PRIVATE_COMPILE_DEFINITIONS SEASTAR_HAVE_ASAN_FIBER_SUPPORT)
endif ()

if (Seastar_ALLOC_PAGE_SIZE)
  target_compile_definitions (seastar
    PUBLIC SEASTAR_OVERRIDE_ALLOCATOR_PAGE_SIZE=${Seastar_ALLOC_PAGE_SIZE})
endif ()

target_compile_definitions (seastar
  PUBLIC SEASTAR_SCHEDULING_GROUPS_COUNT=${Seastar_SCHEDULING_GROUPS_COUNT})

if (Seastar_CXX_FLAGS)
  list (APPEND Seastar_PRIVATE_CXX_FLAGS ${Seastar_CXX_FLAGS})
endif ()

# When using split dwarf --gdb-index is effectively required since
# otherwise gdb is just too slow. We also want to use split dwarf in
# as many compilation units as possible.  So while these flags don't
# have to be public, we don't expect anyone to want to build seastar
# with them and some client code without.
if (Seastar_SPLIT_DWARF AND (NOT (CMAKE_BUILD_TYPE STREQUAL "Dev")))
  set (Seastar_SPLIT_DWARF_FLAG "-Wl,--gdb-index")
  target_link_libraries (seastar PUBLIC ${Seastar_SPLIT_DWARF_FLAG})
  target_compile_options (seastar PUBLIC "-gsplit-dwarf")
endif ()

if (Seastar_HEAP_PROFILING)
    set_property (
      SOURCE "src/core/memory.cc"
      PROPERTY
      COMPILE_DEFINITIONS SEASTAR_HEAPPROF)
    set_property (
      SOURCE "src/core/reactor.cc"
      PROPERTY
      COMPILE_DEFINITIONS SEASTAR_HEAPPROF)
endif ()

if (Seastar_DEFERRED_ACTION_REQUIRE_NOEXCEPT)
  list (APPEND Seastar_PRIVATE_COMPILE_DEFINITIONS SEASTAR_DEFERRED_ACTION_REQUIRE_NOEXCEPT)
endif ()

if (Seastar_DPDK)
  #
  # The DPDK architecture needs to be transitively applied to consumers of Seastar as well.
  #

  set (Seastar_ARCH_FOR_native "native")
  set (Seastar_ARCH_FOR_nhm "nehalem")
  set (Seastar_ARCH_FOR_wsm "westmere")
  set (Seastar_ARCH_FOR_snb "sandybridge")
  set (Seastar_ARCH_FOR_ivb "ivybridge")
  set (Seastar_ARCH_FOR_armv8a "armv8-a")
  set (Seastar_ARCH ${Seastar_ARCH_FOR_${Seastar_DPDK_MACHINE}})

  if (NOT Seastar_ARCH)
    message (FATAL_ERROR "Unrecognized DPDK machine identifier: ${Seastar_DPDK_MACHINE}")
  endif ()

  target_compile_options (seastar
    PUBLIC -march=${Seastar_ARCH})

  target_compile_definitions (seastar
    PUBLIC SEASTAR_HAVE_DPDK)

  # No pmd driver code will be pulled in without "--whole-archive". To
  # avoid exposing that to seastar users, combine dpdk into a single
  # .o file.
  add_custom_command (
    OUTPUT seastar-dpdk.o
    COMMAND ld -r -o seastar-dpdk.o --whole-archive ${dpdk_LIBRARIES} --no-whole-archive
  )

  # This just provides the include path to cmake
  target_link_libraries (seastar
    PUBLIC dpdk::dpdk)
endif ()

if (Seastar_HWLOC)
  list (APPEND Seastar_PRIVATE_COMPILE_DEFINITIONS SEASTAR_HAVE_HWLOC)

  target_link_libraries (seastar
    PRIVATE hwloc::hwloc)
endif ()

set_option_if_package_is_found (Seastar_IO_URING LibUring)
if (Seastar_IO_URING)
  list (APPEND Seastar_PRIVATE_COMPILE_DEFINITIONS SEASTAR_HAVE_URING)
  target_link_libraries (seastar
    PRIVATE URING::uring)
endif ()

if (Seastar_LD_FLAGS)
  target_link_options (seastar
    PRIVATE ${Seastar_LD_FLAGS})
endif ()

if (Seastar_NUMA)
  list (APPEND Seastar_PRIVATE_COMPILE_DEFINITIONS SEASTAR_HAVE_NUMA)

  target_link_libraries (seastar
    PRIVATE numactl::numactl)
endif ()

seastar_supports_flag ("-Werror=unused-result" ErrorUnused_FOUND)
if (ErrorUnused_FOUND)
  if (Seastar_UNUSED_RESULT_ERROR)
    target_compile_options (seastar
      PUBLIC -Werror=unused-result)
  else()
    target_compile_options (seastar
      PUBLIC -Wno-error=unused-result)
  endif ()
endif ()

seastar_supports_flag ("-Wno-error=#warnings" ErrorWarnings_FOUND)
if (ErrorWarnings_FOUND)
  target_compile_options (seastar
      PUBLIC "-Wno-error=#warnings")
endif ()

if ((CMAKE_BUILD_TYPE STREQUAL "Debug") OR (CMAKE_BUILD_TYPE STREQUAL "Sanitize"))
  target_compile_definitions (seastar
    PUBLIC
      SEASTAR_DEBUG
      SEASTAR_DEFAULT_ALLOCATOR
      SEASTAR_SHUFFLE_TASK_QUEUE)
endif ()

set (Seastar_DEBUG_SHARED_PTR_MODES "Debug" "Sanitize")
if (Seastar_DEBUG_SHARED_PTR IN_LIST True_STRING_VALUES OR
    ((Seastar_DEBUG_SHARED_PTR IN_LIST Default_STRING_VALUES) AND
     (CMAKE_BUILD_TYPE IN_LIST Seastar_DEBUG_SHARED_PTR_MODES)))
  target_compile_definitions (seastar
    PUBLIC
      SEASTAR_DEBUG_SHARED_PTR)
endif ()

set (Seastar_STACK_GUARD_MODES "Debug" "Sanitize" "Dev")
if ((Seastar_STACK_GUARDS STREQUAL "ON") OR
    ((Seastar_STACK_GUARDS STREQUAL "DEFAULT") AND
     (CMAKE_BUILD_TYPE IN_LIST Seastar_STACK_GUARD_MODES)))
  # check for -fstack-clash-protection together with -Werror, because
  # otherwise clang can soft-fail (return 0 but emit a warning) instead.
  seastar_supports_flag ("-fstack-clash-protection -Werror" StackClashProtection_FOUND)
  if (StackClashProtection_FOUND)
    target_compile_options (seastar
      PUBLIC
      -fstack-clash-protection)
  endif ()
  target_compile_definitions (seastar
    PRIVATE
      SEASTAR_THREAD_STACK_GUARDS)
endif ()

if ((CMAKE_BUILD_TYPE STREQUAL "Dev") OR (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  target_compile_definitions (seastar
    PUBLIC
      SEASTAR_TYPE_ERASE_MORE)
endif ()

target_compile_definitions (seastar
  PRIVATE ${Seastar_PRIVATE_COMPILE_DEFINITIONS})

target_compile_options (seastar
  PRIVATE ${Seastar_PRIVATE_CXX_FLAGS})

set_target_properties (seastar
  PROPERTIES
    CXX_STANDARD ${CMAKE_CXX_STANDARD}
    CXX_EXTENSIONS ON)

add_library (seastar_private INTERFACE)

target_compile_definitions (seastar_private
  INTERFACE ${Seastar_PRIVATE_COMPILE_DEFINITIONS})

target_compile_options (seastar_private
  INTERFACE ${Seastar_PRIVATE_CXX_FLAGS})

target_link_libraries (seastar_private
  INTERFACE seastar)

#
# The testing library.
#

if (Seastar_INSTALL OR Seastar_TESTING)
  add_library (seastar_testing
    include/seastar/testing/entry_point.hh
    include/seastar/testing/exchanger.hh
    include/seastar/testing/random.hh
    include/seastar/testing/seastar_test.hh
    include/seastar/testing/test_case.hh
    include/seastar/testing/test_runner.hh
    include/seastar/testing/thread_test_case.hh
    src/testing/entry_point.cc
    src/testing/random.cc
    src/testing/seastar_test.cc
    src/testing/test_runner.cc)

  add_library (Seastar::seastar_testing ALIAS seastar_testing)

  target_compile_definitions (seastar_testing
    PRIVATE ${Seastar_PRIVATE_COMPILE_DEFINITIONS})

  target_compile_options (seastar_testing
    PRIVATE ${Seastar_PRIVATE_CXX_FLAGS})

  target_link_libraries (seastar_testing
    PUBLIC
      Boost::unit_test_framework
      Boost::dynamic_linking
      seastar)

  add_library(seastar_perf_testing
    src/testing/random.cc
    include/seastar/testing/perf_tests.hh
    tests/perf/perf_tests.cc
    tests/perf/linux_perf_event.cc)
  add_library (Seastar::seastar_perf_testing ALIAS seastar_perf_testing)
  target_compile_definitions (seastar_perf_testing
    PRIVATE ${Seastar_PRIVATE_COMPILE_DEFINITIONS})
  target_compile_options (seastar_perf_testing
    PRIVATE ${Seastar_PRIVATE_CXX_FLAGS})
  target_link_libraries (seastar_perf_testing
    PUBLIC
    seastar)

endif ()

#
# The tests themselves.
#

if (Seastar_TESTING)
  enable_testing ()

  if (Seastar_EXCLUDE_TESTS_FROM_ALL)
    set (exclude EXCLUDE_FROM_ALL)
  else ()
    set (exclude "")
  endif ()

  add_subdirectory (tests ${exclude})
endif ()

#
# Demonstrations.
#

if (Seastar_DEMOS)
  if (Seastar_EXCLUDE_DEMOS_FROM_ALL)
    set (exclude EXCLUDE_FROM_ALL)
  else ()
    set (exclude "")
  endif ()

  add_subdirectory (demos ${exclude})
endif ()

#
# Documentation.
#

if (Seastar_DOCS)
  add_subdirectory (doc)
endif ()

#
# Applications.
#

if (Seastar_APPS)
  if (Seastar_EXCLUDE_APPS_FROM_ALL)
    set (exclude EXCLUDE_FROM_ALL)
  else ()
    set (exclude "")
  endif ()

  add_subdirectory (apps ${exclude})
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Dev")
  include (CheckHeaders)
  add_custom_target (checkheaders)
  foreach (lib seastar seastar_testing seastar_perf_testing)
    if (TARGET ${lib})
      seastar_check_self_contained (${lib}
        INCLUDE "\\.hh$"
        # impl.hh headers are internal implementations of .hh, so they are not
        # compilable. let's exclude them from the files to be checked.
        EXCLUDE "_impl.hh$|-impl.hh$")
    endif ()
  endforeach ()
endif ()

#
# Installation and export.
#

if (Seastar_INSTALL)
  #
  # pkg-config generation.
  #
  # Note that unlike the CMake "config module", this description is not relocatable because
  # some dependencies do not natively support pkg-config.
  #

  # Necessary here for pkg-config.
  include (GNUInstallDirs)

  # Set paths in pkg-config files for installation.
  set (Seastar_PKG_CONFIG_PREFIX ${CMAKE_INSTALL_PREFIX})
  set (Seastar_PKG_CONFIG_LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
  set (Seastar_PKG_CONFIG_SEASTAR_INCLUDE_FLAGS "-I${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")

  configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/seastar.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/seastar-install.pc.in
    @ONLY)

  configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/seastar-testing.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/seastar-testing-install.pc.in
    @ONLY)

  # Set paths in pkg-config files for direct use in the build directory.
  set (Seastar_PKG_CONFIG_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
  set (Seastar_PKG_CONFIG_LIBDIR ${CMAKE_CURRENT_BINARY_DIR})
  set (Seastar_PKG_CONFIG_SEASTAR_INCLUDE_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/include -I${CMAKE_CURRENT_BINARY_DIR}/gen/include")

  configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/seastar.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/seastar.pc.in
    @ONLY)

  configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/seastar-testing.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/seastar-testing.pc.in
    @ONLY)

  file (GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/seastar.pc
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/seastar.pc.in)

  file (GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/seastar-testing.pc
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/seastar-testing.pc.in)

  file (GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/seastar-install.pc
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/seastar-install.pc.in)

  file (GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/seastar-testing-install.pc
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/seastar-testing-install.pc.in)

  include (CMakePackageConfigHelpers)
  set (install_cmakedir ${CMAKE_INSTALL_LIBDIR}/cmake/Seastar)

  install (
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install (
    DIRECTORY ${Seastar_GEN_BINARY_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install (
    PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/seastar-json2code.py
    DESTINATION ${CMAKE_INSTALL_BINDIR})

  install (
    TARGETS
      seastar
      seastar_testing
      seastar_perf_testing
    EXPORT seastar-export
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install (
    EXPORT seastar-export
    FILE SeastarTargets.cmake
    NAMESPACE Seastar::
    DESTINATION ${install_cmakedir})

  write_basic_package_version_file (
    ${CMAKE_CURRENT_BINARY_DIR}/SeastarConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion)

  configure_package_config_file (
    ${CMAKE_CURRENT_LIST_DIR}/cmake/SeastarConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SeastarConfig.cmake
    INSTALL_DESTINATION ${install_cmakedir})

  install (
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/SeastarConfig.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/SeastarConfigVersion.cmake
    DESTINATION ${install_cmakedir})

  install (
    FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindGnuTLS.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindLinuxMembarrier.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindSanitizers.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindSourceLocation.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindStdAtomic.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findc-ares.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findcryptopp.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Finddpdk.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findhwloc.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findlksctp-tools.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findlz4.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findnumactl.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findragel.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findrt.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findyaml-cpp.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/SeastarDependencies.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindLibUring.cmake
    DESTINATION ${install_cmakedir})

  install (
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake/code_tests
    DESTINATION ${install_cmakedir})

  install (
    FILES ${CMAKE_CURRENT_BINARY_DIR}/seastar-install.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    RENAME seastar.pc)

  install (
    FILES ${CMAKE_CURRENT_BINARY_DIR}/seastar-testing-install.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    RENAME seastar-testing.pc)

  #
  # Export targets from the build tree for the user package registry.
  #

  export (
    EXPORT seastar-export
    FILE ${CMAKE_CURRENT_BINARY_DIR}/SeastarTargets.cmake
    NAMESPACE Seastar::)

  export (PACKAGE Seastar)

  #
  # Packaging.
  #

  set (CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set (CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set (CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

  include (CPack)
endif ()

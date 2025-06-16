cmake_minimum_required(VERSION 3.20)

function(find_current_module)
    set(SEARCH_LEVELS 3)
    get_filename_component(CURRENT_DIR ${CMAKE_SOURCE_DIR} ABSOLUTE)
    
    set(SEARCH_DIR ${CURRENT_DIR})
    
    foreach(LEVEL RANGE ${SEARCH_LEVELS})
        if(EXISTS "${SEARCH_DIR}/zephyr" AND IS_DIRECTORY "${SEARCH_DIR}/zephyr")
            if(EXISTS "${SEARCH_DIR}/zephyr/module.yml")
                set(module_dir ${SEARCH_DIR} PARENT_SCOPE)
                get_filename_component(module_name ${SEARCH_DIR} NAME)
                set(module_name ${module_name} PARENT_SCOPE)
                return()
            endif()
        endif()
        
        get_filename_component(PARENT_DIR ${SEARCH_DIR} DIRECTORY)
        if("${PARENT_DIR}" STREQUAL "${SEARCH_DIR}")
            break()
        endif()
        set(SEARCH_DIR ${PARENT_DIR})
    endforeach()
    
    message(FATAL_ERROR "No zephyr module directory found within ${SEARCH_LEVELS} parent directories")
endfunction()

function(parse_module_id input_string out_module_name out_module_dir)
  # Count colons to ensure exactly one
  string(REGEX MATCHALL ":" colon_matches "${input_string}")
  list(LENGTH colon_matches colon_count)

  if(NOT colon_count EQUAL 1)
    message(FATAL_ERROR "Input '${input_string}' must be in the format module_name:module_dir")
  endif()

  # Find the position of the colon
  string(FIND "${input_string}" ":" sep_pos)

  # Extract module_name and module_dir
  string(SUBSTRING "${input_string}" 0 ${sep_pos} module_name)
  math(EXPR dir_start "${sep_pos} + 1")
  string(SUBSTRING "${input_string}" ${dir_start} -1 module_dir)

  # Return the values through out parameters
  set(${out_module_name} "${module_name}" PARENT_SCOPE)
  set(${out_module_dir}  "${module_dir}"  PARENT_SCOPE)
endfunction()

function(zephyr_include_module module_id)

  if(${module_id} STREQUAL ":self")
    find_current_module()
  else()
    parse_module_id("${module_id}" module_name module_dir)
  endif()

  if(DEFINED EXTRA_ZEPHYR_MODULES)
    set(EXTRA_ZEPHYR_MODULES "${EXTRA_ZEPHYR_MODULES};${module_dir}" PARENT_SCOPE)
  else()
    set(EXTRA_ZEPHYR_MODULES "${module_dir}" PARENT_SCOPE)
  endif()

  if(DEFINED ZTEST_INCLUDE_EXTRA_ZEPHYR_MODULES)
    set(ZTEST_INCLUDE_EXTRA_ZEPHYR_MODULES "${ZTEST_INCLUDE_EXTRA_ZEPHYR_MODULES};${module_name}" PARENT_SCOPE)
  else()
    set(ZTEST_INCLUDE_EXTRA_ZEPHYR_MODULES "${module_name}" PARENT_SCOPE)
  endif()
endfunction()

# ------------------------------
# Test Case 1: Normal ID
# ------------------------------
# set(test_id_1 "my_mod:modules/my_mod")
# zephyr_include_module("${test_id_1}")
# message(STATUS "[Test 1] EXTRA_ZEPHYR_MODULES = ${EXTRA_ZEPHYR_MODULES}")
# message(STATUS "[Test 1] ZTEST_INCLUDE_EXTRA_ZEPHYR_MODULES = ${ZTEST_INCLUDE_EXTRA_ZEPHYR_MODULES}")

# ------------------------------
# Test Case 2: :self
# ------------------------------
# Only works if a zephyr/module.yml exists in a parent dir
# You can comment this out if not testing that case
zephyr_include_module(":self")
message(STATUS "[Test 2] EXTRA_ZEPHYR_MODULES = ${EXTRA_ZEPHYR_MODULES}")
message(STATUS "[Test 2] ZTEST_INCLUDE_EXTRA_ZEPHYR_MODULES = ${ZTEST_INCLUDE_EXTRA_ZEPHYR_MODULES}")


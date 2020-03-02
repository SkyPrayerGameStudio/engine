function(engine_resolve_dependencies target list)
	get_property(DEPENDENCIES GLOBAL PROPERTY ${target}_DEPENDENCIES)
	set(_local_deps)
	foreach (D ${DEPENDENCIES})
		if (NOT DEFINED ${D}_SOURCE_DIR)
			continue()
		endif()
		list(APPEND _local_deps ${D})
		set(_recursive_deps)
		engine_resolve_dependencies(${D} _recursive_deps)
		list(APPEND _local_deps ${_recursive_deps})
	endforeach()
	list(APPEND _local_deps ${${list}})
	list(REMOVE_DUPLICATES _local_deps)
	set(${list} ${_local_deps} PARENT_SCOPE)
endfunction()

# some cross compiling toolchains define this
if(NOT COMMAND find_host_program)
	macro(find_host_program)
		find_program(${ARGN})
	endmacro()
endif()

macro(convert_to_camel_case IN OUT)
	string(REPLACE "-" "_" _list ${IN})
	string(REPLACE "_" ";" _list ${_list})
	set(_final "")
	if (_list)
		foreach(_e ${_list})
			string(SUBSTRING ${_e} 0 1 _first_letter)
			string(TOUPPER ${_first_letter} _first_letter)
			string(SUBSTRING ${_e} 1 -1 _remaining)
			set(_final "${_final}${_first_letter}${_remaining}")
		endforeach()
	else()
		string(SUBSTRING ${IN} 0 1 _first_letter)
		string(TOUPPER ${_first_letter} _first_letter)
		string(SUBSTRING ${IN} 1 -1 _remaining)
		set(_final "${_final}${_first_letter}${_remaining}")
	endif()
	set(${OUT} ${_final})
endmacro()

macro(engine_mark_as_generated)
	set_source_files_properties(${ARGN} PROPERTIES GENERATED TRUE)
	#set_source_files_properties(${ARGN} PROPERTIES LANGUAGE CXX)
endmacro()

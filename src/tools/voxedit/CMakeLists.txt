include_directories(modules)
add_subdirectory(modules)

project(voxedit)
set(SRCS
	VoxEdit.h VoxEdit.cpp
)

set(FILES
	voxedit/voxedit-keybindings.cfg
	voxedit/scripts/test-import-plane.cfg
	voxedit/scripts/test-layers.cfg
	voxedit/scripts/test-animate.cfg
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES voxedit-ui voxedit-util)

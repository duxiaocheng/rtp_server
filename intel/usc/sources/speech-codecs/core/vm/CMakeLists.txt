target( vm core )

include_directories (
  ${CMAKE_SOURCE_DIR}/core/vm/include
)

add_definitions ( -D_CRT_NON_CONFORMING_SWPRINTFS -DVM_AFFINITY_ENABLED )
make_library( static )

target( speech_amrwbe codec/speech )

include_directories (
  ${CMAKE_SOURCE_DIR}/core/usc/include
  ${CMAKE_SOURCE_DIR}/codec/speech/common/include
  ${CMAKE_SOURCE_DIR}/codec/speech/amrwbe/include
  ${CMAKE_SOURCE_DIR}/codec/speech/amrwb/include
)

make_library( static )

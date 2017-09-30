target( speech_g723.1 codec/speech )

include_directories (
  ${CMAKE_SOURCE_DIR}/core/usc/include
  ${CMAKE_SOURCE_DIR}/codec/speech/common/include
  ${CMAKE_SOURCE_DIR}/codec/speech/g723.1/include
  ${CMAKE_SOURCE_DIR}/codec/speech/vad/include
)

make_library( static )

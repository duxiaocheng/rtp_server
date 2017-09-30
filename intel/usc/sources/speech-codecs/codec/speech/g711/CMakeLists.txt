target( speech_g711 codec/speech )

include_directories (
  ${CMAKE_SOURCE_DIR}/core/usc/include
  ${CMAKE_SOURCE_DIR}/codec/speech/common/include
  ${CMAKE_SOURCE_DIR}/codec/speech/g711/include
  ${CMAKE_SOURCE_DIR}/codec/speech/g729i/include
)

make_library( static )

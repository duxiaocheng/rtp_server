target( speech_alc codec/speech )

include_directories (
  ${CMAKE_SOURCE_DIR}/core/usc/include
)

make_library( static )

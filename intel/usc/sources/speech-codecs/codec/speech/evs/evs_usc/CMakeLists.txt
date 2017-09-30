target( evs_usc codec/speech/speech_evs )

include_directories (
  ${CMAKE_SOURCE_DIR}/core/usc/include
  ${CMAKE_SOURCE_DIR}/codec/speech/common/include
  ${CMAKE_SOURCE_DIR}/codec/speech/evs/include
  ${CMAKE_SOURCE_DIR}/codec/speech/evs/evs_usc
)


make_library( static )

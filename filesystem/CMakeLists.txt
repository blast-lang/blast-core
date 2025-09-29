blastcore_submodule(
  filesystem
  filesystem.hpp filesystem.cpp
)

target_link_libraries(filesystem ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
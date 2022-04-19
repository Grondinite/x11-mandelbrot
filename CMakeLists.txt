 cmake_minimum_required (VERSION 2.6)
 project (X11Pong CXX C)

 include (${CMAKE_ROOT}/Modules/FindX11.cmake)
 message("X11_FOUND: ${X11_FOUND}")

 add_executable(X11Pong main.c)
 install(TARGETS X11Pong DESTINATION bin)

 add_definitions(-DCMAKE_HAS_X)
 include_directories(${X11_INCLUDE_DIR})
 target_link_libraries(X11Pong ${X11_LIBRARIES})
 target_link_libraries(X11Pong m)

 # build a CPack driven installer package
 include(CPack)

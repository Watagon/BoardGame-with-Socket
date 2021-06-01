Project("Riversi" C)

# add_executable(riversi riversi.c)
# target_link_libraries(riversi X11)

add_executable(connect4_front connect4_front.c connect4.c)
target_link_libraries(connect4_front X11)
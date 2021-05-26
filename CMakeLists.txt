Project("Riversi" C)

# add_executable(riversi riversi.c)
# target_link_libraries(riversi X11)

add_executable(othello_front othello_front.c)
target_link_libraries(othello_front X11)
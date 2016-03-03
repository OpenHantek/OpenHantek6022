file(GLOB_RECURSE IMAGES "images/*.png" "images/*.icns" "images/*.svg")
install(FILES ${IMAGES} DESTINATION "images")

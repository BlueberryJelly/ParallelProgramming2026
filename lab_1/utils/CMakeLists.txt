add_library(utils STATIC)
target_sources(utils
    PUBLIC
    FILE_SET CXX_MODULES
    FILES
    matrix.cppm
    in_out_matrix.cppm
    utils.ixx)
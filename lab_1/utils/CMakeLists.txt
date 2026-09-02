add_library(utils STATIC)
target_sources(utils
    PUBLIC
    FILE_SET CXX_MODULES
    FILES
    in_out_data.cppm
    square_matrix.cppm
    utils.ixx)
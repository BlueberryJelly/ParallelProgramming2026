project(lab_03 LANGUAGES CXX)

find_package(MPI COMPONENTS CXX)

if(NOT MPI_CXX_FOUND)
    message(STATUS "MPI не найден — lab_03 исключена из сборки")
    return()
endif()

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE utils MPI::MPI_CXX)

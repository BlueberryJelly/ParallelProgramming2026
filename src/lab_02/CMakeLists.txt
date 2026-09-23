project(lab_02 LANGUAGES CXX)

find_package(OpenMP REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE utils OpenMP::OpenMP_CXX)

cmake_minimum_required(VERSION 3.11)

project(ImgConv CXX)
set(CMAKE_CXX_STANDARD 17)

# добавляем библиотеку, которую тоже надо будет собрать
add_subdirectory(../ImgLib ImgLibBuildDir)

if (CMAKE_SYSTEM_NAME MATCHES "^MINGW")
    set(SYSTEM_LIBS -lstdc++)
else()
    set(SYSTEM_LIBS)
endif()

# основная цель - конвертер изображения в main.cpp
add_executable(imgconv main.cpp)
# где искать include h-файлы
target_include_directories(imgconv PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../ImgLib")
# указания компоновщику
target_link_libraries(imgconv ImgLib ${SYSTEM_LIBS})

# Запускается из папки debug такими командами:
# cmake ../ImgConverter -DCMAKE_BUILD_TYPE=Debug -DLIBJPEG_DIR="Y:\cpp_trying\cpp_projects\SPRINT14\try_cmake_7_jpeg\libjpeg" -G "MinGW Makefiles"
# cmake --build . 

project(ALB)

include_directories("${PROJECT_SOURCE_DIR}/../include")

IF(WIN32)
  ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
ENDIF(WIN32)

add_library(ALB *.cpp)
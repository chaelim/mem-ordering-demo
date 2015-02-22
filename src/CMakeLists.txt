project(mem_order_demo)

file(GLOB SRCFILES *.cpp)
file(GLOB INCFILES *.h)

add_executable(${PROJECT_NAME}  ${SRCFILES} ${INCFILES})

cmake_minimum_required(VERSION 3.17)
project(GEDS VERSION 1.1)

add_subdirectory(sentry)
add_subdirectory(GEDS)

add_dependencies(GEDSv${GEDS_VERSION} sentry)

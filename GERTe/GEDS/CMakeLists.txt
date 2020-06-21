include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sentry/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SOURCES
	"*.cpp"
)

add_executable(GEDSv${GEDS_VERSION} ${SOURCES})

target_link_libraries(GEDSv${GEDS_VERSION} LINK_PUBLIC sentry)

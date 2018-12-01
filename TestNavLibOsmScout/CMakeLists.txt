# CMakeList.txt: CMake-Projekt für "TestNavLibOsmScout". Schließen Sie die Quelle ein, und definieren Sie
# projektspezifische Logik hier.
#
cmake_minimum_required (VERSION 3.8)

find_package(iconv)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
    SET (project_BIN ${PROJECT_NAME})
else()
    SET (project_BIN ${PROJECT_NAME}.bin)
endif()

if(OSMSCOUT_FOUND)
    SET ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOSMSCOUT")
    INCLUDE_DIRECTORIES(${OSMSCOUT_INCLUDE_DIRS})
    INCLUDE_DIRECTORIES(${CAIRO_INCLUDE_DIRS})
    INCLUDE_DIRECTORIES(${PANGO_INCLUDE_DIRS})
else()
	message(FATAL_ERROR "lib osm scout")
endif()

# Fügen Sie der ausführbaren Datei dieses Projekts eine Quelle hinzu.
add_executable (${project_BIN} "TestNavLibOsmScout.cpp" "TestNavLibOsmScout.h" "utils/easylogging++.cc" "NMEADecoder.cpp" "Tokenizer.cpp" "PathGenerator.cpp" "Simulator.cpp" "PathGeneratorNMEA.cpp")

if(OSMSCOUT_FOUND)
    TARGET_LINK_LIBRARIES(${project_BIN} ${OSMSCOUT_LIBRARIES})
    # TARGET_LINK_LIBRARIES(${project_BIN} ${CAIRO_LIBRARIES})
    if(WIN32)
        FOREACH(scoutlib ${OSMSCOUT_LIBRARIES})
            if(NOT scoutlib STREQUAL "debug" AND NOT scoutlib STREQUAL "optimized")
                # get_filename_component(DLLSOURCE ${scoutlib} PATH)
                string(REPLACE "/lib/" "/bin/" DLLSOURCE ${scoutlib})
				if(MINGW)
					string(REPLACE ".a" "" DLLSOURCE ${DLLSOURCE})
				else()
					string(REPLACE ".lib" ".dll" DLLSOURCE ${DLLSOURCE})
				endif()
                message(STATUS "Generating Copy Rule for OSMSCOUT Library ${DLLSOURCE}")   
                add_custom_command(TARGET ${project_BIN} POST_BUILD # Adds a post-build event to MyTest
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
                               "${DLLSOURCE}"      # <--this is in-file
                               $<TARGET_FILE_DIR:${project_BIN}>)
            endif()
        ENDFOREACH(scoutlib)
		FOREACH(scoutlib ${ICONV_LIBRARIES})
            if(NOT scoutlib STREQUAL "debug" AND NOT scoutlib STREQUAL "optimized")
                # get_filename_component(DLLSOURCE ${scoutlib} PATH)
                string(REPLACE "/lib/" "/bin/" DLLSOURCE ${scoutlib})
				if(MINGW)
					string(REPLACE ".a" "" DLLSOURCE ${DLLSOURCE})
				else()
					string(REPLACE ".lib" ".dll" DLLSOURCE ${DLLSOURCE})
				endif()
                message(STATUS "Generating Copy Rule for OSMSCOUT Library ${DLLSOURCE}")   
                add_custom_command(TARGET ${project_BIN} POST_BUILD # Adds a post-build event to MyTest
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
                               "${DLLSOURCE}"      # <--this is in-file
                               $<TARGET_FILE_DIR:${project_BIN}>)
            endif()
        ENDFOREACH(scoutlib)
    endif()
endif()

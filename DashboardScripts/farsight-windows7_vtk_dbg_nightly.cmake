SET(CTEST_SOURCE_NAME "src/vtk")
SET(CTEST_BINARY_NAME "bin/vtk-nightly")
SET(CTEST_DASHBOARD_ROOT "C:/dashboard")
SET(CTEST_SOURCE_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_SOURCE_NAME}")
SET(CTEST_BINARY_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_BINARY_NAME}")

SET (CTEST_START_WITH_EMPTY_BINARY_DIRECTORY TRUE)

SET(CTEST_COMMAND
  "\"c:/Program Files (x86)/CMake 2.8/bin/ctest.exe\" -V -VV -D Nightly -A \"${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}\""
  )

SET(CTEST_CMAKE_COMMAND
  "\"c:/Program Files (x86)/CMake 2.8/bin/cmake.exe\""
  )

find_program(CTEST_GIT_COMMAND NAMES "git.cmd" HINTS "C:/Program Files (x86)/Git/cmd/git.cmd")

SET( CMPLR_PATH "C:/Program Files (x86)/Microsoft Visual Studio 9.0/Common7/IDE/devenv.com" )
SET( VPROJ_PATH "C:/dashboard/bin/vtk-nightly/VTK.sln" )
SET( PRLL_STR_CXX "/DWIN32 /D_WINDOWS /W3 /Zm1000 /EHsc /GR /MP16" )
SET( PRLL_STR_C "/DWIN32 /D_WINDOWS /W3 /Zm1000 /MP16" )

SET(CTEST_INITIAL_CACHE "
SITE:STRING=farsight-win_7_64
BUILDNAME:STRING=vs9-64-dbg-use-boost-svn
CMAKE_GENERATOR:INTERNAL=Visual Studio 9 2008 Win64
MAKECOMMAND:STRING=${CMPLR_PATH} ${VPROJ_PATH} /build Debug /project ALL_BUILD
CMAKE_CXX_FLAGS:STRING=${PRLL_STR_CXX}
CMAKE_C_FLAGS:STRING=${PRLL_STR_C}
QT_QMAKE_EXECUTABLE:STRING=C:/Qt/4.8.1/bin/qmake.exe
Boost_INCLUDE_DIR:STRING=C:/dashboard/src/boost
BUILD_SHARED_LIBS:BOOL=OFF
BUILD_EXAMPLES:BOOL=ON
BUILD_DOXYGEN:BOOL=OFF
BUILD_TESTING:BOOL=ON
VTK_USE_GUISUPPORT:BOOL=ON
VTK_USE_QT:BOOL=ON
VTK_USE_QTCHARTS:BOOL=ON
VTK_USE_BOOST:BOOL=ON
")

#SET(CTEST_GIT_COMMAND "C:/Program Files (x86)/Git/cmd/git.cmd")


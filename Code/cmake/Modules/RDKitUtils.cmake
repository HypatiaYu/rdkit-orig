include(BoostUtils)

macro(rdkit_library)
  PARSE_ARGUMENTS(RDKLIB
    "LINK_LIBRARIES;DEST"
    "SHARED"
    ${ARGN})
  CAR(RDKLIB_NAME ${RDKLIB_DEFAULT_ARGS})
  CDR(RDKLIB_SOURCES ${RDKLIB_DEFAULT_ARGS})
  if(MSVC)
    add_library(${RDKLIB_NAME} ${RDKLIB_SOURCES})
  else(MSVC)
    IF(RDKLIB_SHARED)
      add_library(${RDKLIB_NAME} SHARED ${RDKLIB_SOURCES})
      INSTALL(TARGETS ${RDKLIB_NAME} 
              DESTINATION ${RDKit_LibDir}/${RDKLIB_DEST})
    ELSE(RDKLIB_SHARED)        
      add_library(${RDKLIB_NAME} ${RDKLIB_SOURCES})
    ENDIF(RDKLIB_SHARED)        
    IF(RDKLIB_LINK_LIBRARIES)
      target_link_libraries(${RDKLIB_NAME} ${RDKLIB_LINK_LIBRARIES})
    ENDIF(RDKLIB_LINK_LIBRARIES)
  endif(MSVC)
endmacro(rdkit_library)
  
macro(rdkit_python_extension)
  PARSE_ARGUMENTS(RDKPY
    "LINK_LIBRARIES;DEPENDS;DEST"
    ""
    ${ARGN})
  CAR(RDKPY_NAME ${RDKPY_DEFAULT_ARGS})
  CDR(RDKPY_SOURCES ${RDKPY_DEFAULT_ARGS})
  PYTHON_ADD_MODULE(${RDKPY_NAME} ${RDKPY_SOURCES})
  set_target_properties(${RDKPY_NAME} PROPERTIES PREFIX "")
if(MSVC)
  set_target_properties(${RDKPY_NAME} PROPERTIES SUFFIX ".pyd")
endif(MSVC)  
  target_link_libraries(${RDKPY_NAME} ${RDKPY_LINK_LIBRARIES} 
                        ${PYTHON_LIBRARIES} ${Boost_LIBRARIES})

  INSTALL(TARGETS ${RDKPY_NAME} 
          LIBRARY DESTINATION ${RDKit_PythonDir}/${RDKPY_DEST})
endmacro(rdkit_python_extension)

macro(rdkit_test)
  PARSE_ARGUMENTS(RDKTEST
    "LINK_LIBRARIES;DEPENDS;DEST"
    ""
    ${ARGN})
  CAR(RDKTEST_NAME ${RDKTEST_DEFAULT_ARGS})
  CDR(RDKTEST_SOURCES ${RDKTEST_DEFAULT_ARGS})
  add_executable(${RDKTEST_NAME} ${RDKTEST_SOURCES})
  target_link_libraries(${RDKTEST_NAME} ${RDKTEST_LINK_LIBRARIES})
  add_test(${RDKTEST_NAME} ${EXECUTABLE_OUTPUT_PATH}/${RDKTEST_NAME})
endmacro(rdkit_test)

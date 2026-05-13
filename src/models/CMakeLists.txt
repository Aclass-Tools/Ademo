add_library(AclassModelSources INTERFACE)

set(ACLASS_MODEL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/pointtablemodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/jsonpreviewparser.cpp
)

set(ACLASS_MODEL_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/pointtablemodel.h
    ${CMAKE_CURRENT_LIST_DIR}/jsonpreviewparser.h
    ${CMAKE_CURRENT_LIST_DIR}/projectsummarycontext.h
)

target_sources(AclassModelSources INTERFACE
    ${ACLASS_MODEL_SOURCES}
    ${ACLASS_MODEL_HEADERS}
)

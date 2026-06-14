add_library(AclassModelSources INTERFACE)

set(ACLASS_MODEL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/pointtablemodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/jsonpreviewparser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/binprotocolloader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/protocolfieldtablemodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/framecodec.cpp
)

set(ACLASS_MODEL_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/pointtablemodel.h
    ${CMAKE_CURRENT_LIST_DIR}/jsonpreviewparser.h
    ${CMAKE_CURRENT_LIST_DIR}/projectsummarycontext.h
    ${CMAKE_CURRENT_LIST_DIR}/binprotocolformat.h
    ${CMAKE_CURRENT_LIST_DIR}/binprotocolloader.h
    ${CMAKE_CURRENT_LIST_DIR}/protocolfieldtablemodel.h
    ${CMAKE_CURRENT_LIST_DIR}/framecodec.h
)

target_sources(AclassModelSources INTERFACE
    ${ACLASS_MODEL_SOURCES}
    ${ACLASS_MODEL_HEADERS}
)

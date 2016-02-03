#-------------------------------------------------------------------------------
# terrainproject
#-------------------------------------------------------------------------------

# glob the source files and add them to folders in visual studio
FILE(GLOB SRC "gamestates/*.cc")
FILE(GLOB HEAD "gamestates/*.h")
LIST(APPEND SRCSGAMESTATES ${SRC} ${HEAD})
SOURCE_GROUP("terrainproject\\gamestates" FILES ${SRCSGAMESTATES})

FILE(GLOB SRC "*.cc")
FILE(GLOB HEAD "*.h")
LIST(APPEND SRCSBASE ${SRC} ${HEAD})
SOURCE_GROUP("terrainproject" FILES ${SRCSBASE})

# define precompiled headers
SET(PCH_files	
    ../stdneb.cc
)
SOURCE_GROUP("Precompiled" FILES ${PCH_files})

LIST(APPEND ALLSRCS ${SRCSBASE} ${SRCSGAMESTATES} ${PCH_files})

ADD_EXECUTABLE(terrainproject ${ALLSRCS})
SET_TARGET_PROPERTIES(terrainproject PROPERTIES FOLDER "terrainproject")

# link to application project, will pull in all dependencies and includes required
TARGET_LINK_LIBRARIES(terrainproject application)
ADD_DEPENDENCIES(terrainproject application)

# apply default compiler settings, has to be done to be consistent throughout
N_SET_COMPILER_DEFAULTS()
N_SET_EXE_LINKER_DEFAULTS(terrainproject)
N_SET_EXE_SUBSYSTEM(terrainproject Windows)

# setup precompiled headers if chosen at project level
IF(N_USE_PRECOMPILED_HEADERS)
	N_SET_PCH(terrainproject  stdneb.h stdneb.cc ../)
ENDIF()
#-------------------------------------------------------------------------------
# terrainrenderer
#-------------------------------------------------------------------------------

# glob the source files and add them to folders in visual studio
FILE(GLOB SRC "gamestates/*.cc")
FILE(GLOB HEAD "gamestates/*.h")
LIST(APPEND SRCSGAMESTATES ${SRC} ${HEAD})
SOURCE_GROUP("terrain-renderer\\gamestates" FILES ${SRCSGAMESTATES})

FILE(GLOB SRC "terrainentity/*.cc")
FILE(GLOB HEAD "terrainentity/*.h")
LIST(APPEND SRCTERRAINENTITY ${SRC} ${HEAD})
SOURCE_GROUP("terrain-renderer\\terrainentity" FILES ${SRCTERRAINENTITY})

FILE(GLOB SRC "*.cc")
FILE(GLOB HEAD "*.h")
LIST(APPEND SRCSBASE ${SRC} ${HEAD})
SOURCE_GROUP("terrain-renderer" FILES ${SRCSBASE})

# define precompiled headers
SET(PCH_files	
    ../stdneb.cc
)
SOURCE_GROUP("Precompiled" FILES ${PCH_files})

LIST(APPEND ALLSRCS ${SRCSBASE} ${SRCSGAMESTATES} ${SRCTERRAINENTITY} ${PCH_files})

ADD_EXECUTABLE(terrain-renderer ${ALLSRCS})
SET_TARGET_PROPERTIES(terrain-renderer PROPERTIES FOLDER "terrain-renderer")

# link to application project, will pull in all dependencies and includes required
TARGET_LINK_LIBRARIES(terrain-renderer application)
ADD_DEPENDENCIES(terrain-renderer application)

# apply default compiler settings, has to be done to be consistent throughout
N_SET_COMPILER_DEFAULTS()
N_SET_EXE_LINKER_DEFAULTS(terrain-renderer)
N_SET_EXE_SUBSYSTEM(terrain-renderer Windows)

# setup precompiled headers if chosen at project level
IF(N_USE_PRECOMPILED_HEADERS)
	N_SET_PCH(terrain-renderer  stdneb.h stdneb.cc ../)
ENDIF()
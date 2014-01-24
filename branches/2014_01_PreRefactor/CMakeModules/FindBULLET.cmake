# Locate Bullet
# This module defines XXX_FOUND, XXX_INCLUDE_DIRS and XXX_LIBRARIES standard variables
#
# Define BULLET_SINGLE_THREADED to "YES" to search for single threaded variant.

INCLUDE(FindPackageTargetLibraries)

SET(BULLET_MSVC_LIB_DIR "")
IF(CMAKE_GENERATOR MATCHES "Visual Studio")
	IF (${CMAKE_GENERATOR} MATCHES "2008")
		SET(BULLET_MSVC_LIB_DIR "9")
	ELSEIF (${CMAKE_GENERATOR} MATCHES "2005")
		SET(BULLET_MSVC_LIB_DIR "8")
	ELSEIF (${CMAKE_GENERATOR} MATCHES "2003")
		SET(BULLET_MSVC_LIB_DIR "71")
	ELSEIF (${CMAKE_GENERATOR} MATCHES "2002")
		SET(BULLET_MSVC_LIB_DIR "7")
	ELSEIF (${CMAKE_GENERATOR} MATCHES "6")
		SET(BULLET_MSVC_LIB_DIR "6")
	ENDIF (${CMAKE_GENERATOR} MATCHES "2008")
ENDIF()

FIND_PATH(BULLET_INCLUDE_DIR btBulletDynamicsCommon.h
	HINTS
	$ENV{BULLET_DIR}
	$ENV{BULLET_PATH}
	${ADDITIONAL_SEARCH_PATHS}
	PATH_SUFFIXES include src include/bullet
)

IF(BULLET_SINGLE_THREADED)
	# WARNING: BulletMath is the name found in MSVC prebuilt bojects, LinearMath is what is found using the CMake script. The case is handled below.
	SET(BULLET_LIBS "BulletSoftBody" "BulletDynamics" "BulletCollision" "LinearMath")		# Tested with Bullet 2.79
ELSE()
	SET(BULLET_LIBS "BulletSoftBody" "BulletDynamics" "BulletCollision" "LinearMath" "BulletMultiThreaded")		# Tested with Bullet 2.79
ENDIF()
SET(BULLET_LIBRARIES)

FOREACH(CUR_LIB ${BULLET_LIBS})
	STRING(TOLOWER "${CUR_LIB}" CUR_LIB_LOWER)

	if (${CUR_LIB} STREQUAL LinearMath)
	    SET(ADDITIONAL_NAMES "LibBulletMath libbulletmath BulletMath bulletMath")
	    SET(ADDITIONAL_NAMES_DEBUG "LibBulletMath_d libbulletmath_d BulletMath_d bulletMath_d LibBulletMath_debug libbulletmath_debug BulletMath_debug bulletMath_debug")
 	else()
            SET(ADDITIONAL_NAMES "")
	    SET(ADDITIONAL_NAMES_DEBUG "") 
        endif()

	FIND_LIBRARY(BULLET_LIBRARY_${CUR_LIB}
		NAMES "Lib${CUR_LIB}" "lib${CUR_LIB_LOWER}" ${CUR_LIB} ${CUR_LIB_LOWER} ${ADDITIONAL_NAMES}
		HINTS
			$ENV{BULLET_DIR}
			$ENV{BULLET_PATH}
			${ADDITIONAL_SEARCH_PATHS}
		PATH_SUFFIXES lib64 lib src "src/${CUR_LIB}" "src/${CUR_LIB_LOWER}" "out/release_dll${BULLET_MSVC_LIB_DIR}/libs" "out/release${BULLET_MSVC_LIB_DIR}/libs" "out/release_dll${BULLET_MSVC_LIB_DIR}/build/lib${CUR_LIB_LOWER}" 
	)

	FIND_LIBRARY(BULLET_LIBRARY_${CUR_LIB}_DEBUG
		NAMES "Lib${CUR_LIB}d" "lib${CUR_LIB_LOWER}d" "${CUR_LIB}d" "${CUR_LIB_LOWER}d" "Lib${CUR_LIB}_d" "lib${CUR_LIB_LOWER}_d" "${CUR_LIB}_d" "${CUR_LIB_LOWER}_d" "${CUR_LIB_LOWER}_debug" "${CUR_LIB}_debug"  "lib${CUR_LIB_LOWER}" ${ADDITIONAL_NAMES_DEBUG} 
		HINTS
			$ENV{BULLET_DIR}
			$ENV{BULLET_PATH}
			${ADDITIONAL_SEARCH_PATHS}
		PATH_SUFFIXES lib64 lib src "src/${CUR_LIB}" "src/${CUR_LIB_LOWER}" "out/debug_dll${BULLET_MSVC_LIB_DIR}/libs" "out/debug${BULLET_MSVC_LIB_DIR}/libs" "out/debug_dll${BULLET_MSVC_LIB_DIR}/build/lib${CUR_LIB_LOWER}" 
	)

ENDFOREACH()



# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if 
# all listed variables are TRUE
SET(BULLET_LIBRARY_FULL_LIST)
FOREACH(CUR_LIB ${BULLET_LIBS})
	LIST(APPEND BULLET_LIBRARY_FULL_LIST "BULLET_LIBRARY_${CUR_LIB}")
ENDFOREACH()


INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BULLET DEFAULT_MSG ${BULLET_LIBRARY_FULL_LIST} BULLET_INCLUDE_DIR)

IF(BULLET_FOUND)
	# Combine all libs to one variable
	FOREACH(CUR_LIB_VARNAME ${BULLET_LIBRARY_FULL_LIST})
		IF(${CUR_LIB_VARNAME})
			FIND_PACKAGE_ADD_TARGET_LIBRARIES(BULLET "${${CUR_LIB_VARNAME}}" "${${CUR_LIB_VARNAME}_DEBUG}")
		ENDIF()
	ENDFOREACH()

	SET(BULLET_INCLUDE_DIRS ${BULLET_INCLUDE_DIR})
	
	if (BULLET_DOUBLE_PRECISION)
	   ADD_DEFINITIONS(-DBT_USE_DOUBLE_PRECISION)
	endif()

   if(BULLET_SINGLE_THREADED)
      ADD_DEFINITIONS(-DBULLET_SINGLETHREAD)
   endif()
	
ELSE()
	SET(BULLET_LIBRARIES)
	SET(BULLET_INCLUDE_DIRS)
ENDIF()

##############################################################################
# AzerothCore Custom Configuration
# These values override the defaults in conf/dist/config.cmake
# Use CACHE and FORCE to ensure proper override of cache variables
##############################################################################

set(SCRIPTS "static" CACHE STRING "Build core with scripts" FORCE)
set(MODULES "static" CACHE STRING "Build core with modules" FORCE)
set(APPS_BUILD "all" CACHE STRING "Build list for applications" FORCE)
set(TOOLS_BUILD "all" CACHE STRING "Build list for tools" FORCE)

# Re-run build list checks to update BUILD_TOOLS_MAPS and BUILD_TOOLS_DB_IMPORT
# These were already called in conf/dist/config.cmake with the old values
CheckApplicationsBuildList()
CheckToolsBuildList()

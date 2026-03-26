# idacpp_plugin_helpers.cmake — thin wrappers around clinglite's plugin helpers.
#
# All real logic lives in clinglite_plugin_helpers.cmake. These wrappers
# accumulate into IDACPP_PLUGIN_* variables (separate from CLINGLITE_PLUGIN_*).
#
# Stateless utilities (bridge gen, make_shared, dispatch gen) are used
# directly from clinglite — no wrappers needed.

include("${CLINGLITE_SOURCE_DIR}/plugins/common/clinglite_plugin_helpers.cmake")

# Registration (accumulates into IDACPP_PLUGIN_*) 
function(idacpp_register_plugin PLUGIN_NAME SETUP_FN SETUP_HEADER SETUP_SOURCE)
    _clinglite_plugin_register_impl(IDACPP
        ${PLUGIN_NAME} ${SETUP_FN} ${SETUP_HEADER} ${SETUP_SOURCE})
endfunction()

# PCH contribution (accumulates into IDACPP_PLUGIN_PCH_*) 
function(idacpp_plugin_extend_pch)
    _clinglite_plugin_extend_pch_impl(IDACPP ${ARGN})
endfunction()

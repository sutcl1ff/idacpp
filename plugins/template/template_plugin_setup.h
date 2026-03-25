// <plugin_name>_plugin_setup.h — <PLUGIN_NAME> extension plugin for idacpp.
//
// Rename this file to <plugin_name>_plugin_setup.h
#pragma once

namespace clinglite { class Interpreter; }

/// Set up the <PLUGIN_NAME> plugin on an initialized interpreter.
void template_plugin_setup(clinglite::Interpreter& interp, bool hasPch);

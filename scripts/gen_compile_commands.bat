pushd ..
premake5 export-compile-commands
cp compile_commands\debug_x86_64.json compile_commands.json
popd

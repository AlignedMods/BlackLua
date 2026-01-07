pushd ..
premake5 gmake --cc=clang
popd

gen_compile_commands.bat

mkdir ./build/
cd ./build/
cmake ..
make
cp ./compile_commands.json ../compile_commands.json
./amp ../../../Music/

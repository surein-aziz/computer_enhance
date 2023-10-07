pushd haversine
CALL build.bat
popd
pushd json_generator
CALL build.bat
popd
pushd repetition_tester
CALL build.bat
CALL build_rel.bat
popd
pushd x64_simulator
CALL build.bat
popd
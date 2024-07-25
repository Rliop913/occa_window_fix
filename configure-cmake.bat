
if "%BUILD_DIR%"=="" set "BUILD_DIR=%cd%\build"
if "%INSTALL_DIR%"=="" set "INSTALL_DIR=%cd%\install"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=RelWithDebInfo"

if "%CC%"=="" set "CC=gcc"
if "%CXX%"=="" set "CXX=g++"
if "%FC%"=="" set "FC=gfortran"

if "%OCCA_ENABLE_DPCPP%"=="" set "OCCA_ENABLE_DPCPP=ON"
if "%OCCA_ENABLE_OPENCL%"=="" set "OCCA_ENABLE_OPENCL=ON"
if "%OCCA_ENABLE_CUDA%"=="" set "OCCA_ENABLE_CUDA=ON"
if "%OCCA_ENABLE_HIP%"=="" set "OCCA_ENABLE_HIP=ON"
if "%OCCA_ENABLE_OPENMP%"=="" set "OCCA_ENABLE_OPENMP=ON"
if "%OCCA_ENABLE_METAL%"=="" set "OCCA_ENABLE_METAL=ON"
if "%OCCA_ENABLE_FORTRAN%"=="" set "OCCA_ENABLE_FORTRAN=OFF"
if "%OCCA_ENABLE_TESTS%"=="" set "OCCA_ENABLE_TESTS=ON"
if "%OCCA_ENABLE_EXAMPLES%"=="" set "OCCA_ENABLE_EXAMPLES=ON"

:: Run CMake with specified parameters
cmake -S . -B "%BUILD_DIR%" ^
  -DCMAKE_CXX_STANDARD=17 ^
  -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
  -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% ^
  -DCMAKE_C_COMPILER=%CC% ^
  -DCMAKE_CXX_COMPILER=%CXX% ^
  -DCMAKE_Fortran_COMPILER=%FC% ^
  -DCMAKE_CXX_FLAGS="%CXXFLAGS%" ^
  -DCMAKE_C_FLAGS="%CFLAGS%" ^
  -DCMAKE_Fortran_FLAGS="%FFLAGS%" ^
  -DOCCA_ENABLE_OPENMP=%OCCA_ENABLE_OPENMP% ^
  -DOCCA_ENABLE_OPENCL=%OCCA_ENABLE_OPENCL% ^
  -DOCCA_ENABLE_DPCPP=%OCCA_ENABLE_DPCPP% ^
  -DOCCA_ENABLE_CUDA=%OCCA_ENABLE_CUDA% ^
  -DOCCA_ENABLE_HIP=%OCCA_ENABLE_HIP% ^
  -DOCCA_ENABLE_METAL=%OCCA_ENABLE_METAL% ^
  -DOCCA_ENABLE_FORTRAN=%OCCA_ENABLE_FORTRAN% ^
  -DOCCA_ENABLE_TESTS=%OCCA_ENABLE_TESTS% ^
  -DOCCA_ENABLE_EXAMPLES=%OCCA_ENABLE_EXAMPLES%

:: Build the project
::cmake --build "%BUILD_DIR%" --target install

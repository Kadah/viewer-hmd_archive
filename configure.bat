
@ECHO OFF
SET ENABLE_UNIT_TESTS=ON
FOR %%A IN (%*) DO (
IF /I "%%A"=="u" SET ENABLE_UNIT_TESTS=ON
IF /I "%%A"=="+u" SET ENABLE_UNIT_TESTS=ON
IF /I "%%A"=="-u" SET ENABLE_UNIT_TESTS=OFF
)

@ECHO ON
autobuild configure -- -DLL_TESTS=%ENABLE_UNIT_TESTS%

pause
echo done

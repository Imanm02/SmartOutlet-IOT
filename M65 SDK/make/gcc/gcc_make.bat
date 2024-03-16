@ECHO OFF

set MAKE=%CSDTK4INSTALLDIR%%CSDTKMAKEDIR%\make.exe

IF /i "%1" == "new" (
	%MAKE% -f make/gcc/gcc_makefile %1 2>&1|tools\tee build/gcc/build.log
) ELSE (
	IF /i "%1" == "clean" (
		%MAKE% -f make/gcc/gcc_makefile %1
		IF EXIST build\gcc\build.log (
			@del /f build\gcc\build.log
		)
	) ELSE (
		IF /i "%1" == "help" (
			%MAKE% -f make/gcc/gcc_makefile %1
		) ELSE (
			ECHO I AM here.
			ECHO Incorect input argument.
		)
	)
)
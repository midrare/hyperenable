SHELL = cmd.exe
RM = del -F -Q
RMDIR = rmdir -S -Q
MKDIR = mkdir
COPY = xcopy /E /C /I /-I /Q /Y

all: src
.PHONY: all src clean

src:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
	@echo > .dummy.tmp
	@$(RM) .dummy.tmp hyper*.exe hyper*.zip

# tar is bundled with Windows 10+
release: src
	@$(COPY) src\\hyperenable.exe .\\
	tar -a -c -f hyperenable-release.zip example_config.yaml hyperenable.exe install.ps1 LICENSE.md README.md
	@$(RM) hyperenable.exe

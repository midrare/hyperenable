RM = del -F -Q
RMDIR = rmdir -S -Q
MKDIR = mkdir
COPY = xcopy /E /C /I /-I /Q /Y

all: src
.PHONY: all src clean

src:
	$(MAKE) -C src
	@$(COPY) src\\*.exe .\\

clean:
	$(MAKE) -C src clean
	@$(RM) hyper*.exe

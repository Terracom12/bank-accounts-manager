.PHONY: all
all:
	cmake -S . -B build -G "Unix Makefiles"
	cmake --build build --config Release
	cp ./build/bank_accounts_exe ./hw06

.PHONY: scr
scr:
	script -c "export PS1='> '; /bin/sh --norc" hw06.scr

.PHONY: tar
tar:
	git ls-files | grep -v .vscode | xargs tar -cf hw06.tar hw06.scr
